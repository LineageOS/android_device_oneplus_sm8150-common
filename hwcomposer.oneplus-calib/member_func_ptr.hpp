#include <type_traits>
#include <cstddef>

struct member_func_ptr_abi {
    void *ptr;
    ptrdiff_t adj;

    inline void set_ptr(void *value) {
#if defined(__arm__) || defined(__aarch64__)
        ptr = value;
#else // elif defined(__i386__) || defined(__x86_64__)
        ptr = __builtin_assume_aligned(value, 2);
#endif
        adj = 0;
    }

    constexpr std::optional<ptrdiff_t> vtbl_offset() {
#if defined(__arm__) || defined(__aarch64__)
        return (adj & 1) ? (ptrdiff_t)ptr : std::optional<ptrdiff_t>{};
#else // elif defined(__i386__) || defined(__x86_64__)
        return ((uintptr_t)ptr & 1) ? ((ptrdiff_t)ptr) - 1 : std::optional<ptrdiff_t>{};
#endif
    }
};

template <typename TPtr, typename TFunc>
static inline typename std::enable_if_t<std::is_member_function_pointer_v<TFunc> && std::is_pointer_v<TPtr>, TPtr> member_func_ptr_cast(TFunc func) {
    union {
        member_func_ptr_abi abi;
        TFunc func;
    } caster;
    static_assert(sizeof(caster.func) == sizeof(caster.abi));
    caster.func = func;
    return caster.abi.ptr;
}

template <typename TPtr, typename TFunc>
static inline typename std::enable_if_t<std::is_same_v<TPtr, member_func_ptr_abi>, TPtr> member_func_ptr_cast(TFunc func) {
    union {
        member_func_ptr_abi abi;
        TFunc func;
    } caster;
    static_assert(sizeof(caster.func) == sizeof(caster.abi));
    caster.func = func;
    return caster.abi;
}

template <typename TFunc, typename TPtr>
static inline typename std::enable_if_t<std::is_pointer_v<TPtr> && std::is_member_function_pointer_v<TFunc>, TFunc> member_func_ptr_cast(TPtr ptr) {
    union {
        member_func_ptr_abi abi;
        TFunc func;
    } caster;
    static_assert(sizeof(caster.func) == sizeof(caster.abi));
    caster.abi.set_ptr(reinterpret_cast<void*>(ptr));
    return caster.func;
}
