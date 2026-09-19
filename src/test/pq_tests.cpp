#include <boost/test/unit_test.hpp>

#include <crypto/pq/pq.h>
#include <crypto/sha256.h>
#include <primitives/transaction.h>
#include <script/interpreter.h>
#include <script/script.h>

#include <cstring>
#include <vector>

BOOST_AUTO_TEST_SUITE(pq_tests)

template <typename Scheme>
static void roundtrip_and_tamper()
{
    std::vector<unsigned char> pk(Scheme::PUBKEY_BYTES);
    std::vector<unsigned char> sk(Scheme::SECKEY_BYTES);
    std::vector<unsigned char> sig(Scheme::SIG_BYTES);

    BOOST_REQUIRE(Scheme::keygen(pk.data(), sk.data()));

    const unsigned char msg[] = "Vexta post-quantum test message";
    size_t siglen = 0;

    BOOST_REQUIRE(Scheme::sign(
        sig.data(),
        &siglen,
        msg,
        sizeof(msg) - 1,
        sk.data()
    ));

    BOOST_CHECK_EQUAL(siglen, Scheme::SIG_BYTES);

    BOOST_CHECK(Scheme::verify(
        sig.data(),
        siglen,
        msg,
        sizeof(msg) - 1,
        pk.data()
    ));

    unsigned char tampered[sizeof(msg)];
    std::memcpy(tampered, msg, sizeof(msg));
    tampered[0] ^= 0x01;

    BOOST_CHECK(!Scheme::verify(
        sig.data(),
        siglen,
        tampered,
        sizeof(msg) - 1,
        pk.data()
    ));
}

BOOST_AUTO_TEST_CASE(signature_hash_pqr_binds_transaction_fields)
{
    CMutableTransaction tx;
    tx.nVersion = 2;
    tx.nLockTime = 500;

    tx.vin.resize(1);
    tx.vin[0].prevout = COutPoint(uint256S("01"), 3);
    tx.vin[0].nSequence = 0xfffffffe;

    tx.vout.emplace_back(123456, CScript() << OP_TRUE);

    std::vector<unsigned char> program(32);
    for (size_t i = 0; i < program.size(); ++i) {
        program[i] = static_cast<unsigned char>(i);
    }

    const CAmount amount = 987654;

    auto get_hash = [&](const CMutableTransaction& t,
                        int witness_version,
                        const std::vector<unsigned char>& prog,
                        CAmount amt) {
        PrecomputedTransactionData cache;
        cache.Init(t, {}, true);
        BOOST_REQUIRE(cache.m_bip143_segwit_ready);
        return SignatureHashPQR(t, 0, witness_version, prog, amt, cache);
    };

    const uint256 base = get_hash(tx, 2, program, amount);

    BOOST_CHECK(base != get_hash(tx, 3, program, amount));

    auto changed_program = program;
    changed_program[0] ^= 1;
    BOOST_CHECK(base != get_hash(tx, 2, changed_program, amount));

    BOOST_CHECK(base != get_hash(tx, 2, program, amount + 1));

    {
        auto changed = tx;
        changed.vin[0].prevout.n = 4;
        BOOST_CHECK(base != get_hash(changed, 2, program, amount));
    }

    {
        auto changed = tx;
        changed.vin[0].nSequence--;
        BOOST_CHECK(base != get_hash(changed, 2, program, amount));
    }

    {
        auto changed = tx;
        changed.vout[0].nValue++;
        BOOST_CHECK(base != get_hash(changed, 2, program, amount));
    }

    {
        auto changed = tx;
        changed.nVersion++;
        BOOST_CHECK(base != get_hash(changed, 2, program, amount));
    }

    {
        auto changed = tx;
        changed.nLockTime++;
        BOOST_CHECK(base != get_hash(changed, 2, program, amount));
    }
}


BOOST_AUTO_TEST_CASE(mldsa65_witness_v2_verify_script)
{
    std::vector<unsigned char> pk(pq::MLDSA65::PUBKEY_BYTES);
    std::vector<unsigned char> sk(pq::MLDSA65::SECKEY_BYTES);
    std::vector<unsigned char> sig(pq::MLDSA65::SIG_BYTES);

    BOOST_REQUIRE(pq::MLDSA65::keygen(pk.data(), sk.data()));

    uint256 pubkey_hash;
    CSHA256().Write(pk.data(), pk.size()).Finalize(pubkey_hash.begin());
    const std::vector<unsigned char> program(pubkey_hash.begin(), pubkey_hash.end());

    const CScript script_pub_key = CScript() << OP_2 << program;
    const CAmount amount = 500000;

    CMutableTransaction tx;
    tx.nVersion = 2;
    tx.vin.resize(1);
    tx.vin[0].prevout = COutPoint(uint256S("01"), 0);
    tx.vin[0].nSequence = 0xfffffffe;
    tx.vout.emplace_back(400000, CScript() << OP_TRUE);

    std::vector<CTxOut> spent_outputs;
    spent_outputs.emplace_back(amount, script_pub_key);

    PrecomputedTransactionData txdata;
    txdata.Init(tx, std::move(spent_outputs));
    BOOST_REQUIRE(txdata.m_bip143_segwit_ready);

    const uint256 sighash = SignatureHashPQR(tx, 0, 2, program, amount, txdata);

    size_t siglen = 0;
    BOOST_REQUIRE(pq::MLDSA65::sign(
        sig.data(),
        &siglen,
        sighash.begin(),
        32,
        sk.data()
    ));
    BOOST_REQUIRE_EQUAL(siglen, pq::MLDSA65::SIG_BYTES);

    tx.vin[0].scriptWitness.stack.clear();
    tx.vin[0].scriptWitness.stack.push_back(sig);
    tx.vin[0].scriptWitness.stack.push_back(pk);

    const CTransaction final_tx(tx);

    PrecomputedTransactionData final_txdata;
    std::vector<CTxOut> final_spent_outputs;
    final_spent_outputs.emplace_back(amount, script_pub_key);
    final_txdata.Init(final_tx, std::move(final_spent_outputs));
    BOOST_REQUIRE(final_txdata.m_bip143_segwit_ready);

    ScriptError err = SCRIPT_ERR_UNKNOWN_ERROR;
    TransactionSignatureChecker checker(
        &final_tx,
        0,
        amount,
        final_txdata,
        MissingDataBehavior::ASSERT_FAIL
    );

    BOOST_CHECK(VerifyScript(
        final_tx.vin[0].scriptSig,
        script_pub_key,
        &final_tx.vin[0].scriptWitness,
        SCRIPT_VERIFY_WITNESS | SCRIPT_VERIFY_PQR,
        checker,
        &err
    ));
    BOOST_CHECK_EQUAL(err, SCRIPT_ERR_OK);

    CMutableTransaction bad_tx(final_tx);
    bad_tx.vin[0].scriptWitness.stack[0][0] ^= 0x01;
    const CTransaction bad_final_tx(bad_tx);

    PrecomputedTransactionData bad_txdata;
    std::vector<CTxOut> bad_spent_outputs;
    bad_spent_outputs.emplace_back(amount, script_pub_key);
    bad_txdata.Init(bad_final_tx, std::move(bad_spent_outputs));

    ScriptError bad_err = SCRIPT_ERR_UNKNOWN_ERROR;
    TransactionSignatureChecker bad_checker(
        &bad_final_tx,
        0,
        amount,
        bad_txdata,
        MissingDataBehavior::ASSERT_FAIL
    );

    BOOST_CHECK(!VerifyScript(
        bad_final_tx.vin[0].scriptSig,
        script_pub_key,
        &bad_final_tx.vin[0].scriptWitness,
        SCRIPT_VERIFY_WITNESS | SCRIPT_VERIFY_PQR,
        bad_checker,
        &bad_err
    ));
    BOOST_CHECK_EQUAL(bad_err, SCRIPT_ERR_PQR_SIG_VERIFY);

    // Before PQR activation, witness v2 keeps future-witness semantics.
    ScriptError preactivation_err = SCRIPT_ERR_UNKNOWN_ERROR;
    TransactionSignatureChecker preactivation_checker(
        &bad_final_tx,
        0,
        amount,
        bad_txdata,
        MissingDataBehavior::ASSERT_FAIL
    );

    BOOST_CHECK(VerifyScript(
        bad_final_tx.vin[0].scriptSig,
        script_pub_key,
        &bad_final_tx.vin[0].scriptWitness,
        SCRIPT_VERIFY_WITNESS,
        preactivation_checker,
        &preactivation_err
    ));
    BOOST_CHECK_EQUAL(preactivation_err, SCRIPT_ERR_OK);

    // A modified public key must fail the witness-program binding.
    CMutableTransaction bad_pubkey_tx(final_tx);
    bad_pubkey_tx.vin[0].scriptWitness.stack[1][0] ^= 0x01;
    const CTransaction bad_pubkey_final_tx(bad_pubkey_tx);

    PrecomputedTransactionData bad_pubkey_txdata;
    std::vector<CTxOut> bad_pubkey_spent_outputs;
    bad_pubkey_spent_outputs.emplace_back(amount, script_pub_key);
    bad_pubkey_txdata.Init(bad_pubkey_final_tx, std::move(bad_pubkey_spent_outputs));

    ScriptError bad_pubkey_err = SCRIPT_ERR_UNKNOWN_ERROR;
    TransactionSignatureChecker bad_pubkey_checker(
        &bad_pubkey_final_tx,
        0,
        amount,
        bad_pubkey_txdata,
        MissingDataBehavior::ASSERT_FAIL
    );

    BOOST_CHECK(!VerifyScript(
        bad_pubkey_final_tx.vin[0].scriptSig,
        script_pub_key,
        &bad_pubkey_final_tx.vin[0].scriptWitness,
        SCRIPT_VERIFY_WITNESS | SCRIPT_VERIFY_PQR,
        bad_pubkey_checker,
        &bad_pubkey_err
    ));
    BOOST_CHECK_EQUAL(bad_pubkey_err, SCRIPT_ERR_PQR_SIG_VERIFY);

    // PQR witness must contain exactly [signature, public key].
    CMutableTransaction extra_stack_tx(final_tx);
    extra_stack_tx.vin[0].scriptWitness.stack.push_back(std::vector<unsigned char>{0x01});
    const CTransaction extra_stack_final_tx(extra_stack_tx);

    PrecomputedTransactionData extra_stack_txdata;
    std::vector<CTxOut> extra_stack_spent_outputs;
    extra_stack_spent_outputs.emplace_back(amount, script_pub_key);
    extra_stack_txdata.Init(extra_stack_final_tx, std::move(extra_stack_spent_outputs));

    ScriptError extra_stack_err = SCRIPT_ERR_UNKNOWN_ERROR;
    TransactionSignatureChecker extra_stack_checker(
        &extra_stack_final_tx,
        0,
        amount,
        extra_stack_txdata,
        MissingDataBehavior::ASSERT_FAIL
    );

    BOOST_CHECK(!VerifyScript(
        extra_stack_final_tx.vin[0].scriptSig,
        script_pub_key,
        &extra_stack_final_tx.vin[0].scriptWitness,
        SCRIPT_VERIFY_WITNESS | SCRIPT_VERIFY_PQR,
        extra_stack_checker,
        &extra_stack_err
    ));
    BOOST_CHECK_EQUAL(extra_stack_err, SCRIPT_ERR_WITNESS_PROGRAM_MISMATCH);

    // Signature length must be exactly ML-DSA-65 SIG_BYTES.
    CMutableTransaction short_sig_tx(final_tx);
    short_sig_tx.vin[0].scriptWitness.stack[0].pop_back();
    const CTransaction short_sig_final_tx(short_sig_tx);

    PrecomputedTransactionData short_sig_txdata;
    std::vector<CTxOut> short_sig_spent_outputs;
    short_sig_spent_outputs.emplace_back(amount, script_pub_key);
    short_sig_txdata.Init(short_sig_final_tx, std::move(short_sig_spent_outputs));

    ScriptError short_sig_err = SCRIPT_ERR_UNKNOWN_ERROR;
    TransactionSignatureChecker short_sig_checker(
        &short_sig_final_tx,
        0,
        amount,
        short_sig_txdata,
        MissingDataBehavior::ASSERT_FAIL
    );

    BOOST_CHECK(!VerifyScript(
        short_sig_final_tx.vin[0].scriptSig,
        script_pub_key,
        &short_sig_final_tx.vin[0].scriptWitness,
        SCRIPT_VERIFY_WITNESS | SCRIPT_VERIFY_PQR,
        short_sig_checker,
        &short_sig_err
    ));
    BOOST_CHECK_EQUAL(short_sig_err, SCRIPT_ERR_PQR_SIG_VERIFY);

    // Public key length must be exactly ML-DSA-65 PUBKEY_BYTES.
    CMutableTransaction short_pubkey_tx(final_tx);
    short_pubkey_tx.vin[0].scriptWitness.stack[1].pop_back();
    const CTransaction short_pubkey_final_tx(short_pubkey_tx);

    PrecomputedTransactionData short_pubkey_txdata;
    std::vector<CTxOut> short_pubkey_spent_outputs;
    short_pubkey_spent_outputs.emplace_back(amount, script_pub_key);
    short_pubkey_txdata.Init(short_pubkey_final_tx, std::move(short_pubkey_spent_outputs));

    ScriptError short_pubkey_err = SCRIPT_ERR_UNKNOWN_ERROR;
    TransactionSignatureChecker short_pubkey_checker(
        &short_pubkey_final_tx,
        0,
        amount,
        short_pubkey_txdata,
        MissingDataBehavior::ASSERT_FAIL
    );

    BOOST_CHECK(!VerifyScript(
        short_pubkey_final_tx.vin[0].scriptSig,
        script_pub_key,
        &short_pubkey_final_tx.vin[0].scriptWitness,
        SCRIPT_VERIFY_WITNESS | SCRIPT_VERIFY_PQR,
        short_pubkey_checker,
        &short_pubkey_err
    ));
    BOOST_CHECK_EQUAL(short_pubkey_err, SCRIPT_ERR_PQR_SIG_VERIFY);

    // The PQR signature must commit to the spent output amount.
    PrecomputedTransactionData wrong_amount_txdata;
    std::vector<CTxOut> wrong_amount_spent_outputs;
    wrong_amount_spent_outputs.emplace_back(amount, script_pub_key);
    wrong_amount_txdata.Init(final_tx, std::move(wrong_amount_spent_outputs));

    ScriptError wrong_amount_err = SCRIPT_ERR_UNKNOWN_ERROR;
    TransactionSignatureChecker wrong_amount_checker(
        &final_tx,
        0,
        amount + 1,
        wrong_amount_txdata,
        MissingDataBehavior::ASSERT_FAIL
    );

    BOOST_CHECK(!VerifyScript(
        final_tx.vin[0].scriptSig,
        script_pub_key,
        &final_tx.vin[0].scriptWitness,
        SCRIPT_VERIFY_WITNESS | SCRIPT_VERIFY_PQR,
        wrong_amount_checker,
        &wrong_amount_err
    ));
    BOOST_CHECK_EQUAL(wrong_amount_err, SCRIPT_ERR_PQR_SIG_VERIFY);

    // The public key must match the 32-byte witness program.
    std::vector<unsigned char> wrong_program = program;
    wrong_program[0] ^= 0x01;
    const CScript wrong_script_pub_key = CScript() << OP_2 << wrong_program;

    PrecomputedTransactionData wrong_program_txdata;
    std::vector<CTxOut> wrong_program_spent_outputs;
    wrong_program_spent_outputs.emplace_back(amount, wrong_script_pub_key);
    wrong_program_txdata.Init(final_tx, std::move(wrong_program_spent_outputs));

    ScriptError wrong_program_err = SCRIPT_ERR_UNKNOWN_ERROR;
    TransactionSignatureChecker wrong_program_checker(
        &final_tx,
        0,
        amount,
        wrong_program_txdata,
        MissingDataBehavior::ASSERT_FAIL
    );

    BOOST_CHECK(!VerifyScript(
        final_tx.vin[0].scriptSig,
        wrong_script_pub_key,
        &final_tx.vin[0].scriptWitness,
        SCRIPT_VERIFY_WITNESS | SCRIPT_VERIFY_PQR,
        wrong_program_checker,
        &wrong_program_err
    ));
    BOOST_CHECK_EQUAL(wrong_program_err, SCRIPT_ERR_PQR_SIG_VERIFY);
}


BOOST_AUTO_TEST_CASE(mldsa65_roundtrip)
{
    roundtrip_and_tamper<pq::MLDSA65>();
}

BOOST_AUTO_TEST_CASE(slh_dsa_witness_v3_verify_script)
{
    std::vector<unsigned char> pk(pq::SPHINCS128s::PUBKEY_BYTES);
    std::vector<unsigned char> sk(pq::SPHINCS128s::SECKEY_BYTES);
    std::vector<unsigned char> sig(pq::SPHINCS128s::SIG_BYTES);

    BOOST_REQUIRE(pq::SPHINCS128s::keygen(pk.data(), sk.data()));

    uint256 pubkey_hash;
    CSHA256().Write(pk.data(), pk.size()).Finalize(pubkey_hash.begin());
    const std::vector<unsigned char> program(pubkey_hash.begin(), pubkey_hash.end());

    const CScript script_pub_key = CScript() << OP_3 << program;
    const CAmount amount = 500000;

    CMutableTransaction tx;
    tx.nVersion = 2;
    tx.vin.resize(1);
    tx.vin[0].prevout = COutPoint(uint256S("02"), 0);
    tx.vin[0].nSequence = 0xfffffffe;
    tx.vout.emplace_back(400000, CScript() << OP_TRUE);

    std::vector<CTxOut> spent_outputs;
    spent_outputs.emplace_back(amount, script_pub_key);

    PrecomputedTransactionData txdata;
    txdata.Init(tx, std::move(spent_outputs));
    BOOST_REQUIRE(txdata.m_bip143_segwit_ready);

    const uint256 sighash = SignatureHashPQR(tx, 0, 3, program, amount, txdata);

    size_t siglen = 0;
    BOOST_REQUIRE(pq::SPHINCS128s::sign(
        sig.data(),
        &siglen,
        sighash.begin(),
        32,
        sk.data()
    ));
    BOOST_REQUIRE_EQUAL(siglen, pq::SPHINCS128s::SIG_BYTES);

    tx.vin[0].scriptWitness.stack.clear();
    tx.vin[0].scriptWitness.stack.push_back(sig);
    tx.vin[0].scriptWitness.stack.push_back(pk);

    const CTransaction final_tx(tx);

    PrecomputedTransactionData final_txdata;
    std::vector<CTxOut> final_spent_outputs;
    final_spent_outputs.emplace_back(amount, script_pub_key);
    final_txdata.Init(final_tx, std::move(final_spent_outputs));
    BOOST_REQUIRE(final_txdata.m_bip143_segwit_ready);

    ScriptError err = SCRIPT_ERR_UNKNOWN_ERROR;
    TransactionSignatureChecker checker(
        &final_tx,
        0,
        amount,
        final_txdata,
        MissingDataBehavior::ASSERT_FAIL
    );

    BOOST_CHECK(VerifyScript(
        final_tx.vin[0].scriptSig,
        script_pub_key,
        &final_tx.vin[0].scriptWitness,
        SCRIPT_VERIFY_WITNESS | SCRIPT_VERIFY_PQR,
        checker,
        &err
    ));
    BOOST_CHECK_EQUAL(err, SCRIPT_ERR_OK);

    CMutableTransaction bad_tx(final_tx);
    bad_tx.vin[0].scriptWitness.stack[0][0] ^= 0x01;
    const CTransaction bad_final_tx(bad_tx);

    PrecomputedTransactionData bad_txdata;
    std::vector<CTxOut> bad_spent_outputs;
    bad_spent_outputs.emplace_back(amount, script_pub_key);
    bad_txdata.Init(bad_final_tx, std::move(bad_spent_outputs));

    ScriptError bad_err = SCRIPT_ERR_UNKNOWN_ERROR;
    TransactionSignatureChecker bad_checker(
        &bad_final_tx,
        0,
        amount,
        bad_txdata,
        MissingDataBehavior::ASSERT_FAIL
    );

    BOOST_CHECK(!VerifyScript(
        bad_final_tx.vin[0].scriptSig,
        script_pub_key,
        &bad_final_tx.vin[0].scriptWitness,
        SCRIPT_VERIFY_WITNESS | SCRIPT_VERIFY_PQR,
        bad_checker,
        &bad_err
    ));
    BOOST_CHECK_EQUAL(bad_err, SCRIPT_ERR_PQR_SIG_VERIFY);

    // Before PQR activation, witness v3 keeps future-witness semantics.
    ScriptError preactivation_err = SCRIPT_ERR_UNKNOWN_ERROR;
    TransactionSignatureChecker preactivation_checker(
        &bad_final_tx,
        0,
        amount,
        bad_txdata,
        MissingDataBehavior::ASSERT_FAIL
    );

    BOOST_CHECK(VerifyScript(
        bad_final_tx.vin[0].scriptSig,
        script_pub_key,
        &bad_final_tx.vin[0].scriptWitness,
        SCRIPT_VERIFY_WITNESS,
        preactivation_checker,
        &preactivation_err
    ));
    BOOST_CHECK_EQUAL(preactivation_err, SCRIPT_ERR_OK);

    // A modified public key must fail the witness-program binding.
    CMutableTransaction bad_pubkey_tx(final_tx);
    bad_pubkey_tx.vin[0].scriptWitness.stack[1][0] ^= 0x01;
    const CTransaction bad_pubkey_final_tx(bad_pubkey_tx);

    PrecomputedTransactionData bad_pubkey_txdata;
    std::vector<CTxOut> bad_pubkey_spent_outputs;
    bad_pubkey_spent_outputs.emplace_back(amount, script_pub_key);
    bad_pubkey_txdata.Init(bad_pubkey_final_tx, std::move(bad_pubkey_spent_outputs));

    ScriptError bad_pubkey_err = SCRIPT_ERR_UNKNOWN_ERROR;
    TransactionSignatureChecker bad_pubkey_checker(
        &bad_pubkey_final_tx,
        0,
        amount,
        bad_pubkey_txdata,
        MissingDataBehavior::ASSERT_FAIL
    );

    BOOST_CHECK(!VerifyScript(
        bad_pubkey_final_tx.vin[0].scriptSig,
        script_pub_key,
        &bad_pubkey_final_tx.vin[0].scriptWitness,
        SCRIPT_VERIFY_WITNESS | SCRIPT_VERIFY_PQR,
        bad_pubkey_checker,
        &bad_pubkey_err
    ));
    BOOST_CHECK_EQUAL(bad_pubkey_err, SCRIPT_ERR_PQR_SIG_VERIFY);

    // PQR witness must contain exactly [signature, public key].
    CMutableTransaction extra_stack_tx(final_tx);
    extra_stack_tx.vin[0].scriptWitness.stack.push_back(std::vector<unsigned char>{0x01});
    const CTransaction extra_stack_final_tx(extra_stack_tx);

    PrecomputedTransactionData extra_stack_txdata;
    std::vector<CTxOut> extra_stack_spent_outputs;
    extra_stack_spent_outputs.emplace_back(amount, script_pub_key);
    extra_stack_txdata.Init(extra_stack_final_tx, std::move(extra_stack_spent_outputs));

    ScriptError extra_stack_err = SCRIPT_ERR_UNKNOWN_ERROR;
    TransactionSignatureChecker extra_stack_checker(
        &extra_stack_final_tx,
        0,
        amount,
        extra_stack_txdata,
        MissingDataBehavior::ASSERT_FAIL
    );

    BOOST_CHECK(!VerifyScript(
        extra_stack_final_tx.vin[0].scriptSig,
        script_pub_key,
        &extra_stack_final_tx.vin[0].scriptWitness,
        SCRIPT_VERIFY_WITNESS | SCRIPT_VERIFY_PQR,
        extra_stack_checker,
        &extra_stack_err
    ));
    BOOST_CHECK_EQUAL(extra_stack_err, SCRIPT_ERR_WITNESS_PROGRAM_MISMATCH);

    // Signature length must be exactly SLH-DSA/SPHINCS+-128s SIG_BYTES.
    CMutableTransaction short_sig_tx(final_tx);
    short_sig_tx.vin[0].scriptWitness.stack[0].pop_back();
    const CTransaction short_sig_final_tx(short_sig_tx);

    PrecomputedTransactionData short_sig_txdata;
    std::vector<CTxOut> short_sig_spent_outputs;
    short_sig_spent_outputs.emplace_back(amount, script_pub_key);
    short_sig_txdata.Init(short_sig_final_tx, std::move(short_sig_spent_outputs));

    ScriptError short_sig_err = SCRIPT_ERR_UNKNOWN_ERROR;
    TransactionSignatureChecker short_sig_checker(
        &short_sig_final_tx,
        0,
        amount,
        short_sig_txdata,
        MissingDataBehavior::ASSERT_FAIL
    );

    BOOST_CHECK(!VerifyScript(
        short_sig_final_tx.vin[0].scriptSig,
        script_pub_key,
        &short_sig_final_tx.vin[0].scriptWitness,
        SCRIPT_VERIFY_WITNESS | SCRIPT_VERIFY_PQR,
        short_sig_checker,
        &short_sig_err
    ));
    BOOST_CHECK_EQUAL(short_sig_err, SCRIPT_ERR_PQR_SIG_VERIFY);

    // Public key length must be exactly SLH-DSA/SPHINCS+-128s PUBKEY_BYTES.
    CMutableTransaction short_pubkey_tx(final_tx);
    short_pubkey_tx.vin[0].scriptWitness.stack[1].pop_back();
    const CTransaction short_pubkey_final_tx(short_pubkey_tx);

    PrecomputedTransactionData short_pubkey_txdata;
    std::vector<CTxOut> short_pubkey_spent_outputs;
    short_pubkey_spent_outputs.emplace_back(amount, script_pub_key);
    short_pubkey_txdata.Init(short_pubkey_final_tx, std::move(short_pubkey_spent_outputs));

    ScriptError short_pubkey_err = SCRIPT_ERR_UNKNOWN_ERROR;
    TransactionSignatureChecker short_pubkey_checker(
        &short_pubkey_final_tx,
        0,
        amount,
        short_pubkey_txdata,
        MissingDataBehavior::ASSERT_FAIL
    );

    BOOST_CHECK(!VerifyScript(
        short_pubkey_final_tx.vin[0].scriptSig,
        script_pub_key,
        &short_pubkey_final_tx.vin[0].scriptWitness,
        SCRIPT_VERIFY_WITNESS | SCRIPT_VERIFY_PQR,
        short_pubkey_checker,
        &short_pubkey_err
    ));
    BOOST_CHECK_EQUAL(short_pubkey_err, SCRIPT_ERR_PQR_SIG_VERIFY);

    // The PQR signature must commit to the spent output amount.
    PrecomputedTransactionData wrong_amount_txdata;
    std::vector<CTxOut> wrong_amount_spent_outputs;
    wrong_amount_spent_outputs.emplace_back(amount, script_pub_key);
    wrong_amount_txdata.Init(final_tx, std::move(wrong_amount_spent_outputs));

    ScriptError wrong_amount_err = SCRIPT_ERR_UNKNOWN_ERROR;
    TransactionSignatureChecker wrong_amount_checker(
        &final_tx,
        0,
        amount + 1,
        wrong_amount_txdata,
        MissingDataBehavior::ASSERT_FAIL
    );

    BOOST_CHECK(!VerifyScript(
        final_tx.vin[0].scriptSig,
        script_pub_key,
        &final_tx.vin[0].scriptWitness,
        SCRIPT_VERIFY_WITNESS | SCRIPT_VERIFY_PQR,
        wrong_amount_checker,
        &wrong_amount_err
    ));
    BOOST_CHECK_EQUAL(wrong_amount_err, SCRIPT_ERR_PQR_SIG_VERIFY);

    // The public key must match the 32-byte witness program.
    std::vector<unsigned char> wrong_program = program;
    wrong_program[0] ^= 0x01;
    const CScript wrong_script_pub_key = CScript() << OP_3 << wrong_program;

    PrecomputedTransactionData wrong_program_txdata;
    std::vector<CTxOut> wrong_program_spent_outputs;
    wrong_program_spent_outputs.emplace_back(amount, wrong_script_pub_key);
    wrong_program_txdata.Init(final_tx, std::move(wrong_program_spent_outputs));

    ScriptError wrong_program_err = SCRIPT_ERR_UNKNOWN_ERROR;
    TransactionSignatureChecker wrong_program_checker(
        &final_tx,
        0,
        amount,
        wrong_program_txdata,
        MissingDataBehavior::ASSERT_FAIL
    );

    BOOST_CHECK(!VerifyScript(
        final_tx.vin[0].scriptSig,
        wrong_script_pub_key,
        &final_tx.vin[0].scriptWitness,
        SCRIPT_VERIFY_WITNESS | SCRIPT_VERIFY_PQR,
        wrong_program_checker,
        &wrong_program_err
    ));
    BOOST_CHECK_EQUAL(wrong_program_err, SCRIPT_ERR_PQR_SIG_VERIFY);
}


BOOST_AUTO_TEST_CASE(slh_dsa_sphincs128s_roundtrip)
{
    roundtrip_and_tamper<pq::SPHINCS128s>();
}

BOOST_AUTO_TEST_SUITE_END()
