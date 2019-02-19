#pragma once
//
// Author: Vladimir Petter
// github: https://github.com/vladp72/iffl
//
// Common definitions and utility functions.
//
#include <type_traits>
#include <memory>
#include <memory_resource>
#include <vector>
#include <tuple>
#include <algorithm>
#include <utility>
#include <atomic>

#include <cerrno>

#include <typeinfo>

#include <intrin.h>

//
// Assertions that stop program if it is at a risk of going to 
// undefined behavior.
//
#ifndef FFL_FAST_FAIL
#define FFL_FAST_FAIL(EC) {__debugbreak();__fastfail(EC);}
#endif

#ifndef FFL_CRASH_APPLICATION
#define FFL_CRASH_APPLICATION() FFL_FAST_FAIL(ENOTRECOVERABLE)
#endif

#ifndef FFL_CODDING_ERROR_IF
#define FFL_CODDING_ERROR_IF(C) if (C) {FFL_FAST_FAIL(ENOTRECOVERABLE);} else {;}
#endif

#ifndef FFL_CODDING_ERROR_IF_NOT
#define FFL_CODDING_ERROR_IF_NOT(C) if (C) {;} else {FFL_FAST_FAIL(ENOTRECOVERABLE);}
#endif

//
// Field offset in the structure
//
#define FFL_FIELD_OFFSET(T, F) (((char const *)(&((T *)NULL)->F)) - (char const *)NULL)
//
// Field offset in the structure plus field size. 
// Offset to the start of padding [if any] for the next field [if any].
//
#define FFL_SIZE_THROUGH_FIELD(T, F) (FFL_FIELD_OFFSET(T, F) + sizeof(((T *)NULL)->F))
//
// Offset to the start of padding [if any] for the next field [if any].
//
#define FFL_PADDING_OFFSET_AFTER_FIELD(T, F) FFL_SIZE_THROUGH_FIELD(T, F)
//
// Size of padding [if any] between two fields.
// Does not validate that F2 is following after F1 in the structure.
//
#define FFL_PADDING_BETWEEN_FIELDS_UNSAFE(T, F1, F2)(FFL_FIELD_OFFSET(T, F2) - FFL_SIZE_THROUGH_FIELD(T, F1))
//
// Size of padding [if any] between two fields.
// Validate that F2 is following after F1 in the structure.
//
#define FFL_PADDING_BETWEEN_FIELDS(T, F1, F2)([] {static_assert(FFL_FIELD_OFFSET(T, F1) <= FFL_FIELD_OFFSET(T, F2), "F1 must have lower offset in structure than F2"); }, FFL_PADDING_BETWEEN_FIELDS_UNSAFE(T, F1, F2))
//
// Calculates pointer to the beginnig of the structure 
// from the pointer to a field in that structure.
//
#define FFL_FIELD_PTR_TO_OBJ_PTR(T, F, P) ((T *)((char const *)(P) - FFL_FIELD_OFFSET(T, F)))

//
// intrusive flat forward list
//
namespace iffl {

//
// If min or max are defined as a macro then 
// save current definition, and undefine it
// at the end of the header we will restore 
// saved definition
//
#ifdef min 
#define FFL_SAVED_MIN_DEFINITION min
#undef min
#endif

#ifdef max 
#define FFL_SAVED_MAX_DEFINITION max
#undef max
#endif

inline static size_t const iffl_npos = std::numeric_limits<size_t>::max();

//
// restore saved min and max definition
//
#ifdef FFL_SAVED_MAX_DEFINITION
#define max FFL_SAVED_MAX_DEFINITION
#undef FFL_SAVED_MAX_DEFINITION
#endif

#ifdef FFL_SAVED_MIN_DEFINITION
#define min FFL_SAVED_MIN_DEFINITION
#undef FFL_SAVED_MIN_DEFINITION
#endif

constexpr inline size_t ptr_to_size(void const *const ptr) {
    static_assert(sizeof(void*) == sizeof(size_t), 
                  "This function assumes that on this platform size of pointer equals to a size of size_t type");
    size_t const *const size_ptr = reinterpret_cast<size_t const *const>(&ptr);
    return *size_ptr;
}

constexpr inline void * const size_to_ptr(size_t size) {
    static_assert(sizeof(void*) == sizeof(size_t),
                  "This function assumes that on this platform size of pointer equals to a size of size_t type");
    void ** ptr_ptr = reinterpret_cast<void **>(&size);
    return *ptr_ptr;
}

template<typename T>
constexpr inline size_t roundup_size_to_alignment(size_t size) noexcept {
    static_assert(alignof(T) > 0, "Cannot devide by 0");
    return ((size + alignof(T)-1) / alignof(T)) *alignof(T);
}

template<typename T>
constexpr inline char * roundup_ptr_to_alignment(char *ptr) noexcept {
    static_assert(alignof(T) > 0, "Cannot devide by 0");
    return ((ptr + alignof(T)-1) / alignof(T)) *alignof(T);
}

template<typename T>
constexpr inline char const * roundup_ptr_to_alignment(char const *ptr) noexcept {
    static_assert(alignof(T) > 0, "Cannot devide by 0");
    return ((ptr + alignof(T)-1) / alignof(T)) *alignof(T);
}

inline void copy_data(char *to_buffer, char const *from_buffer, size_t length) noexcept {
    memcpy(to_buffer, from_buffer, length);
}

inline void move_data(char *to_buffer, char const *from_buffer, size_t length) noexcept {
    memmove(to_buffer, from_buffer, length);
}

inline void fill_buffer(char *buffer, int value, size_t length) noexcept {
    memset(buffer, value, length);
}

inline void zero_buffer(char *buffer, size_t length) noexcept {
    fill_buffer(buffer, 0, length);
}

//
// flat_forward_list uses this
// helper to deallocate memory
// on failures
//
template <typename G>
class scope_guard :public G {

public:

    constexpr scope_guard(G g)
        : G{ g } {
    }

    constexpr scope_guard(scope_guard &&) = default;
    constexpr scope_guard &operator=(scope_guard &&) = default;

    constexpr scope_guard(scope_guard &) = delete;
    constexpr scope_guard &operator=(scope_guard &) = delete;

    ~scope_guard() noexcept {
        discharge();
    }

    void discharge() noexcept {
        if (armed_) {
            this->G::operator()();
            armed_ = false;
        }
    }

    constexpr void disarm() noexcept {
        armed_ = false;
    }

    constexpr void arm() noexcept {
        armed_ = true;
    }

    constexpr bool is_armed() const noexcept {
        return armed_;
    }

    explicit operator bool() const noexcept {
        return is_armed();
    }

private:
    bool armed_{ true };
};

//
// Creates an instance of guard given
// input functor. Helper for lambdas where 
// we do not know name of lambda type
//
template <typename G>
inline constexpr auto make_scope_guard(G &&g) {
    return scope_guard<G>{std::forward<G>(g)};
}

//
// Helper class used as a parameter in 
// flat_forward_list constructor to designate
// that it should take ownership of the
// buffer rather than make a copy.
//
struct attach_buffer {};

//
// Helper class used in flat_forward_list::detach
// designating that output should contain address 
// of last element and end of buffer rahter than 
// theirs offsets
//
struct as_pointers {};

struct range {
    //
    // Starting offset of element in a buffer
    // Element always starts at the beginning of the 
    // buffer
    //
    size_t buffer_begin{ 0 };
    //
    // starting element + element size
    //
    size_t data_end{ 0 };
    //
    // end of the buffer used by this element 
    // without padding.
    // Where next element is starting.
    //
    size_t buffer_end{ 0 };

    constexpr size_t begin() const {
        return buffer_begin;
    }

    constexpr void verify() const noexcept {
        FFL_CODDING_ERROR_IF_NOT(buffer_begin <= data_end &&
                                 data_end <= buffer_end);
    }

    constexpr size_t data_size() const noexcept {
        //
        // Current element size without padding
        //
        return data_end - buffer_begin;
    }

    constexpr size_t buffer_size() const noexcept {
        return buffer_end - buffer_begin;
    }

    constexpr size_t unused_capacity() const noexcept {
        return buffer_end - data_end;
    }

    void zero_unused_capacity_data_ptr(char *data_ptr) const noexcept {
        zero_buffer(data_ptr,
                    unused_capacity());
    }

    void zero_unused_capacity_container_ptr(char *container_ptr) const noexcept {
        zero_unused_capacity_data_ptr(container_ptr + buffer_begin);
    }

    constexpr bool buffer_contains(size_t position) const noexcept {
        return buffer_begin <= position && position < buffer_end;
    }

    constexpr bool data_contains(size_t position) const noexcept {
        return buffer_begin <= position && position < data_end;
    }
};

template<size_t ALIGNMENT_V>
struct range_with_alighment : public range {

    static constexpr size_t const alignment{ ALIGNMENT_V };

    static constexpr size_t roundup_size_to_alignment(size_t size) noexcept {
        if constexpr (ALIGNMENT_V) {
            return ((size + ALIGNMENT_V - 1) / ALIGNMENT_V) *ALIGNMENT_V;
        } else {
            return size;
        }
    }

    constexpr size_t data_end_unaligned() const noexcept {
        return data_end;
    }
    
    constexpr size_t data_end_aligned() const noexcept {
        return roundup_size_to_alignment(data_end);
    }

    constexpr size_t buffer_end_unaligned() const noexcept {
        return buffer_end;
    }

    constexpr size_t buffer_end_aligned() const noexcept {
        return roundup_size_to_alignment(buffer_end);
    }

    constexpr size_t data_size_not_padded() const noexcept {
        return range::data_size();
    }

    constexpr size_t data_size_padded() const noexcept {
        //
        // current element size if we also add padding
        // to keep next element alligned
        //
        return data_end_unaligned() - begin();
    }

    constexpr size_t required_data_padding() const noexcept {
        //
        // Size of required data padding
        //
        return data_end_aligned() - data_end_unaligned();
    }

    constexpr size_t required_buffer_padding() const noexcept {
        //
        // Size of required data padding
        //
        return buffer_end_aligned() - buffer_end_unaligned();
    }

    constexpr size_t buffer_size_not_padded() const noexcept {
        return range::buffer_size();
    }

    constexpr size_t buffer_size_padded() const noexcept {
        return buffer_end_aligned() - begin();
    }
};

template<size_t ALIGNMENT_V>
struct offset_with_aligment {

    size_t offset{ 0 };

    static constexpr size_t roundup_size_to_alignment(size_t offset) noexcept {
        if constexpr (ALIGNMENT_V) {
            return ((offset + ALIGNMENT_V - 1) / ALIGNMENT_V) *ALIGNMENT_V;
        } else {
            return offset;
        }
    }

    constexpr size_t offset_unaligned() const noexcept {
        return offset_aligned - offset_unaligned;
    }

    constexpr size_t offset_aligned() const noexcept {
        return roundup_size_to_alignment(offset);
    }

    constexpr size_t padding_size() const noexcept {
        return offset_aligned() - offset;
    }
};

template<size_t ALIGNMENT_V>
struct size_with_padding {

    size_t size{ 0 };

    static constexpr size_t roundup_size_to_alignment(size_t size) noexcept {
        if constexpr (ALIGNMENT_V) {
            return ((size + ALIGNMENT_V - 1) / ALIGNMENT_V) *ALIGNMENT_V;
        } else {
            return size;
        }
    }

    constexpr size_t size_not_padded() const noexcept {
        return size;
    }

    constexpr size_t size_padded() const noexcept {
        return roundup_size_to_alignment(size);
    }

    constexpr size_t padding_size() const noexcept {
        return size_padded() - size;
    }
};

} // namespace iffl
