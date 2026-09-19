#include <crypto/randomx_hash.h>

#include <randomx.h>

struct VextaRandomXHasher::Impl
{
    randomx_cache* cache{nullptr};
    randomx_vm* vm{nullptr};

    Impl(const void* key, std::size_t key_size)
    {
        cache = randomx_alloc_cache(RANDOMX_FLAG_DEFAULT);
        if (cache == nullptr) {
            return;
        }

        randomx_init_cache(cache, key, key_size);

        vm = randomx_create_vm(
            RANDOMX_FLAG_DEFAULT,
            cache,
            nullptr);

        if (vm == nullptr) {
            randomx_release_cache(cache);
            cache = nullptr;
        }
    }

    ~Impl()
    {
        if (vm != nullptr) {
            randomx_destroy_vm(vm);
        }

        if (cache != nullptr) {
            randomx_release_cache(cache);
        }
    }
};

VextaRandomXHasher::VextaRandomXHasher(
    const void* key,
    std::size_t key_size)
    : m_impl(std::make_unique<Impl>(key, key_size))
{
}

VextaRandomXHasher::~VextaRandomXHasher() = default;

bool VextaRandomXHasher::IsValid() const
{
    return m_impl != nullptr &&
           m_impl->cache != nullptr &&
           m_impl->vm != nullptr;
}

bool VextaRandomXHasher::Hash(
    const void* input,
    std::size_t input_size,
    uint8_t output[VEXTA_RANDOMX_HASH_SIZE])
{
    if (!IsValid()) {
        return false;
    }

    randomx_calculate_hash(
        m_impl->vm,
        input,
        input_size,
        output);

    return true;
}

bool VextaRandomXHash(
    const void* input,
    std::size_t input_size,
    const void* key,
    std::size_t key_size,
    uint8_t output[VEXTA_RANDOMX_HASH_SIZE])
{
    VextaRandomXHasher hasher(key, key_size);

    if (!hasher.IsValid()) {
        return false;
    }

    return hasher.Hash(input, input_size, output);
}
