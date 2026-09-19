// VEXTA: post-quantum RNG shim. Backs the vendored references' randombytes()
// with Vexta Core's strong CSPRNG (GetStrongRandBytes). Chunked to <=32 bytes
// per call because the internal RNG path expects small requests.

#include <random.h>

#include <cstddef>
#include <cstdint>

extern "C" void randombytes(uint8_t* out, size_t outlen)
{
    while (outlen > 0) {
        const size_t chunk = outlen < 32 ? outlen : 32;
        GetStrongRandBytes(out, static_cast<int>(chunk));
        out += chunk;
        outlen -= chunk;
    }
}
