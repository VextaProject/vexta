#!/usr/bin/env python3
import hashlib, struct, time

COIN = 100000000

pszTimestamp = b"Vexta Signet Genesis Block 09/Jul/2026 - VTX signet network begins"
nTime = 1783555202
nBits = 0x1e0377ae
nVersion = 1
genesisReward = 50 * COIN

def sha256d(b):
    return hashlib.sha256(hashlib.sha256(b).digest()).digest()

def ser_uint256_zero():
    return b"\x00" * 32

def ser_compact_size(n):
    if n < 253:
        return struct.pack("<B", n)
    if n <= 0xffff:
        return b"\xfd" + struct.pack("<H", n)
    if n <= 0xffffffff:
        return b"\xfe" + struct.pack("<I", n)
    return b"\xff" + struct.pack("<Q", n)

def push_data(data):
    return bytes([len(data)]) + data

def script_num(n):
    if n == 0:
        return b""
    result = bytearray()
    neg = n < 0
    absvalue = -n if neg else n
    while absvalue:
        result.append(absvalue & 0xff)
        absvalue >>= 8
    if result[-1] & 0x80:
        result.append(0x80 if neg else 0)
    elif neg:
        result[-1] |= 0x80
    return bytes(result)

def push_num(n):
    b = script_num(n)
    return bytes([len(b)]) + b

scriptSig = push_num(486604799) + push_num(4) + push_data(pszTimestamp)
scriptPubKey = b"\x00\xac"

tx = b""
tx += struct.pack("<i", 1)
tx += ser_compact_size(1)
tx += ser_uint256_zero()
tx += struct.pack("<I", 0xffffffff)
tx += ser_compact_size(len(scriptSig))
tx += scriptSig
tx += struct.pack("<I", 0xffffffff)
tx += ser_compact_size(1)
tx += struct.pack("<q", genesisReward)
tx += ser_compact_size(len(scriptPubKey))
tx += scriptPubKey
tx += struct.pack("<I", 0)

merkle = sha256d(tx)

def bits_to_target(bits):
    exponent = bits >> 24
    mantissa = bits & 0xffffff
    return mantissa * (1 << (8 * (exponent - 3)))

target = bits_to_target(nBits)

print("timestamp:", pszTimestamp.decode())
print("time:", nTime)
print("bits:", hex(nBits))
print("merkle:", merkle[::-1].hex())
print("target:", hex(target))

nonce = 0
start = time.time()

while nonce <= 0xffffffff:
    header = b""
    header += struct.pack("<i", nVersion)
    header += b"\x00" * 32
    header += merkle
    header += struct.pack("<I", nTime)
    header += struct.pack("<I", nBits)
    header += struct.pack("<I", nonce)

    h = sha256d(header)
    value = int.from_bytes(h[::-1], "big")

    if value <= target:
        print()
        print("FOUND")
        print("nTime =", nTime)
        print("nNonce =", nonce)
        print("nBits =", hex(nBits))
        print("hashGenesisBlock =", h[::-1].hex())
        print("hashMerkleRoot   =", merkle[::-1].hex())
        print("seconds =", round(time.time() - start, 2))
        break

    if nonce % 1000000 == 0 and nonce > 0:
        print("nonce", nonce, "elapsed", round(time.time() - start, 1))

    nonce += 1
else:
    print("not found")
