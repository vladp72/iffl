#pragma once

//!
//! @file iffl_common.h
//!
//! @author Vladimir Petter
//!
//! [iffl github](https://github.com/vladp72/iffl)
//!
//! @brief Common definitions, utility functions and vocabulary types.
//!

#include <type_traits>
#include <vector>
#include <tuple>
#include <algorithm>
#include <utility>
#include <atomic>
#include <cerrno>
#include <typeinfo>
#include <memory>
#include <memory_resource>
#include <cstdio>
#include <intrin.h>

//!
//! @brief Fail fast with error code.
//! @param EC - fail fast error code
//!
#ifndef FFL_FAST_FAIL
#define FFL_FAST_FAIL(EC) {__debugbreak();__fastfail(EC);}
#endif
//!
//! @brief Fail fast with error ENOTRECOVERABLE(127 or 0x7f)
//!
#ifndef FFL_CRASH_APPLICATION
#define FFL_CRASH_APPLICATION() FFL_FAST_FAIL(ENOTRECOVERABLE)
#endif
//!
//! @brief If expression is true then fail fast with error ENOTRECOVERABLE(127 or 0x7f)
//! @param C expression that we are asserting
//!
#ifndef FFL_CODDING_ERROR_IF
#define FFL_CODDING_ERROR_IF(C) if (C) {FFL_FAST_FAIL(ENOTRECOVERABLE);} else {;}
#endif
//!
//! @brief If expression is false then fail fast with error ENOTRECOVERABLE(127 or 0x7f)
//! @param C expression that we are asserting
//!
#ifndef FFL_CODDING_ERROR_IF_NOT
#define FFL_CODDING_ERROR_IF_NOT(C) if (C) {;} else {FFL_FAST_FAIL(ENOTRECOVERABLE);}
#endif

//!
//! @brief Field offset in the structure
//! @param T type field is member of
//! @param F field name
//!
#define FFL_FIELD_OFFSET(T, F) (((char const *)(&((T *)NULL)->F)) - (char const *)NULL)
//!
//! @brief Field offset in the structure plus field size. 
//!        Offset to the start of padding [if any] for the next field [if any].
//! @param T type field is member of
//! @param F field name
//!
#define FFL_SIZE_THROUGH_FIELD(T, F) (FFL_FIELD_OFFSET(T, F) + sizeof(((T *)NULL)->F))
//!
//! @brief Offset to the start of padding [if any] for the next field [if any].
//! @param T type field is member of
//! @param F field name
//!
#define FFL_PADDING_OFFSET_AFTER_FIELD(T, F) FFL_SIZE_THROUGH_FIELD(T, F)
//!
//! @brief Size of padding [if any] between two fields.
//!        Does not validate that F2 is following after F1 in the structure.
//! @param T type fields are members of
//! @param F1 firt field name
//! @param F2 second field name
//!
#define FFL_PADDING_BETWEEN_FIELDS_UNSAFE(T, F1, F2)(FFL_FIELD_OFFSET(T, F2) - FFL_SIZE_THROUGH_FIELD(T, F1))
//!
//! @brief Size of padding [if any] between two fields.
//!        Validate that F2 is following after F1 in the structure.
//! @param T type fields are members of
//! @param F1 firt field name
//! @param F2 second field name
//!
#define FFL_PADDING_BETWEEN_FIELDS(T, F1, F2)([] {static_assert(FFL_FIELD_OFFSET(T, F1) <= FFL_FIELD_OFFSET(T, F2), "F1 must have lower offset in structure than F2"); }, FFL_PADDING_BETWEEN_FIELDS_UNSAFE(T, F1, F2))
//!
//! @brief Calculates pointer to the beginnig of the structure 
//!        from the pointer to a field in that structure.
//! @param T type fields are members of
//! @param F field name
//! @param P pointer to the field
//
#define FFL_FIELD_PTR_TO_OBJ_PTR(T, F, P) ((T *)((char const *)(P) - FFL_FIELD_OFFSET(T, F)))

//!
//! @namespace iffl::mpl
//! @brief intrusive flat forward list
//!
namespace iffl {

//!
//! @brief If min or max are defined as a macro then 
//! save current definition, and undefine it
//! at the end of the header we will restore 
//! saved definition
//!
#ifdef min 
#define FFL_SAVED_MIN_DEFINITION min
#undef min
#endif

#ifdef max 
#define FFL_SAVED_MAX_DEFINITION max
#undef max
#endif
//!
//! @brief Constant value that represents invalid position/offset
//! once numeric_limits are anotated as constexpr we should make
//! this variable constexpr
//!
inline static size_t const npos = std::numeric_limits<size_t>::max();

//!
//! @brief Restore saved min and max definition
//!
#ifdef FFL_SAVED_MAX_DEFINITION
#define max FFL_SAVED_MAX_DEFINITION
#undef FFL_SAVED_MAX_DEFINITION
#endif

#ifdef FFL_SAVED_MIN_DEFINITION
#define min FFL_SAVED_MIN_DEFINITION
#undef FFL_SAVED_MIN_DEFINITION
#endif

//!
//! @brief Reinterprets pointer to void value as size_t value
//! @param ptr - pointer that value we want to put in size_t
//! @return - value of pointer places in size_t
//!
constexpr inline size_t ptr_to_size(void const *const ptr) {
    static_assert(sizeof(void*) == sizeof(size_t), 
                  "This function assumes that on this platform size of pointer equals to a size of size_t type");
    size_t const *const size_ptr = reinterpret_cast<size_t const *const>(&ptr);
    return *size_ptr;
}

//!
//! @brief Reinterprets size_t value as pointer to void
//! @param size - value that we want to reinterpret as a pointer
//! @return - pointer that contains same value as passed in size parameter
//!
constexpr inline void * const size_to_ptr(size_t size) {
    static_assert(sizeof(void*) == sizeof(size_t),
                  "This function assumes that on this platform size of pointer equals to a size of size_t type");
    void ** ptr_ptr = reinterpret_cast<void **>(&size);
    return *ptr_ptr;
}
//!
//! @brief Rounds up size to specified alignment
//! @param size - value to round up
//! @param alignment - alignment we are rounding up to
//! @return - value of size rounded up to alignment
//!
constexpr inline size_t roundup_size_to_alignment(size_t size, size_t alignment) noexcept {
    return alignment ? ((size + alignment - 1) / alignment) *alignment : size;
}
//!
//! @brief Rounds up size to specified alignment
//! @tparam T - type that alignment we are aligning to
//! @param size - value to round up
//! @return - value of size rounded up to the type alignment
//!
template<typename T>
constexpr inline size_t roundup_size_to_alignment(size_t size) noexcept {
    static_assert(alignof(T) > 0, "Cannot devide by 0");
    return roundup_size_to_alignment(size, alignof(T));
}
//!
//! @brief Rounds up pointer to specified alignment
//! @param ptr - pointer to round up
//! @param alignment - alignment we are rounding up to
//! @return - value of pointer rounded up to alignment
//!
constexpr inline char * roundup_ptr_to_alignment(char *ptr, size_t alignment) noexcept {
    return reinterpret_cast<char *>(size_to_ptr(roundup_size_to_alignment(ptr_to_size(ptr), alignment)));
}
//!
//! @brief Rounds up pointer to specified alignment
//! @tparam T - type that alignment we are aligning to
//! @param ptr - pointer to round up
//! @return - pointer rounded up to the type alignment
//!
template<typename T>
constexpr inline char * roundup_ptr_to_alignment(char *ptr) noexcept {
    static_assert(alignof(T) > 0, "Cannot devide by 0");
    return roundup_ptr_to_alignment(ptr, alignof(T));
}
//!
//! @brief Rounds up pointer to specified alignment
//! @param ptr - pointer to round up
//! @param alignment - alignment we are rounding up to
//! @return - value of pointer rounded up to alignment
//!
constexpr inline char const * roundup_ptr_to_alignment(char const *ptr, size_t alignment) noexcept {
    return reinterpret_cast<char const *>(size_to_ptr(roundup_size_to_alignment(ptr_to_size(ptr), alignment)));
}
//!
//! @brief Rounds up pointer to specified alignment
//! @tparam T - type that alignment we are aligning to
//! @param ptr - pointer to round up
//! @return - pointer rounded up to the type alignment
//!
template<typename T>
constexpr inline char const * roundup_ptr_to_alignment(char const *ptr) noexcept {
    static_assert(alignof(T) > 0, "Cannot devide by 0");
    return roundup_ptr_to_alignment(ptr, alignof(T));
}
//!
//! @brief copies length bytes from from_buffer to to_buffer.
//!        source and destination buffers cannot overlap
//! @param to_buffer - Destination buffer. Must be at least length bytes long
//! @param from_buffer - Source buffer. Must be at least length bytes long
//! @param length - number of bytes to copy
//!
inline void copy_data(char *to_buffer, char const *from_buffer, size_t length) noexcept {
    memcpy(to_buffer, from_buffer, length);
}
//!
//! @brief copies length bytes from from_buffer to to_buffer.
//!        source and destination buffers can overlap
//! @param to_buffer - Destination buffer. Must be at least length bytes long
//! @param from_buffer - Source buffer. Must be at least length bytes long
//! @param length - number of bytes to copy
//!

inline void move_data(char *to_buffer, char const *from_buffer, size_t length) noexcept {
    memmove(to_buffer, from_buffer, length);
}
//!
//! @brief sets "length" consequative bytes of "to_buffer" to "value".
//! @param buffer - Destination buffer.
//! @param value - value we are assigning
//! @param length - number of bytes to assign value to
//!
inline void fill_buffer(char *buffer, int value, size_t length) noexcept {
    memset(buffer, value, length);
}
//!
//! @brief sets "length" consequative bytes of "to_buffer" to 0.
//! @param buffer - Destination buffer.
//! @param length - number of bytes to assign 0 to
//!
inline void zero_buffer(char *buffer, size_t length) noexcept {
    fill_buffer(buffer, 0, length);
}
//!
//! @class scope_guard 
//! @brief template class that can be parametrised with a functor
//! or a lambda that it will call in distructor. 
//! @details Used to execute 
//! functor as we are leaving scope, unless it was explicitely 
//! disarmed.
//! flat_forward_list uses this helper to deallocate memory
//! on failures.
//! scoped_guald inherits from the functor to minimize tyoe size
//! when functor is emty type. For more information see Empty Base
//! Class Optimization (EBCO).
//!
//! @tparam G type of functor or a lambda
//!
template <typename G>
class scope_guard :private G {

public:
    //!
    //! @brief Constructor from an instance of functor
    //! @param g - guard we are syncing to the guard
    //!
    explicit constexpr scope_guard(G const &g)
        : G{ g } {
    }
    //!
    //! @brief Constructor from an instance of functor
    //! @param g - guard we are syncing to the guard
    //!
    explicit constexpr scope_guard(G &&g)
        : G{ std::move(g) } {
    }
    //!
    //! @brief Guard supports moving between scopes
    //!
    constexpr scope_guard(scope_guard &&) = default;
    //!
    //! @brief Guard supports moving between scopes
    //!
    constexpr scope_guard &operator=(scope_guard &&) = default;
    //!
    //! @brief Guard does not support copy
    //!
    constexpr scope_guard(scope_guard &) = delete;
    //!
    //! @brief Guard does not support copy
    //!
    constexpr scope_guard &operator=(scope_guard &) = delete;
    //!
    //! @brief Distructor attempts to call functor if guard was not disarmed
    //! 
    ~scope_guard() noexcept {
        discharge();
    }
    //!
    //! @brief Use this method to discharge guard now
    //!
    void discharge() noexcept {
        if (armed_) {
            this->G::operator()();
            armed_ = false;
        }
    }
    //!
    //! @brief Disarming guard tells it not discharge during destruction
    //!
    constexpr void disarm() noexcept {
        armed_ = false;
    }
    //!
    //! @brief Can be used to re-arm guard after prior disarm or discharge
    //!
    constexpr void arm() noexcept {
        armed_ = true;
    }
    //!
    //! @brief Tells if guard is armed
    //! @return true if guard is armed and false otherwise
    //!
    constexpr bool is_armed() const noexcept {
        return armed_;
    }
    //!
    //! @brief Implicit conversion to bool
    //! @return true if guard is armed and false otherwise
    //!
    constexpr explicit operator bool() const noexcept {
        return is_armed();
    }

private:
    //!
    //! @brief value is true when guard is armed and false otherwie
    //!
    bool armed_{ true };
};

//!
//! @brief Factory method for scoped_guard
//! @details Creates an instance of guard given
//! input functor. Helper for lambdas where 
//! we do not know name of lambda type
//! @tparam G - type of functor. It is deduced by compiler from parameter type
//! @param g - forwarding reference for the functor. 
//!
template <typename G>
inline constexpr auto make_scope_guard(G &&g) {
    return scope_guard<G>{std::forward<G>(g)};
}

//!
//! @class attach_buffer
//! @brief Helper class used as a parameter in 
//! flat_forward_list constructor to designate
//! that it should take ownership of the
//! buffer rather than make a copy.
//!
struct attach_buffer {};

//!
//! @class as_pointers
//! @details Helper class used in flat_forward_list::detach
//! designating that output should contain address 
//! of last element and end of buffer rahter than 
//! theirs offsets
//!
struct as_pointers {};

//!
//! @class range
//! @brief Vocabulary type that describes a subbuffer in a larger buffer, 
//! and portion of that subbuffer actuallu used by the data.
//! @details Following picture explains that concept
//!
//! @code
//!  buffer_begin       data_end       buffer_end
//!        |                |                |
//!        V                V                V
//! ---------------------------------------------------------------
//! | .... | <element data> | <unused space> | [next element] ... |
//! ---------------------------------------------------------------
//! @endcode
//!
//! unused space between data_end and buffer_end is either thace between 
//! elements in the buffer and/or padding.
//! This vocabulary type is a convinient building block for flat list.
//! When we move elements in the buffer it is more convinient
//! to operate on relative offsets in the buffer.
//!
//!
struct range {
    //!
    //! @brief Starting offset of element in a buffer
    //! Element always starts at the beginning of the 
    //! buffer
    //!
    size_t buffer_begin{ 0 };
    //!
    //! @brief Offset in the buffer where data end
    //!
    size_t data_end{ 0 };
    //!
    //! @brief Offset of the end of the buffer
    //! Next element of the flat list starts at this offset.
    //!
    size_t buffer_end{ 0 };
    //!
    //! @brief offst of the buffer abd data begin
    //! @return current value of buffer_begin
    //!
    constexpr size_t begin() const {
        return buffer_begin;
    }
    //!
    //! @brief Fail fast if range invariants are broken
    //!
    constexpr void verify() const noexcept {
        FFL_CODDING_ERROR_IF_NOT(buffer_begin <= data_end &&
                                 data_end <= buffer_end);
    }
    //!
    //! @brief Data size
    //! @return Returns number of bytes used by the element data
    //!
    constexpr size_t data_size() const noexcept {
        return data_end - buffer_begin;
    }
    //!
    //! @brief Buffer size
    //! @return Returns size of buffer in bytes 
    //!
    constexpr size_t buffer_size() const noexcept {
        return buffer_end - buffer_begin;
    }
    //!
    //! @brief Size of unused buffer
    //! @return Returns size of buffer not ued by the data
    //!
    constexpr size_t unused_capacity() const noexcept {
        return buffer_end - data_end;
    }
    //!
    //! @brief Fills unused part of buffer with fill_byte
    //! @param data_ptr - a pointer to the element start
    //! @param fill_byte - a value to fill bytes with
    //!
    void fill_unused_capacity_data_ptr(char *data_ptr, int fill_byte) const noexcept {
        fill_buffer(data_ptr + data_size(),
                    fill_byte,
                    unused_capacity());
    }
    //!
    //! @brief Zeroes unused part of buffer
    //! @param data_ptr - a pointer to the element start
    //!
    void zero_unused_capacity_data_ptr(char *data_ptr) const noexcept {
        fill_unused_capacity_data_ptr(data_ptr, 0);
    }
    //!
    //! @brief Fills unused part of buffer with fill_byte
    //! @param container_ptr - a pointer to the buffer start
    //! @param fill_byte - a value to fill bytes with
    //!
    void fill_unused_capacity_container_ptr(char *container_ptr, int fill_byte) const noexcept {
        fill_unused_capacity_data_ptr(container_ptr + buffer_begin, fill_byte);
    }
    //!
    //! @brief Zeroes unused part of buffer
    //! @param container_ptr - a pointer to the buffer start
    //!
    void zero_unused_capacity_container_ptr(char *container_ptr) const noexcept {
        zero_unused_capacity_data_ptr(container_ptr + buffer_begin);
    }
    //!
    //! @brief Tells if position falls into the buffer
    //! @param position - position offset in the parent buffer
    //!
    constexpr bool buffer_contains(size_t position) const noexcept {
        return buffer_begin <= position && position < buffer_end;
    }
    //!
    //! @brief Tells if position falls into the data buffer
    //! @param position - position offset in the parent buffer
    //!
    constexpr bool data_contains(size_t position) const noexcept {
        return buffer_begin <= position && position < data_end;
    }
};

//!
//! @class range_with_alighment
//! @brief Vocabulary type that describes a subbuffer in a larger buffer 
//! and portion of that subbuffer actuallu used by the data.
//! @details This type extends range with template parameter that specifies
//! element's type alignment requirements
//! @tparam ALIGNMENT_V - alignment requirements of the element's type
//!
template<size_t ALIGNMENT_V>
struct range_with_alighment : public range {
    //!
    //! Element's type alignment requirements
    //!
    constexpr static size_t const alignment{ ALIGNMENT_V };
    //!
    //! @return Offset of the end of the data padded to
    //! element type alignment
    //!
    constexpr size_t data_end_aligned() const noexcept {
        return roundup_size_to_alignment(data_end, ALIGNMENT_V);
    }
    //!
    //! @return Offset of the end of the buffer padded to
    //! element type alignment
    //!
    constexpr size_t buffer_end_aligned() const noexcept {
        return roundup_size_to_alignment(buffer_end, ALIGNMENT_V);
    }
    //!
    //! @return Data size padded to element type alignment
    //!
    constexpr size_t data_size_padded() const noexcept {
        //
        // current element size if we also add padding
        // to keep next element alligned
        //
        return data_end_aligned() - begin();
    }
    //!
    //! @return Data padding size
    //!
    constexpr size_t required_data_padding() const noexcept {
        //
        // Size of required data padding
        //
        return data_end_aligned() - data_end;
    }
    //!
    //! @return Buffer padding size
    //!
    constexpr size_t required_buffer_padding() const noexcept {
        //
        // Size of required data padding
        //
        return buffer_end_aligned() - buffer_end;
    }
    //!
    //! @return Buffer size padded to element type alignment
    //!
    constexpr size_t buffer_size_padded() const noexcept {
        return buffer_end_aligned() - begin();
    }
};
//!
//! @class offset_with_aligment
//! @brief Vocabulary type that describes an offset in a larger buffer 
//! and template parameter that specifies element's type alignment requirements
//! @tparam ALIGNMENT_V - alignment requirements of the element's type
//!
template<size_t ALIGNMENT_V>
struct offset_with_aligment {
    //!
    //! Element's type alignment requirements
    //!
    constexpr static size_t const alignment{ ALIGNMENT_V };
    //!
    //! @brief Not padded offset in a buffer
    //!
    size_t offset{ 0 };
    //!
    //! @return Returns aligned offset in a buffer
    //!
    constexpr size_t offset_aligned() const noexcept {
        return roundup_size_to_alignment(offset, ALIGNMENT_V);
    }
    //!
    //! @return Returns padding size
    //!
    constexpr size_t padding_size() const noexcept {
        return offset_aligned() - offset;
    }
};
//!
//! @class size_with_padding
//! @brief Vocabulary type that describes an size 
//! and template parameter that specifies element's type alignment requirements
//! @tparam ALIGNMENT_V - alignment requirements of the element's type
//!
template<size_t ALIGNMENT_V>
struct size_with_padding {
    //!
    //! @brief Element's type alignment requirements
    //!
    constexpr static size_t const alignment{ ALIGNMENT_V };
    //!
    //! @brief Not padded size
    //!
    size_t size{ 0 };
    //!
    //! @return Returns size padded to allignment
    //!
    constexpr size_t size_padded() const noexcept {
        return roundup_size_to_alignment(size, ALIGNMENT_V);
    }
    //!
    //! @return Returns size of padding
    //!
    constexpr size_t padding_size() const noexcept {
        return size_padded() - size;
    }
};

// CLASS TEMPLATE _Compressed_pair
struct zero_then_variadic_args_t
{// tag type for value-initializing first,
};// constructing second from remaining args

struct one_then_variadic_args_t
{// tag type for constructing first from one arg,
};// constructing second from remaining args


template<class T1,
         class T2,
         bool = std::is_empty_v<T1> && !std::is_final_v<T1>>
class compressed_pair final : private T1 {// store a pair of values, deriving from empty first

    using base_t = T1;// for visualization

public:
    template<class... Other>
    constexpr explicit compressed_pair(zero_then_variadic_args_t,
                                       Other&&... v2)
        : T1()
        , v2_(std::forward<Other>(v2)...) {// construct from forwarded values
    }

    template<class Other1,
             class... Other2>
    constexpr compressed_pair(one_then_variadic_args_t,
                              Other1&& v1, 
                              Other2&&... v2)
        : T1(std::forward<Other1>(v1))
        , v2_(std::forward<Other2>(v2)...) {// construct from forwarded values
    }

    constexpr T1& get_first() noexcept {// return reference to first
        return (*this);
    }

    constexpr T1 const & get_first() const noexcept {// return const reference to first
        return (*this);
    }

    constexpr T2& get_second() noexcept {// return reference to second
        return (v2_);
    }

    constexpr T2 const & get_second() const noexcept {// return const reference to second
        return (v2_);
    }

private:
    T2 v2_;
};

template<class T1,
         class T2>
class compressed_pair<T1, T2, false> final {// store a pair of values, not deriving from first

public:
    template<class... Other>
    constexpr explicit compressed_pair(zero_then_variadic_args_t,
                                       Other&&... v2)
        : v1_()
        , v2_(std::forward<Other>(v2)...) {// construct from forwarded values
    }

    template<class Other1,
             class... Other2>
    constexpr compressed_pair(one_then_variadic_args_t,
                              Other1&& v1, 
                              Other2&&... v2)
        : v1_(std::forward<Other1>(v1))
        , v2_(std::forward<Other2>(v2)...) {// construct from forwarded values
    }

    constexpr T1& get_first() noexcept {// return reference to first
        return (v1_);
    }

    constexpr T1 const & get_first() const noexcept {// return const reference to first
        return (v1_);
    }

    constexpr T2& get_second() noexcept {// return reference to second
        return (v2_);
    }

    constexpr T2 const & get_second() const noexcept {// return const reference to second
        return (v2_);
    }

private:
    T1 v1_;
    T2 v2_;
};

} // namespace iffl
