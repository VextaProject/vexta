#ifndef VEXTA_CRYPTO_RANDOMX_HASH_H
#define VEXTA_CRYPTO_RANDOMX_HASH_H

#include <cstddef>
#include <cstdint>
#include <memory>

static constexpr std::size_t VEXTA_RANDOMX_HASH_SIZE = 32;

class VextaRandomXHasher
{
public:
    VextaRandomXHasher(const void* key, std::size_t key_size);
    ~VextaRandomXHasher();

    VextaRandomXHasher(const VextaRandomXHasher&) = delete;
    VextaRandomXHasher& operator=(const VextaRandomXHasher&) = delete;

    bool IsValid() const;

    bool Hash(
        const void* input,
        std::size_t input_size,
        uint8_t output[VEXTA_RANDOMX_HASH_SIZE]);

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

/**
 * Calculate a RandomX hash using an explicit key.
 *
 * This wrapper intentionally uses RANDOMX_FLAG_DEFAULT so consensus hashing
 * does not depend on CPU-specific runtime feature detection.
 *
 * Returns false if RandomX cache or VM allocation fails.
 */
bool VextaRandomXHash(
    const void* input,
    std::size_t input_size,
    const void* key,
    std::size_t key_size,
    uint8_t output[VEXTA_RANDOMX_HASH_SIZE]);

#endif // VEXTA_CRYPTO_RANDOMX_HASH_H
