#include <crypto/randomx_hash.h>

#include <randomx.h>

bool VextaRandomXHash(
    const void* input,
    std::size_t input_size,
    const void* key,
    std::size_t key_size,
    uint8_t output[VEXTA_RANDOMX_HASH_SIZE])
{
    randomx_cache* cache = randomx_alloc_cache(RANDOMX_FLAG_DEFAULT);
    if (cache == nullptr) {
        return false;
    }

    randomx_init_cache(cache, key, key_size);

    randomx_vm* vm = randomx_create_vm(
        RANDOMX_FLAG_DEFAULT,
        cache,
        nullptr);

    if (vm == nullptr) {
        randomx_release_cache(cache);
        return false;
    }

    randomx_calculate_hash(vm, input, input_size, output);

    randomx_destroy_vm(vm);
    randomx_release_cache(cache);

    return true;
}
