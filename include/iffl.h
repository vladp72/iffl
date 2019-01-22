//
// Author: Vladimir Petter
//
// Implements intrusive flat forward list.
//
// This container is designed to contain element with 
// following general structure: 
//
//                      ------------------------------------------------------------
//                      |                                                          |
//                      |                                                          V
// | <fields> | offset to next element | <offsets of data> | [data] | [padding] || [next element] ...
// |                        header                         | [data] | [padding] || [next element] ...
//
// Examples:
//
// FILE_FULL_EA_INFORMATION 
//   https://docs.microsoft.com/en-us/windows-hardware/drivers/ddi/content/wdm/ns-wdm-_file_full_ea_information
//
// FILE_NOTIFY_EXTENDED_INFORMATION 
//   https://docs.microsoft.com/en-us/windows/desktop/api/winnt/ns-winnt-_file_notify_extended_information
//
// WIN32_FIND_DATA 
//   https://docs.microsoft.com/en-us/windows/desktop/api/minwinbase/ns-minwinbase-_win32_find_dataa
//
// FILE_INFO_BY_HANDLE_CLASS 
//   https://msdn.microsoft.com/en-us/8f02e824-ca41-48c1-a5e8-5b12d81886b5
//   output for the following information classes
//        FileIdBothDirectoryInfo
//        FileIdBothDirectoryRestartInfo
//        FileIoPriorityHintInfo
//        FileRemoteProtocolInfo
//        FileFullDirectoryInfo
//        FileFullDirectoryRestartInfo
//        FileStorageInfo
//        FileAlignmentInfo
//        FileIdInfo
//        FileIdExtdDirectoryInfo
//        FileIdExtdDirectoryRestartInfo
//
// Output of NtQueryDirectoryFile 
//   https://docs.microsoft.com/en-us/windows-hardware/drivers/ddi/content/ntifs/ns-ntifs-_file_both_dir_information
//
// Cluster property list CLUSPROP_VALUE
//   https://docs.microsoft.com/en-us/windows/desktop/api/clusapi/ns-clusapi-clusprop_value
// 
// This module implements 
//
//   function flat_forward_list_validate that 
//   can be use to deal with untrusted buffers.
//   You can use it to validate if untrusted buffer 
//   contains a valid list, and to find boundary at 
//   which list gets invalid.
//   It also can be used to enumirate over validated
//   elements 
//
//   flat_forward_list_iterator and flat_forward_list_const_iterator 
//   that can be used to enmirate over previously validated buffer
//
//   flat_forward_list a container that provides a set of helper 
//   algorithms and manages y while list changes.
//
//   pmr_flat_forward_list is a an aliase of flat_forward_list
//   where allocatoe is polimorfic_allocator.
//
//   debug_memory_resource a memory resource that can be used along
//   with polimorfic allocator for debugging contained.
//
// Interface tht user is responsible for:
//
//  Since we are implementing intrusive container, user have to give us a 
//  helper calss thar implement folowing methods 
//   - tell us minimum required size element have to have ti be able to query element size
//          constexpr static size_t minimum_size() noexcept
//   - query offset to next element 
//          constexpr static size_t get_next_element_offset(char const *buffer) noexcept
//   - update offset to the next element
//          constexpr static void set_next_element_offset(char *buffer, size_t size) noexcept
//   - calculate element size from data
//          constexpr static size_t calculate_next_element_offset(char const *buffer) noexcept
//   - validate that data fit into the buffer
//          constexpr static bool validate(size_t buffer_size, char const *buffer) noexcept
//
//  By default we are looking for a partial specialization for the element type.
//  For example:
//
//    namespace iffl {
//        template <>
//        struct flat_forward_list_traits<FLAT_FORWARD_LIST_TEST> {
//            constexpr static size_t minimum_size() noexcept { <implementation> }
//            constexpr static size_t get_next_element_offset(char const *buffer) noexcept { <implementation> }
//            constexpr static void set_next_element_offset(char *buffer, size_t size) noexcept { <implementation> }
//            constexpr static size_t calculate_next_element_offset(char const *buffer) noexcept { <implementation> }
//            constexpr static bool validate(size_t buffer_size, char const *buffer) noexcept {<implementation>}
//        };
//    }
//
//
//   for sample implementation see flat_forward_list_traits<FLAT_FORWARD_LIST_TEST> @ test\iffl_test_cases.cpp
//   and addition documetation in this mode right above where primary
//   template for flat_forward_list_traits is defined
//
//   If picking traits using partial specialization is not preferable then traits can be passed as
//   an explicit template parameter. For example:
// 
//   using ffl_iterator = iffl::flat_forward_list_iterator<FLAT_FORWARD_LIST_TEST, my_traits>;
//
// Debugging:
//
//   if you define FFL_DBG_CHECK_DATA_VALID then every method
//   that modifies data in container will call flat_forward_list_validate
//   at the end and makes sure that data are valid, and last element is
//   where container believes it should be.
//   Cost O(n).
//
//   If you define FFL_DBG_CHECK_ITERATOR_VALID then every input 
//   iterator will be cheked to match to valid element in the container.
//   Cost O(n)
//
//   You can use debug_memory_resource and pmr_flat_forward_list to validate
//   that all allocations are freed and that there are no buffer overruns/underruns
//

#include <type_traits>
#include <memory>
#include <memory_resource>
#include <vector>
#include <tuple>
#include <algorithm>
#include <utility>
#include <atomic>

#include <intrin.h>

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

//
// Assertions
//
#ifndef FFL_CODDING_ERROR_IF
#define FFL_CODDING_ERROR_IF(C) if (C) {__debugbreak();} else {;}
#endif

#ifndef FFL_CODDING_ERROR_IF_NOT
#define FFL_CODDING_ERROR_IF_NOT(C) if (C) {;} else {__debugbreak();}
#endif

#ifndef FFL_CRASH_APPLICATION
#define FFL_CRASH_APPLICATION() {__debugbreak();}
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

template<typename T>
constexpr inline size_t roundup_size_to_alignment(size_t size) noexcept {
    static_assert(alignof(T) > 0, "make sure algorithm does not break");
    return ((size + alignof(T)-1) / alignof(T)) *alignof(T);
}

template<typename T>
constexpr inline char * roundup_ptr_to_alignment(char *ptr) noexcept {
    static_assert(alignof(T) > 0, "make sure algorithm does not break");
    return ((ptr + alignof(T)-1) / alignof(T)) *alignof(T);
}

template<typename T>
constexpr inline char const * roundup_ptr_to_alignment(char const *ptr) noexcept {
    static_assert(alignof(T) > 0, "make sure algorithm does not break");
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
// For example see flat_forward_list_traits<FLAT_FORWARD_LIST_TEST> @ test\iffl_test_cases.cpp
// 
// Specialize this class for the tipe that is an element header for your
// flat forward list
//
// Example:
// See flat_forward_list_traits<FLAT_FORWARD_LIST_TEST> @ test\iffl_tests.cpp
//
// This is the only method required by flat_forward_list_iterator.
// Returns offset to the next element or 0 if this is the last element.
//
// constexpr static size_t get_next_element_offset(char const *buffer) noexcept;
//
// This method is requiered for flat_forward_list_validate algorithm
// Minimum number of bytes to be able to safely query offset to next 
// and safely examine other fields.
//
// constexpr static size_t minimum_size() noexcept;
//
// This method is required for flat_forward_list_validate algorithm
// Validates that variable data fits in the buffer
//
// constexpr static bool validate(size_t buffer_size, char const *buffer) noexcept;
//
// This method is used by flat_forward_list container to update offset to the
// next element. size can be 0 if this element is last or above zero for any other element. 
//
// constexpr static void set_next_element_offset(char *buffer, size_t size) noexcept
//
// This method is used by flat_forward_list. It calculates size of element, but it should
// not use next element offset, and instead it should calculate size based on the data this 
// element contains. It is used when we append new element to the container, and need to
// update offset to the next on the element that used to be last. In that case offset to 
// the next is determined by calling this method. 
// Another example that uses this method is element_shrink_to_fit.
//
// constexpr static size_t calculate_next_element_offset(char const *buffer) noexcept 
//
template <typename T>
struct flat_forward_list_traits;

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
// Default element validation functor.
// Calls flat_forward_list_traits<T>::validate(...)
//
template<typename T,
         typename TT = flat_forward_list_traits<T>>
struct default_validate_element_fn {
    bool operator() (size_t buffer_size, char const *buffer) const noexcept {
        return TT::validate(buffer_size, buffer);
    }
};

//
// Use this functor if you want to noop element validation
//
struct noop_validate_element_fn {
    bool operator() (size_t, char const *) const noexcept {
        return true;
    }
};

//
// Template parameters:
//      T  - type of element header
//      TT - type trait for T.
//           algorithms expects following methods
//              - get_next_element_offset
//              - minimum_size
//              - validate
//
// Input:
//      first - start of buffer we are validating
//      end - first byte pass the buffer we are validation
//      buffer size == end - first
//      validate_element_fn - a functor that is used to validate
//      element. For instance if element contains offset to variable
//      lengths data, it can check that these data are in bound of 
//      the buffer. Default dunctor calls TT::validate.
//      You can use noop_validate_element_fn if you do not want 
//      validate element
//
// Output:
//      pair
//          first - result of validation
//                - true if buffer is null
//                - true if buffer is empty
//                - true if we found a valid element with offset to the next element equals 0
//                - false is all other cases
//          second - pointer to the last valid element
//                 - null if no valid elements were found
//
// Examples:
//         <true, nullptr>  - buffer is NULL or empty
//                            It is safe to use iterators.
//         <false, nullptr> - buffer is too small to query next element offset,
//                            or offset to next element is pointing beyond end,
//                            or element fields validation did not pass.
//                            We did not find any entries that pass validation
//                            so far.
//                            Buffer contains no flat list.
//         <false, ptr>     - same as above, but we did found at least one valid element.
//                            Buffer contains valid head, but tail is corrupt and have to 
//                            be either truncated by setting next offset on the last valid 
//                            element to 0 or if possible by fixing first elements pass the
//                            last valid element.
//         <true, ptr>      - buffer contains a valid flat forward list, it is safe
//                            to use iterators
//
template<typename T,
         typename TT = flat_forward_list_traits<T>,
         typename F = default_validate_element_fn<T, TT>>
constexpr inline std::pair<bool, char const *> flat_forward_list_validate(char const *first,
                                                                          char const *end, 
                                                                          F const &validate_element_fn = default_validate_element_fn<T, TT>{}) noexcept {
    //
    // by default we did not found any valid elements
    //
    char const *last_valid = nullptr;
    //
    // null buffer is defined as valid
    //
    if (first == nullptr) {
        FFL_CODDING_ERROR_IF_NOT(nullptr == end);
        return std::make_pair(true, last_valid);
    }
    //
    // empty buffer is defined as valid
    //
    if (first == end) {
        return std::make_pair(true, last_valid);
    }
    //
    // Can we safely subtract pointers?
    //
    FFL_CODDING_ERROR_IF(end < first);

    size_t remaining_length = end - first;
    bool result = false;
    for (;;) {
        //
        // is it large enough to even query next offset?
        //
        if (remaining_length < TT::minimum_size()) {
            break;
        }
        //
        // Is offset of the next element in bound of the remaining buffer
        //
        size_t next_element_offset = TT::get_next_element_offset(first);
        if (remaining_length < next_element_offset) {
            break;
        }
        //
        // minimum and next are valid, check rest of the fields
        //
        if (!validate_element_fn(remaining_length, first)) {
            break;
        }
        //
        // This is end of list, we are done,
        // and everything is valid
        //
        if (0 == next_element_offset) {
            last_valid = first;
            result = true;
            break;
        }
        //
        // Advance last valid element forward
        //
        last_valid = first;
        //
        // Advance current element forward
        //
        first += next_element_offset;
        remaining_length -= next_element_offset;
    }
    
    return std::make_pair(result, last_valid);
}

template<typename T,
         typename TT = flat_forward_list_traits<T>,
         typename F = default_validate_element_fn<T, TT>>
constexpr inline std::pair<bool, char *> flat_forward_list_validate(char *first, 
                                                                    char *end,
                                                                    F const &validate_element_fn = default_validate_element_fn<T, TT>{}) noexcept {
    auto[result, last_valid] = flat_forward_list_validate<T, TT>((char const *)first,
                                                                 (char const *)end,
                                                                 validate_element_fn);
    return std::make_pair(result, (char *)last_valid);
}

template<typename T,
         typename TT = flat_forward_list_traits<T>,
         typename F = default_validate_element_fn<T, TT>>
constexpr inline std::pair<bool, T*> flat_forward_list_validate(T *first, 
                                                                T *end,
                                                                F const &validate_element_fn = default_validate_element_fn<T, TT>{}) noexcept {
    auto[result, last_valid] = flat_forward_list_validate<T, TT>((char const *)first,
                                                                 (char const *)end,
                                                                 validate_element_fn);
    return std::make_pair(result, (T *)last_valid);
}

template<typename T,
         typename TT = flat_forward_list_traits<T>,
         typename F = default_validate_element_fn<T, TT>>
constexpr inline std::pair<bool, unsigned char const*> flat_forward_list_validate(unsigned char const *first, 
                                                                                  unsigned char const *end,
                                                                                  F const &validate_element_fn = default_validate_element_fn<T, TT>{}) noexcept {
    auto[result, last_valid] = flat_forward_list_validate<T, TT>((char const *)first,
                                                                 (char const *)end,
                                                                 validate_element_fn);
    return std::make_pair(result, (unsigned char const *)last_valid);
}

template<typename T,
         typename TT = flat_forward_list_traits<T>,
         typename F = default_validate_element_fn<T, TT>>
constexpr inline std::pair<bool, unsigned char*> flat_forward_list_validate(unsigned char *first, 
                                                                            unsigned char *end,
                                                                            F const &validate_element_fn = default_validate_element_fn<T, TT>{}) noexcept {
    auto[result, last_valid] = flat_forward_list_validate<T, TT>(first,
                                                                 end,
                                                                 validate_element_fn);
    return std::make_pair(result, (unsigned char *)last_valid);
}

template<typename T,
         typename TT = flat_forward_list_traits<T>,
         typename F = default_validate_element_fn<T, TT>>
constexpr inline std::pair<bool, void const*> flat_forward_list_validate(void const *first,
                                                                         void const *end,
                                                                         F const &validate_element_fn = default_validate_element_fn<T, TT>{}) noexcept {
    auto[result, last_valid] = flat_forward_list_validate<T, TT>((char const *)first,
                                                                 (char const *)end,
                                                                 validate_element_fn);
    return std::make_pair(result, (void const *)last_valid);
}

template<typename T,
         typename TT = flat_forward_list_traits<T>,
         typename F = default_validate_element_fn<T, TT>>
constexpr inline std::pair<bool, void*> flat_forward_list_validate(void *first,
                                                                   void *end,
                                                                   F const &validate_element_fn = default_validate_element_fn<T, TT>{}) noexcept {
    auto[result, last_valid] = flat_forward_list_validate<T, TT>(first,
                                                                 end,
                                                                 validate_element_fn);
    return std::make_pair(result, (void *)last_valid);
}

template <typename T>
struct container_element_type_base {
    using value_type = T;
    using pointer = T * ;
    using const_pointer = T const *;
    using reference = T & ;
    using const_reference = T const &;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
};

//
// Iterator for flat forward list
// T  - type of element header
// TT - type trait for T that is used to get offset to the
//      next element in the flat list. It must implement
//      get_next_element_offset method.
//
// Once iterator is incremented pass last element it becomes equal
// to default initialized iterator so you can use
// flat_forward_list_iterator_t{} as a sentinel that can be used as an
// end iterator.
//
template<typename T,
         typename TT = flat_forward_list_traits<T>>
    class flat_forward_list_iterator_t : public std::iterator <std::forward_iterator_tag, T>
                                       , public TT {
public:

    using element_type = T;
    using element_traits = TT;
    //
    // Selects between constant and non-constant pointer to the buffer
    // depending if T is const
    //
    using buffer_char_pointer = std::conditional_t<std::is_const_v<T>, char const *, char *>;
    using buffer_unsigned_char_pointer = std::conditional_t<std::is_const_v<T>, unsigned char const *, unsigned char *>;
    using buffer_void_pointer = std::conditional_t<std::is_const_v<T>, void const *, void *>;

    constexpr flat_forward_list_iterator_t() noexcept = default;
    constexpr flat_forward_list_iterator_t(flat_forward_list_iterator_t const &) noexcept = default;
    constexpr flat_forward_list_iterator_t(flat_forward_list_iterator_t && other) noexcept
        : p_{other.p_} {
        other.p_ = nullptr;
    }
    //
    // copy constructor of const iterator from non-const iterator
    //
    template< typename I,
              typename = std::enable_if_t<std::is_const_v<T> &&
                                          std::is_same_v<std::remove_cv_t<I>,
                                                         flat_forward_list_iterator_t< std::remove_cv_t<T>, TT>>>>
    constexpr flat_forward_list_iterator_t(I & other) noexcept
        : p_{ other.get_ptr() } {
    }   
    //
    // move constructor of const iterator from non-const iterator
    //
    template< typename I,
              typename = std::enable_if_t<std::is_const_v<T> &&
                                          std::is_same_v<std::remove_cv_t<I>,
                                                         flat_forward_list_iterator_t< std::remove_cv_t<T>, TT>>>>
    constexpr flat_forward_list_iterator_t(I && other) noexcept
        : p_{ other.get_ptr() } {
        other.reset_ptr(nullptr);
    }

    constexpr explicit flat_forward_list_iterator_t(buffer_char_pointer p) noexcept
        : p_(p) {
    }

    constexpr explicit flat_forward_list_iterator_t(T *p) noexcept
        : p_((buffer_char_pointer)p) {
    }

    constexpr explicit flat_forward_list_iterator_t(buffer_unsigned_char_pointer p) noexcept
        : p_((buffer_char_pointer )p) {
    }
    
    constexpr explicit flat_forward_list_iterator_t(buffer_void_pointer p) noexcept
        : p_((buffer_char_pointer )p) {
    }

    constexpr flat_forward_list_iterator_t &operator= (flat_forward_list_iterator_t const &) noexcept = default;
    
    constexpr flat_forward_list_iterator_t & operator= (flat_forward_list_iterator_t && other) noexcept {
        if (this != &other) {
            p_ = other.p_;
            other.p_ = nullptr;
        }
        return *this;
    }
    //
    // copy assignment operator to const iterator from non-const iterator
    //
    template <typename I,
              typename = std::enable_if_t<std::is_const_v<T> &&
                                          std::is_same_v<std::remove_cv_t<I>,
                                                         flat_forward_list_iterator_t< std::remove_cv_t<T>, TT>>>>
    constexpr flat_forward_list_iterator_t & operator= (I & other) noexcept {
        p_ = other.get_ptr();
        return *this;
    }
    //
    // move assignment operator to const iterator from non-const iterator
    //
    template <typename I,
              typename = std::enable_if_t<std::is_const_v<T> &&
                                          std::is_same_v<std::remove_cv_t<I>,
                                                         flat_forward_list_iterator_t< std::remove_cv_t<T>, TT>>>>
    constexpr flat_forward_list_iterator_t & operator= (I && other) noexcept {
        p_ = other.get_ptr();
        other.reset_ptr(nullptr);
        return *this;
    }

    constexpr flat_forward_list_iterator_t &operator= (buffer_char_pointer p) noexcept {
        p_ = p;
        return *this;
    }

    constexpr flat_forward_list_iterator_t &operator= (T *p) noexcept {
        p_ = (buffer_char_pointer)p;
        return *this;
    }

    constexpr flat_forward_list_iterator_t &operator= (buffer_unsigned_char_pointer p) noexcept {
        p_ = (buffer_char_pointer )p;
        return *this;
    }
    
    constexpr flat_forward_list_iterator_t &operator= (buffer_void_pointer p) noexcept {
        p_ = (buffer_char_pointer )p;
        return *this;
    }

    constexpr void swap(flat_forward_list_iterator_t & other) noexcept {
        buffer_char_pointer *tmp = p_;
        p_ = other.p_;
        other.p_ = tmp;
    }

    constexpr explicit operator bool() const {
        return p_ != nullptr;
    }
    //
    // equality between iterator, iterator const,
    // const_iterator and const_iterator const
    //
    template <typename I,
              typename = std::enable_if_t<std::is_same_v<std::remove_cv_t<I>, 
                                                         flat_forward_list_iterator_t< std::remove_cv_t<T>, TT>> ||
                                          std::is_same_v<std::remove_cv_t<I>, 
                                                         flat_forward_list_iterator_t< std::add_const_t<T>, TT>>>>
    constexpr bool operator == (I const &other) const noexcept {
        return p_ == other.get_ptr();
    }
    //
    // enequality between iterator, iterator const,
    // const_iterator and const_iterator const
    //
    template <typename I,
              typename = std::enable_if_t<std::is_same_v<std::remove_cv_t<I>, 
                                                         flat_forward_list_iterator_t< std::remove_cv_t<T>, TT>> ||
                                          std::is_same_v<std::remove_cv_t<I>, 
                                                         flat_forward_list_iterator_t< std::add_const_t<T>, TT>>>>
    constexpr bool operator != (I const &other) const noexcept {
        return !operator==(other);
    }

    constexpr flat_forward_list_iterator_t &operator++() noexcept {
        size_t next_offset = element_traits::get_next_element_offset(p_);
        if (0 == next_offset) {
            p_ = nullptr;
        } else {
            p_ += element_traits::get_next_element_offset(p_);
        }
        return *this;
    }

    constexpr flat_forward_list_iterator_t operator++(int) noexcept {
        flat_forward_list_iterator_t tmp{ p_ };
        size_t next_offset = element_traits::get_next_element_offset(p_);
        if (0 == next_offset) {
            p_ = nullptr; 
        } else {
            p_ += element_traits::get_next_element_offset(p_);
        }
        return tmp;
    }

    constexpr flat_forward_list_iterator_t operator+(unsigned int advance_by) const noexcept {
        flat_forward_list_iterator_t result{ get_ptr() };
        while (nullptr != result.get_ptr() && 0 != advance_by) {
            ++result;
            --advance_by;
        }
        return result;
    }

    constexpr T &operator*() const noexcept {
        return *(T *)(p_);
    }
    
    constexpr T *operator->() const noexcept {
        return (T *)(p_);
    }

    constexpr buffer_char_pointer get_ptr() const noexcept {
        return p_;
    }

    constexpr buffer_char_pointer reset_ptr(buffer_char_pointer p) noexcept {
        return p_ = p;
    }

private:
    buffer_char_pointer p_{ nullptr };
};
//
// non-const iterator
//
template<typename T,
         typename TT = flat_forward_list_traits<T>>
using flat_forward_list_iterator = flat_forward_list_iterator_t<T, TT>;

//
// const_iteartor
//
template<typename T,
         typename TT = flat_forward_list_traits<T>>
using flat_forward_list_const_iterator = flat_forward_list_iterator_t< std::add_const_t<T>, TT>;

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

template <typename T,
          typename TT = flat_forward_list_traits<T>,
          typename A = std::allocator<char>>
class flat_forward_list : private A
                        , private TT
                        , public container_element_type_base<T> {
public:

    //
    // Technically we need T to be 
    // - trivialy destructable
    // - trivialy constructable
    // - trivialy movable
    // - trivialy copyable
    //
    static_assert(std::is_pod_v<T>, "T must be a Plain Old Definition");

    using size_type = typename container_element_type_base<T>::size_type;
    using difference_type = typename container_element_type_base<T>::difference_type;
    using buffer_value_type = char;
    using const_buffer_value_type = char const;
    using element_traits_type = TT;
    using allocator_type = A ;
    using allocator_type_t = std::allocator_traits<A>;
    using buffer_pointer = char *;
    using const_buffer_pointer = char const *;
    using buffer_reference = char & ;
    using const_buffer_reference = char const &;

    //
    // pointer to the start of buffer
    // used buffer size
    // total buffer size
    //
    using detach_type_as_size = std::tuple<char *, size_t, size_t>;
    using detach_type_as_pointers = std::tuple<char *, char *, char *>;

    using iterator = flat_forward_list_iterator<T, TT>;
    using const_iterator = flat_forward_list_const_iterator<T, TT>;

    inline static size_type const npos = std::numeric_limits<size_type>::max();

    flat_forward_list() noexcept {
    }

    explicit flat_forward_list(A a) noexcept 
        : A( a ) {
    }

    flat_forward_list(flat_forward_list && other) noexcept
        : A(std::move(other).get_allocator()) {
        move_from(std::move(other));
    }

    flat_forward_list(flat_forward_list const &other)
        : A(allocator_type_t::select_on_container_copy_construction(other.get_allocator())) {
        copy_from(other);
    }

    template <typename AA = A>
    flat_forward_list(attach_buffer,
                      char *buffer_begin,
                      char *last_element,
                      char *buffer_end,
                      AA &&a = AA{}) noexcept
        : A(std::forward<AA>(a)) {
        attach(buffer_begin, last_element, buffer_end);
    }

    template <typename AA = A>
    flat_forward_list(char const *buffer_begin,
                      char const *last_element,
                      char const *buffer_end,
                      AA &&a = AA{})
        : A( std::forward<AA>(a) ) {
        copy_from_buffer(buffer_begin, last_element, buffer_end);
    }

    template <typename AA = A>
    flat_forward_list(attach_buffer,
                      char *buffer,
                      size_t buffer_size,
                      AA &&a = AA{}) noexcept
        : A( std::forward<AA>(a) ) {
        if (!attach(buffer, buffer_size)) {
            raise_invalid_buffer_exception("flat_forward_list construction with attach buffer validation failed");
        }
    }

    template <typename AA = A>
    flat_forward_list(char const *buffer,
                      size_t buffer_size,
                      AA &&a = AA{}) noexcept
        : A(std::forward<AA>(a)) {
        if (!copy_from_buffer(buffer, buffer_size)) {
            raise_invalid_buffer_exception("flat_forward_list construction with copy buffer validation failed");
        }
    }

    flat_forward_list &operator= (flat_forward_list && other) noexcept (allocator_type_t::propagate_on_container_move_assignment::value) {
        if (this != &other) {
            if constexpr (allocator_type_t::propagate_on_container_move_assignment::value) {
                move_allocator_from(std::move(other));
                move_from(std::move(other));
            } else {
                try_move_from(std::move(other));
            }
        }
        return *this;
    }

    flat_forward_list &operator= (flat_forward_list const &other) {
        if (this != &other) {
            //
            // If we are propagating allocator then delete old data
            // before changing allocator
            //
            if constexpr (allocator_type_t::propagate_on_container_copy_assignment::value) {
                clear();
                *static_cast<A *>(this) = allocator_type_t::select_on_container_copy_construction(other.get_allocator());
            }
            
            copy_from(other);
        }
        return *this;
    }

    ~flat_forward_list() noexcept {
        clear();
    }

    detach_type_as_size detach() noexcept {
        auto result{ std::make_tuple( buffer_begin_,  used_capacity(), total_capacity() ) };
        buffer_begin_ = nullptr;
        buffer_end_ = nullptr;
        last_element_ = nullptr;
        return result;
    }

    detach_type_as_pointers detach(as_pointers) noexcept {
        auto result{ std::make_tuple( buffer_begin_,  last_element_, buffer_end_) };
        buffer_begin_ = nullptr;
        buffer_end_ = nullptr;
        last_element_ = nullptr;
        return result;
    }

    void attach(char *buffer_begin,
                char *last_element,
                char *buffer_end) {

        FFL_CODDING_ERROR_IF(buffer_begin_ == buffer_begin);
        FFL_CODDING_ERROR_IF_NOT(buffer_begin < last_element && last_element < buffer_end);
        clear();
        buffer_begin_ = buffer_begin;
        buffer_end_ = buffer_end;
        last_element_ = last_element;
    }

    bool try_attach(char *buffer,
                    size_t buffer_size) noexcept {
        FFL_CODDING_ERROR_IF(buffer_begin_ == buffer);
        auto[is_valid, last_valid] = flat_forward_list_validate<T, TT>(buffer, 
                                                                       buffer + buffer_size);
        if (is_valid) {
            attach(buffer, last_valid, buffer + buffer_size);
        }
        return is_valid;
    }

    void copy_from_buffer(char const *buffer_begin, 
                          char const *last_element, 
                          char const *buffer_end) {
        flat_forward_list l(get_allocator());

        FFL_CODDING_ERROR_IF_NOT(buffer_begin < last_element && last_element < buffer_end);

        size_type buffer_size = buffer_end - buffer_begin;
        size_type last_element_offset = last_element - buffer_begin;

        l.buffer_begin_ = allocate_buffer(buffer_size);
        copy_data(l.buffer_begin_, buffer_begin, buffer_size);
        l.buffer_end_ = l.buffer_begin_ + buffer_size;
        l.last_element_ = l.buffer_begin_ + last_element_offset;
        swap(l);
    }

    bool copy_from_buffer(char const *buffer_begin, 
                          char const *buffer_end) {

        auto[is_valid, last_valid] = flat_forward_list_validate<T, TT>(buffer_begin,
                                                                       buffer_end);
        if (is_valid) {
            copy_from_buffer(buffer_begin,
                             last_valid,
                             buffer_end);
        }

        return is_valid;
    }

    bool copy_from_buffer(char const *buffer,
                          size_type buffer_size) {

        return copy_from_buffer(buffer, buffer + buffer_size);
    }

    template<typename AA>
    bool is_compatible_allocator(AA const &other_allocator) const noexcept {
        return other_allocator == get_allocator();
    }

    A &get_allocator() & noexcept {
        return *this;
    }

    A const &get_allocator() const & noexcept {
        return *this;
    }

    A && get_allocator() && noexcept {
        return std::move(*this);
    }

    size_type max_size() const noexcept {
        return allocator_type_t::max_size(get_allocator());
    }

    void clear() noexcept {
        validate_pointer_invariants();
        if (buffer_begin_) {
            deallocate_buffer(buffer_begin_, total_capacity());
            buffer_begin_ = nullptr;
            buffer_end_ = nullptr;
            last_element_ = nullptr;
        }
        validate_pointer_invariants();
    }

    void shrink_to_fit() {
        resize_buffer(used_capacity());
    }

    void resize_buffer(size_type size) {
        validate_pointer_invariants();

        all_sizes prev_sizes{ get_all_sizes() };

        char *new_buffer{ nullptr };
        size_t new_buffer_size{ 0 };
        auto deallocate_buffer{ make_scoped_deallocator(&new_buffer, &new_buffer_size) };
        //
        // growing capacity
        //
        if (prev_sizes.total_capacity < size) {
            new_buffer = allocate_buffer(size);
            new_buffer_size = size;
            if (last_element_) {
                copy_data(new_buffer, buffer_begin_, prev_sizes.used_capacity);
                last_element_ = new_buffer + prev_sizes.last_element_offset;
            }
            commit_new_buffer(new_buffer, new_buffer_size);
            buffer_end_ = buffer_begin_ + size;
        //
        // shrinking to 0 is simple
        //
        } else if (0 == size) {
            clear();
        //
        // to shrink to a smaler size we first need to 
        // find last element that would completely fit 
        // in the new buffer, and if we found any, then
        // make it new last element. If we did not find 
        // any, then designate that there are no last 
        // element
        //
        } else if (prev_sizes.total_capacity > size) {

            new_buffer = allocate_buffer(size);
            new_buffer_size = size;
            //
            // shrinking. 
            //
            auto[is_valid, last_valid] = flat_forward_list_validate<T, TT>(buffer_begin_, buffer_begin_ + size);
            if (is_valid) {
                FFL_CODDING_ERROR_IF_NOT(last_valid == last_element_);
            } else {
                FFL_CODDING_ERROR_IF_NOT(last_valid != last_element_);
            }

            size_t new_last_element_offset{ 0 };
            size_t new_last_element_size{ 0 };
            size_t new_used_capacity{ 0 };

            if (last_valid) {
                new_last_element_offset = last_valid - buffer_begin_;
                new_last_element_size = element_traits_type::calculate_next_element_offset(last_valid);
                new_used_capacity = new_last_element_offset + new_last_element_size;

                element_traits_type::set_next_element_offset(last_valid, 0);
                copy_data(new_buffer, buffer_begin_, new_used_capacity);
                last_element_ = new_buffer + new_last_element_offset;
            } else {
                last_element_ = nullptr;
            }

            commit_new_buffer(new_buffer, new_buffer_size);
            buffer_end_ = buffer_begin_ + size;
        }

        validate_pointer_invariants();
        validate_data_invariants();
    }

    void push_back(size_type init_buffer_size,
                   char const *init_buffer = nullptr) {

        emplace_back(init_buffer_size,
                      [init_buffer_size, init_buffer](char *buffer,
                                                      size_type element_size) {
                        FFL_CODDING_ERROR_IF_NOT(init_buffer_size == element_size);

                        if (init_buffer) {
                            copy_data(buffer, init_buffer, element_size);
                        } else {
                            zero_buffer(buffer, element_size);
                        }
                     });
    }

    template <typename F, typename ... P>
    void emplace_back(size_type element_size,
                      F const &fn,
                      P&& ... p) {
        validate_pointer_invariants();

        FFL_CODDING_ERROR_IF(element_size < element_traits_type::minimum_size());
      
        char *new_buffer{ nullptr };
        size_t new_buffer_size{ 0 };
        auto deallocate_buffer{ make_scoped_deallocator(&new_buffer, &new_buffer_size) };

        all_sizes prev_sizes{get_all_sizes()};
      
        char *cur{ nullptr };

        //
        // Do we need to resize buffer?
        //
        if (prev_sizes.remaining_capacity < element_size) {
            new_buffer = allocate_buffer(prev_sizes.used_capacity + element_size);
            new_buffer_size = prev_sizes.used_capacity + element_size;
            cur = new_buffer + prev_sizes.used_capacity;
        } else {
            cur = buffer_begin_ + prev_sizes.used_capacity;
        }

        fn(cur, element_size, std::forward<P>(p)...);

        element_traits_type::set_next_element_offset(cur, 0);
        //
        // data can consume less than requested size, 
        // but should not consume more
        //
        FFL_CODDING_ERROR_IF(element_size < element_traits_type::calculate_next_element_offset(cur));
        //
        // element that used to be last is becomes just
        // an element in the middle so we need to change 
        // its next element pointer
        //
        if (last_element_) {
            element_traits_type::set_next_element_offset(last_element_, prev_sizes.last_element_size);
        }
        //
        // now see if we need to update existing 
        // buffer or swap with the new buffer
        //
        if (new_buffer != nullptr) {
            //
            // If we are reallocating buffer then move data
            //
            if (buffer_begin_) {
                copy_data(new_buffer, buffer_begin_, prev_sizes.used_capacity);
            }
            commit_new_buffer(new_buffer, new_buffer_size);
        }
        last_element_ = cur;

        validate_pointer_invariants();
        validate_data_invariants();
    }

    iterator insert(iterator const &it, size_type init_buffer_size, char const *init_buffer = nullptr) {
        emplace(it, 
                init_buffer_size,
                [init_buffer_size, init_buffer](char *buffer,
                                                size_type element_size) {
                    FFL_CODDING_ERROR_IF_NOT(init_buffer_size == element_size);

                    if (init_buffer) {
                        copy_data(buffer, init_buffer, element_size);
                    } else {
                        zero_buffer(buffer, element_size);
                    }
                });
    }

    template <typename F, typename ... P>
    iterator emplace(iterator const &it,
                     size_type new_element_size, 
                     F const &fn,
                     P&& ... p) {

        validate_pointer_invariants();
        validate_iterator(it);

        FFL_CODDING_ERROR_IF(new_element_size < element_traits_type::minimum_size());

        if (nullptr == it.get_ptr()) {
            emplace_back(new_element_size, fn, std::forward<P>(p)...);
            return last();
        }

        char *new_buffer{ nullptr };
        size_t new_buffer_size{ 0 };
        auto deallocate_buffer{ make_scoped_deallocator(&new_buffer, &new_buffer_size) };

        all_sizes prev_sizes{ get_all_sizes() };
        auto[element_start, element_end] = element_range(it);
        //
        // Note that in this case old element that was in that position
        // is a part of the tail
        //
        size_type tail_size{ prev_sizes.used_capacity - element_start };

        char *begin{ nullptr };
        char *cur{ nullptr };

        //
        // Do we need to resize buffer?
        //
        if (prev_sizes.remaining_capacity < new_element_size) {
            new_buffer = allocate_buffer(prev_sizes.used_capacity + new_element_size);
            new_buffer_size = prev_sizes.used_capacity + new_element_size;
            cur = new_buffer + element_start;
            begin = new_buffer;
        } else {
            cur = it.get_ptr();
            begin = buffer_begin_;
        }
        char *new_tail_start{ nullptr };
        //
        // Common part where we construct new part of the buffer
        //
        // Move tail forward. 
        // This will free space to insert new element
        //
        if (!new_buffer) {
            new_tail_start = begin + element_start + new_element_size;
            move_data(new_tail_start, it.get_ptr(), tail_size);
        }
        //
        // construct
        //
        try {
            fn(cur, new_element_size, std::forward<P>(p)...);
        } catch (...) {
            //
            // on failure to costruct move tail back
            // only if we are not reallocating
            //
            if (!new_buffer) {
                move_data(it.get_ptr(), new_tail_start, tail_size);
            }
            throw;
        }

        element_traits_type::set_next_element_offset(cur, new_element_size);
        //
        // data can consume less than requested size, 
        // but should not consume more
        //
        FFL_CODDING_ERROR_IF(new_element_size < element_traits_type::calculate_next_element_offset(cur));
        //
        // now see if we need to update existing 
        // buffer or swap with the new buffer
        //
        if (new_buffer != nullptr) {
            //
            // If we are reallocating buffer then move all elements
            // before and after position that we are inserting to
            //
            if (buffer_begin_) {
                copy_data(new_buffer, buffer_begin_, element_start);
                copy_data(cur + new_element_size, it.get_ptr(), tail_size);
            }
            commit_new_buffer(new_buffer, new_buffer_size);
        }
        //
        // Last element moved ahead by the size of the new inserted element
        //
        last_element_ = buffer_begin_ + prev_sizes.last_element_offset + new_element_size;

        validate_pointer_invariants();
        validate_data_invariants();
        //
        // Return iterator pointing to the new inserted element
        //
        return iterator{ cur };
    }

    void push_front(size_type init_buffer_size, char const *init_buffer = nullptr) {
        emplace(begin(),
                init_buffer_size,
                [init_buffer_size, init_buffer](char *buffer,
                                                size_type element_size) {
                    FFL_CODDING_ERROR_IF_NOT(init_buffer_size == element_size);

                    if (init_buffer) {
                        copy_data(buffer, init_buffer, element_size);
                    } else {
                        zero_buffer(buffer, element_size);
                    }
                });
    }

    template <typename F
              //, typename ... P
              >
    void emplace_front(size_type element_size, 
                       F const &fn
                       //, P&& ... p
                       ) {
        //emplace(begin(), element_size, fn, std::forward<P>(p)...);
        emplace(begin(), element_size, fn);
    }

    void pop_front() {
        validate_pointer_invariants();

        FFL_CODDING_ERROR_IF(nullptr == last_element_);
        //
        // If we have only one element then simply forget it
        //
        if (last_element_ == buffer_begin_) {
            last_element_ = nullptr;
            return;
        }
        //
        // Otherwise calculate sizes and offsets
        //
        all_sizes prev_sizes{ get_all_sizes() };
        auto[first_element_start_offset, first_element_end_offset] = element_range(iterator{ buffer_begin_ });
        FFL_CODDING_ERROR_IF_NOT(first_element_start_offset == 0);
        size_type bytes_to_copy{ prev_sizes.used_capacity - first_element_end_offset };
        //
        // Shift all elements after the first element
        // to the start of buffer
        //
        move_data(buffer_begin_, buffer_begin_ + first_element_end_offset, bytes_to_copy);

        last_element_ = buffer_begin_ + prev_sizes.last_element_offset - first_element_end_offset;

        validate_pointer_invariants();
        validate_data_invariants();
    }

    void erase_after(iterator const &it) noexcept {
        validate_pointer_invariants();
        //
        // Canot erase after end
        //
        validate_iterator_not_end(it);
        //
        // There is no elements after last
        //
        FFL_CODDING_ERROR_IF(it.get_ptr() == last_element_);
        FFL_CODDING_ERROR_IF(nullptr == last_element_);
        //
        // Find pointer to the element that we are erasing
        //
        iterator element_to_erase_it = it;
        ++element_to_erase_it;
        bool erasing_last_element = last_element_ == element_to_erase_it.get_ptr();

        if (erasing_last_element) {
            //
            // trivial case when we are deleting last element and this element 
            // is becoming last
            //
            element_traits_type::set_next_element_offset(it.get_ptr(), 0);
            last_element_ = it.get_ptr();
        } else {
            //
            // calculate sizes and offsets
            //
            all_sizes prev_sizes{ get_all_sizes() };
            auto[element_to_erase_start_offset, element_to_erase_end_offset] = element_range(element_to_erase_it);
            size_type element_to_erase_size{ element_to_erase_end_offset - element_to_erase_start_offset };
            size_type tail_size{ prev_sizes.used_capacity - element_to_erase_end_offset };
            //
            // Shift all elements after the element that we are erasing
            // to the position where erased element used to be
            //
            move_data(buffer_begin_ + element_to_erase_start_offset, buffer_begin_ + element_to_erase_end_offset, tail_size);
            last_element_ -= element_to_erase_size;
        }

        validate_pointer_invariants();
        validate_data_invariants();
    }

    //
    // Usualy pair of iterators define aa half opened range [start, end)
    // Note that in this case our range is (start, last]. In other words
    // we will erase all elements after start, including last.
    // We give this method unusual name so people would stop and read
    // this comment about range
    //
    void erase_after_half_closed(iterator const &before_start, iterator const &last) noexcept {
        validate_pointer_invariants();
        //
        // Canot erase after end
        //
        validate_iterator_not_end(before_start);
        validate_iterator(before_start);
        //
        // There is no elements after last
        //
        FFL_CODDING_ERROR_IF(before_start.get_ptr() == last_element_);
        //
        // If we are truncating entire tail
        //
        if (nullptr == last.get_ptr()) {
            erase_all_after(before_start);
            return;
        }
        //
        // We can support before_start == last by simply calling erase(last).
        // That will change complexity of algorithm by adding extra O(N).
        //
        FFL_CODDING_ERROR_IF_NOT(before_start.get_ptr() < last.get_ptr());

        //
        // Find pointer to the element that we are erasing
        //
        iterator first_element_to_erase_it = before_start;
        ++first_element_to_erase_it;
        //
        // calculate sizes and offsets
        //
        all_sizes prev_sizes{ get_all_sizes() };
        auto[first_element_to_erase_start_offset, first_element_to_erase_end_offset] = element_range(first_element_to_erase_it);
        auto[last_element_to_erase_start_offset, last_element_to_erase_end_offset] = element_range(last);
        size_type bytes_to_copy{ prev_sizes.used_capacity - last_element_to_erase_end_offset };
        size_type bytes_erased{ last_element_to_erase_end_offset - first_element_to_erase_start_offset };
        //
        // Shift all elements after the last element that we are erasing
        // to the position where first erased element used to be
        //
        move_data(buffer_begin_ + first_element_to_erase_start_offset, 
                  buffer_begin_ + last_element_to_erase_end_offset, 
                  bytes_to_copy);
        last_element_ -= bytes_erased;

        validate_pointer_invariants();
        validate_data_invariants();
    }

    void erase_all_after(iterator const &it) noexcept {
        validate_pointer_invariants();
        validate_iterator(it);

        //
        // erasing after end iterator is a noop
        //
        if (nullptr != it.get_ptr()) {
            last_element_ = it.get_ptr();
            element_traits_type::set_next_element_offset(last_element_, 0);

            validate_pointer_invariants();
            validate_data_invariants();
        }
    }

    iterator erase_all_from(iterator const &it) noexcept {
        validate_pointer_invariants();
        validate_iterator(it);

        //
        // erasing after end iterator is a noop
        //
        if (nullptr != it.get_ptr()) {

            if (it.get_ptr() == buffer_begin_) {
                last_element_ = nullptr;
                return end();
            }

            auto[element_to_erase_start_offset, element_to_erase_end_offset] = element_range(it);
            iterator element_before = find_element_before(element_to_erase_start_offset);
            FFL_CODDING_ERROR_IF(nullptr == element_before.get_ptr());

            erase_all_after(element_before);

            element_traits_type::set_next_element_offset(element_before.get_ptr(), 0);

            return element_before;
        }

        return end();
    }

    void erase_all() noexcept {
        validate_pointer_invariants();
        last_element_ = nullptr;
    }

    //
    // This gives us erase semantics everyone is used to, but increases 
    // complexity by O(n)
    //
    iterator erase(iterator const &it) noexcept {
        validate_pointer_invariants();
        validate_iterator_not_end(it);

        if (it.get_ptr() == buffer_begin_) {
            pop_front();
            return begin();
        }

        auto[element_to_erase_start_offset, element_to_erase_end_offset] = element_range(it);

        if (it.get_ptr() == last_element_) {
            //
            // if we are erazing last element then
            // we need to find element before last
            // and update it's next to 0. 
            // Next element is end.
            //
            iterator element_before_it{ find_element_before(element_to_erase_start_offset) };
            FFL_CODDING_ERROR_IF(nullptr == element_before_it.get_ptr());
            element_traits_type::set_next_element_offset(element_before_it.get_ptr(), 0);
            last_element_ = element_before_it.get_ptr();
            return iterator{};
        } else {
            all_sizes prev_sizes{ get_all_sizes() };
            size_type element_size{ element_to_erase_end_offset - element_to_erase_start_offset };
            size_type tail_size{ prev_sizes.used_capacity - element_to_erase_end_offset };
            //
            // shift remaining elements will erase this element.
            //
            move_data(buffer_begin_ + element_to_erase_start_offset, buffer_begin_ + element_to_erase_end_offset, tail_size);
            last_element_ -= element_size;
            return it;
        }
    }

    iterator erase(iterator const &start, iterator const &end) noexcept {
        validate_pointer_invariants();
        validate_iterator_not_end(start);
        validate_iterator(end);

        if (nullptr == end.get_ptr()) {
            return erase_all_from(start);
        }

        if (start == end) {
            return start;
        }

        all_sizes prev_sizes{ get_all_sizes() };

        auto[first_element_to_erase_start_offset, first_element_to_erase_end_offset] = element_range(start);
        auto[end_element_to_erase_start_offset, end_element_to_erase_end_offset] = element_range(end);
        size_type bytes_to_copy{ prev_sizes.used_capacity - end_element_to_erase_start_offset};
        size_type bytes_erased{ end_element_to_erase_start_offset - first_element_to_erase_start_offset };

        move_data(buffer_begin_ + first_element_to_erase_start_offset, 
                  buffer_begin_ + end_element_to_erase_start_offset, 
                  bytes_to_copy);

        last_element_ -= bytes_erased;

        validate_pointer_invariants();
        validate_data_invariants();

        return start;
    }

    void swap(flat_forward_list &other) noexcept (allocator_type_t::propagate_on_container_swap::value ||
                                                  allocator_type_t::propagate_on_container_move_assignment::value) {
        if constexpr (allocator_type_t::propagate_on_container_swap::value) {
            std::swap(get_allocator(), other.get_allocator());
            std::swap(buffer_begin_, other.buffer_begin_);
            std::swap(buffer_end_, other.buffer_end_);
            std::swap(last_element_, other.last_element_);
        } else {
            flat_forward_list tmp{ std::move(other) };
            other = std::move(*this);
            *this = std::move(tmp);
        }
    }

    template <typename LESS_F>
    void sort(LESS_F const &fn) {
        //
        // Create a vector of iterators to all elements in the
        // list and sort it using function passed to us by the 
        // user.
        //
        std::vector<const_iterator> iterator_array;      
        for (const_iterator i = begin(); i != end(); ++i) {
            iterator_array.push_back(i);
        }
        std::sort(iterator_array.begin(), 
                  iterator_array.end(),
                  [&fn](const_iterator const &lhs, 
                        const_iterator const &rhs) -> bool {
                        
                        return fn(*lhs, *rhs);
                  });

        //
        // Now that we have an array of sorted iterators
        // copy elements to the new list in the sorting order
        //
        flat_forward_list sorted_list(get_allocator());
        sorted_list.resize_buffer(used_capacity());
        for (const_iterator const &i : iterator_array) {
            sorted_list.push_back(element_used_size(i), i.get_ptr());
        }

        //
        // Swap with sorted list
        //
        swap(sorted_list);
    }

    void reverse() {
        flat_forward_list reversed_list(get_allocator());
        reversed_list.resize_buffer(used_capacity());
        for (iterator const &i = begin(); i != end(); ++i) {
            reversed_list.push_front(i.get_ptr(), element_used_size(i));
        }
        //
        // Swap with reversed list
        //
        swap(reversed_list);
    }

    template<class F>
    void merge(flat_forward_list &other, 
               F const &fn) {

        flat_forward_list merged_list(get_allocator());

        iterator this_start = begin();
        iterator this_end = end();
        iterator other_start = other.begin();
        iterator other_end = other.end();

        for ( ; this_start != this_end && other_start != other_end;) {
            if (fn(*this_start, *other_start)) {
                merged_list.push_back(element_required_size(this_start), this_start.get_ptr());
                ++this_start;
            } else {
                merged_list.push_back(other.element_required_size(other_start), other_start.get_ptr());
                ++other_start;
            }
        }

        for (; this_start != this_end; ++this_start) {
            merged_list.push_back(element_required_size(this_start), this_start.get_ptr());
        }

        for (; other_start != other_end; ++other_start) {
            merged_list.push_back(other.element_required_size(other_start), other_start.get_ptr());
        }

        swap(merged_list);
        other.clear();
    }

    template<typename F>
    void unique(F const &fn) {   
        iterator first = begin();       
        if (first != end()) {
            iterator last = end();
            iterator after = first;
            for (++after; after != end(); ++after) {
                if (fn(*first, *after)) {
                    //
                    // remember last element for which predicate is true
                    //
                    last = after;
                } else if (last != end()) {
                    //
                    // If predicate is not true then erase (first, last]
                    // elements after last shift left so first+1 is pointing
                    // to the first element that is not equal to what we just 
                    // deleted.
                    //
                    erase_after_half_closed(first, last);
                    last = end();
                    ++first;
                    after = first;
                } else {
                    //
                    // nothing to delete, just move first forward
                    //
                    first = after;
                }
            }
        }
    }

    template<typename F>
    void remove_if(F const &fn) {
        iterator first = end();
        for (iterator last = begin(); 
             last != end(); 
             ++last) {

            if (fn(*last)) {
                if (first == end()) {
                    first = last;
                }
            } else if (first != end()) {
                last = erase(first, last);
                first = end();
            } 
        }
    }
     
    T &front() {
        validate_pointer_invariants();
        FFL_CODDING_ERROR_IF(last_element_ == nullptr || buffer_begin_ == nullptr);
        return *(T *)buffer_begin_;
    }

    T const &front() const {
        validate_pointer_invariants();
        FFL_CODDING_ERROR_IF(last_element_ == nullptr || buffer_begin_ == nullptr);
        return *(T *)buffer_begin_;
    }

    T &back() {
        validate_pointer_invariants();
        FFL_CODDING_ERROR_IF(last_element_ == nullptr);
        return *(T *)last_element_;
    }

    T const &back() const {
        validate_pointer_invariants();
        FFL_CODDING_ERROR_IF(last_element_ == nullptr);
        return *(T *)last_element_;
    }

    iterator begin() noexcept {
        validate_pointer_invariants();
        return last_element_ ? iterator{ buffer_begin_ }
                             : iterator{ };
    }

    const_iterator begin() const noexcept {
        validate_pointer_invariants();
        return last_element_ ? const_iterator{ buffer_begin_ }
                             : const_iterator{ };
    }

    //
    // It is not clear how to implement it 
    // without adding extra boolen flags to iterator.
    // Since elements are variable length we need to be able
    // to query offset to next, and we cannot not do that
    // no before_begin element
    //
    // iterator before_begin() noexcept {
    // }

    iterator last() noexcept {
        validate_pointer_invariants();
        return last_element_ ? iterator{ last_element_ }
                             : iterator{ };
    }

    const_iterator last() const noexcept {
        validate_pointer_invariants();
        return last_element_ ? const_iterator{ last_element_ }
                             : const_iterator{ };
    }

    iterator end() noexcept {
        validate_pointer_invariants();
        return iterator{ };
    }

    const_iterator end() const noexcept {
        validate_pointer_invariants();
        return const_iterator{ };
    }

    const_iterator cbegin() const noexcept {
        validate_pointer_invariants();
        return last_element_ ? const_iterator{ buffer_begin_ }
                             : const_iterator{ };
    }

    const_iterator clast() const noexcept {
        validate_pointer_invariants();
        return last_element_ ? const_iterator{ last_element_ }
                             : iterator{ };
    }

    const_iterator cend() const noexcept {
        validate_pointer_invariants();
        return const_iterator{ };
    }

    char *data() noexcept {
        return buffer_begin_;
    }

    char const *data() const noexcept {
        return buffer_begin_;
    }

    bool revalidate_data() {
        auto[valid, last] = flat_forward_list_validate<T, TT>(buffer_begin_, buffer_end_);
        if (valid) {
            last_element_ = last;
        }
        return valid;
    }

    void all_elements_shrink_to_fit() {
        for (iterator const &i = begin(); i != end(); ++i) {
            element_shrink_to_fit(i);
        }
    }

    void element_shrink_to_fit(iterator const &it) {
        validate_pointer_invariants();
        validate_iterator_not_end(it);

        iterator ret_it = element_resize(it,
                                           element_required_size(it),
                                           [] (char *buffer,  
                                               size_type old_size, 
                                               size_type new_size) {
                                               //
                                               // must be shrinking, and
                                               // data must fit new buffer
                                               //
                                               FFL_CODDING_ERROR_IF_NOT(new_size <= old_size);
                                               //
                                               // Cannot assert it here sice we have not changed next element offset yet
                                               // we will validate element at the end
                                               //
                                               //FFL_CODDING_ERROR_IF_NOT(element_traits_type::validate(new_size, buffer));
                                           });
        //
        // We are either shrinking or not changing size 
        // so there should be no reallocation 
        //
        FFL_CODDING_ERROR_IF_NOT(it == ret_it);
    }

    iterator element_add_size(iterator const &it,
                              size_type size_to_add) {
        validate_pointer_invariants();
        validate_iterator_not_end(it);

        return element_resize(it, 
                              element_used_size(it) + size_to_add,
                              [] (char *buffer,  
                                  size_type old_size, 
                                  size_type new_size) {
                                  //
                                  // must be extending, and
                                  // data must fit new buffer
                                  //
                                  FFL_CODDING_ERROR_IF_NOT(old_size <= new_size);
                                  zero_buffer(buffer + old_size, new_size - old_size);
                                  FFL_CODDING_ERROR_IF_NOT(element_traits_type::validate(new_size, buffer));
                              });
    }

    //
    // Should take constructor
    //
    template <typename F
             ,typename ... P
             >
    iterator element_resize(iterator const &it, 
                            size_type element_new_size, 
                            F const &fn
                            ,P&& ... p
                           ) noexcept {
        validate_pointer_invariants();
        validate_iterator_not_end(it);
        //
        // Resize to 0 is same as erase
        //
        if (0 == element_new_size) {
            return erase(it);
        }

        FFL_CODDING_ERROR_IF(element_new_size < element_traits_type::minimum_size());

        char *new_buffer{ nullptr };
        size_type new_buffer_size{ 0 };
        auto deallocate_buffer{ make_scoped_deallocator(&new_buffer, &new_buffer_size) };

        all_sizes prev_sizes{ get_all_sizes() };
        auto[element_start, element_end] = element_range(it);
        size_type element_size{ element_end - element_start };
        //
        // erase to the same size is a noop
        //
        if (element_new_size == element_size) {
            return it;
        }

        size_type tail_size{ prev_sizes.used_capacity - element_end };
        //
        // negative if shrinking and positive if expanding
        //
        difference_type element_size_diff{ static_cast<difference_type>(element_new_size - element_size) };

        char *begin{ nullptr };
        char *cur{ nullptr };

        //
        // Do we need to resize buffer?
        //
        if (static_cast<difference_type>(prev_sizes.remaining_capacity) < element_size_diff) {
            new_buffer = allocate_buffer(prev_sizes.used_capacity + element_new_size - element_size);
            new_buffer_size = prev_sizes.used_capacity + element_new_size - element_size;
            cur = new_buffer + element_start;
            begin = new_buffer;
        } else {
            cur = it.get_ptr();
            begin = buffer_begin_;
        }
        //
        // Common part where we construct new part of the buffer
        //

        if (nullptr != new_buffer) {
            //
            // If we are reallocating then start from copying element
            //
            copy_data(begin + element_start, 
                      buffer_begin_ + element_start, 
                      element_size);

        } else if (tail_size && 0 < element_size_diff) {
            //
            // if we are extending then shift tail to the right.
            // Note that after that element might end up having 
            // some unused capacity, and we are not going to shrink 
            // it back.
            //
            move_data(buffer_begin_ + element_start + element_new_size, 
                      buffer_begin_ + element_start + element_size, 
                      tail_size);
            //
            // If this is not the last element then change size
            //
            if (tail_size != 0) {
                element_traits_type::set_next_element_offset(cur, element_new_size);
            }

        } else {
            //
            // we are shrinking, nothing to do yet
            // 
        }
        //
        // Attempt to resize data
        //
        fn(cur, element_size, element_new_size, std::forward<P>(p)...);
        //fn(cur, element_size, element_new_size);
        //
        // data can consume less than requested size, 
        // but should not consume more
        //
        FFL_CODDING_ERROR_IF_NOT(element_traits_type::calculate_next_element_offset(cur) <= element_new_size);
        //
        // now see if we need to update existing 
        // buffer or swap with the new buffer
        //
        if (new_buffer != nullptr) {
            //
            // If we are reallocating buffer then move all elements
            // before position that we are inserting
            //
            if (tail_size) {
                element_traits_type::set_next_element_offset(cur, element_new_size);
            }
            copy_data(new_buffer, buffer_begin_, element_start);
            copy_data(cur + element_new_size, buffer_begin_ + element_end, tail_size);

            commit_new_buffer(new_buffer, new_buffer_size);
        } else if (tail_size && element_size_diff < 0) {
            //
            // if we are shrinking in place then shift tail to the left.
            //
            move_data(buffer_begin_ + element_start + element_new_size,
                      buffer_begin_ + element_start + element_size,
                      tail_size);
            //
            // If this is not the last element then change size
            //
            if (tail_size != 0) {
                element_traits_type::set_next_element_offset(cur, element_new_size);
            }
        }
        //
        // Adjust last element pointer.
        // Note that element_size_diff
        // is negative when element shrinks.
        // if tail is 0 then there is no elements 
        // after this one and last element is at 
        // the same offset as it used to be
        //
        last_element_ = buffer_begin_ + 
                        static_cast<difference_type>(prev_sizes.last_element_offset) + 
                        static_cast<difference_type>(tail_size ? element_size_diff : 0);

        validate_pointer_invariants();
        validate_data_invariants();
        //
        // Return iterator pointing to the new inserted element
        //
        return iterator{ cur };
    }

    size_type element_required_size(const_iterator const &it) const noexcept {
        validate_pointer_invariants();
        validate_iterator_not_end(it);
        return element_traits_type::calculate_next_element_offset(it.get_ptr());
    }

    size_type element_used_size(const_iterator const &it) const noexcept {
        validate_pointer_invariants();
        validate_iterator_not_end(it);
        size_type next_offset = element_traits_type::get_next_element_offset(it.get_ptr());
        if (0 != next_offset) {
            return next_offset;
        }
        size_type requred_size = element_traits_type::calculate_next_element_offset(it.get_ptr());
        return requred_size;
    }

    std::pair<size_type, size_type> element_range(const_iterator const &it) const noexcept {
        validate_iterator(it);
        if (nullptr == it.get_ptr()) {
            return std::make_pair(npos, npos);
        }

        size_type start_offset = it.get_ptr() - buffer_begin_;
        size_type end_offset = start_offset;
        if (last_element_ == it.get_ptr()) {
            end_offset += element_traits_type::calculate_next_element_offset(it.get_ptr());
        } else {
            end_offset += element_traits_type::get_next_element_offset(it.get_ptr());
        }
        return std::make_pair(start_offset, end_offset);
    }

    bool element_contains(const_iterator const &it, size_type position) const noexcept {
        validate_iterator(it);
        if (nullptr == it.get_ptr() || npos == position) {
            return false;
        }
        auto[start_offset, end_offset] = element_range(it);
        return start_offset <= position && position < end_offset;
    }

    iterator find_element_before(size_type position) noexcept {
        validate_pointer_invariants();
        if (!last_element_) {
            return iterator{};
        }
        auto[is_valid, last_valid] = flat_forward_list_validate<T, TT>(buffer_begin_, buffer_begin_ + position);
        if (last_valid) {
            return iterator{ const_cast<char *>(last_valid) };
        }
        return iterator{};
    }

    const_iterator find_element_before(size_type position) const noexcept {
        validate_pointer_invariants();
        if (!last_element_) {
            return const_iterator{};
        }
        auto[is_valid, last_valid] = flat_forward_list_validate<T, TT>(buffer_begin_, 
                                                                       buffer_begin_ + position);
        if (last_valid) {
            return const_iterator{ last_valid };
        }
        return const_iterator{};
    }

    iterator find_element_at(size_type position) noexcept {
        iterator it = find_element_before(position);
        if (nullptr != it.get_ptr()) {
            ++it;
            if (nullptr != it.get_ptr()) {
                FFL_CODDING_ERROR_IF_NOT(element_contains(it, position));
                return it;
            }
        }
        return iterator{};
    }

    const_iterator find_element_at(size_type position) const noexcept {
        const_iterator it = find_element_before(position);
        if (nullptr != it.get_ptr()) {
            ++it;
            if (nullptr != it.get_ptr()) {
                FFL_CODDING_ERROR_IF_NOT(element_contains(it, position));
                return it;
            }
        }
        return const_iterator{};
    }

    iterator find_element_after(size_type position) noexcept {
        iterator it = find_element_at(position);
        if (nullptr != it.get_ptr()) {
            ++it;
            if (nullptr != it.get_ptr()) {
                return it;
            }
        }
        return iterator{};
    }

    const_iterator find_element_after(size_type position) const noexcept {
        const_iterator it = find_element_at(position);
        if (nullptr != it.get_ptr()) {
            ++it;
            if (nullptr != it.get_ptr()) {
                return it;
            }
        }
        return const_iterator{};
    }

    size_type size() const noexcept {
        validate_pointer_invariants();
        size_type s = 0;
        std::for_each(cbegin(), cend(), [&s](T const &) {
            ++s;
        });
        return s;
    }

    bool empty() const noexcept {
        validate_pointer_invariants();
        return  last_element_ == nullptr;
    }

    size_type used_capacity() const noexcept {
        validate_pointer_invariants();
        return last_element_ ? (last_element_ - buffer_begin_) +
                                element_traits_type::calculate_next_element_offset(last_element_)
                             : 0;
    }

    size_type total_capacity() const noexcept {
        validate_pointer_invariants();
        return buffer_end_ - buffer_begin_;
    }

    size_type remaining_capacity() const noexcept {
        return total_capacity() - used_capacity();
    }

private:

    void move_allocator_from(flat_forward_list &&other) {
        *static_cast<A *>(this) = std::move(other).get_allocator();
    }

    void move_from(flat_forward_list &&other) noexcept {
        clear();
        buffer_begin_ = other.buffer_begin_;
        buffer_end_ = other.buffer_end_;
        last_element_ = other.last_element_;
        other.buffer_begin_ = nullptr;
        other.buffer_end_ = nullptr;
        other.last_element_ = nullptr;
    }

    void try_move_from(flat_forward_list &&other) {
        if (other.get_allocator() == get_allocator()) {
            move_from(std::move(other));
        } else {
            copy_from(other);
        }
    }

    void copy_from(flat_forward_list const &other) {
        clear();
        if (other.last_element_) {
            all_sizes other_sizes{ other.get_all_sizes() };
            buffer_begin_ = allocate_buffer(other_sizes.used_capacity);
            copy_data(buffer_begin_, other.buffer_begin_, other_sizes.used_capacity);
            buffer_end_ = buffer_begin_ + other_sizes.used_capacity;
            last_element_ = buffer_begin_ + other_sizes.last_element_offset;
        }
    }

    char *allocate_buffer(size_t buffer_size) {
        char *ptr{ allocator_type_t::allocate(*this, buffer_size, 0) };
        FFL_CODDING_ERROR_IF(nullptr == ptr);
        //FFL_CODDING_ERROR_IF(*reinterpret_cast<size_t const*>(&ptr) & 0xFFF);
        return ptr;
    }

    void deallocate_buffer(char *buffer, size_t buffer_size) {
        FFL_CODDING_ERROR_IF(0 == buffer_size || nullptr == buffer);
        allocator_type_t::deallocate(*this, buffer, buffer_size);
    }

    void commit_new_buffer(char *&buffer, size_t &buffer_size) {
        char *old_begin = buffer_begin_;
        FFL_CODDING_ERROR_IF(buffer_end_ < buffer_begin_);
        size_type old_size = buffer_end_ - buffer_begin_;
        FFL_CODDING_ERROR_IF(buffer == nullptr && buffer_size != 0);
        FFL_CODDING_ERROR_IF(buffer != nullptr && buffer_size == 0);
        buffer_begin_ = buffer;
        buffer_end_ = buffer_begin_ + buffer_size;
        buffer = old_begin;
        buffer_size = old_size;
    }

    auto make_scoped_deallocator(char **buffer, size_t *buffer_size) {
        return make_scope_guard([this, buffer, buffer_size]() {
            if (*buffer) {
                deallocate_buffer(*buffer, *buffer_size);
                *buffer = nullptr;
                *buffer_size = 0;
            }
        });
    }

    void validate_data_invariants() const noexcept {
#ifdef FFL_DBG_CHECK_DATA_VALID
        auto[valid, last] = flat_forward_list_validate<T, TT>(buffer_begin_, buffer_end_);
        FFL_CODDING_ERROR_IF_NOT(valid || nullptr == last);
        FFL_CODDING_ERROR_IF_NOT(last == last_element_);
#endif //FFL_DBG_CHECK_DATA_VALID
    }

    void validate_pointer_invariants() const noexcept {
        //FFL_CODDING_ERROR_IF(*reinterpret_cast<size_t const*>(&buffer_begin_) & 0xFFF);
        if (nullptr == last_element_) {
            FFL_CODDING_ERROR_IF_NOT(buffer_begin_ <= buffer_end_);
        } else {
            FFL_CODDING_ERROR_IF_NOT(buffer_begin_ <= last_element_ && last_element_ <= buffer_end_);
        }
    }

    void validate_iterator(const_iterator const &it) const noexcept {
        //
        // If we do not have any valid elements then
        // only end iterator is valid.
        // Otherwise iterator should be an end or point somewhere
        // between begin of the buffer and start of the first element
        //
        if (nullptr == last_element_) {
            FFL_CODDING_ERROR_IF_NOT(nullptr == it.get_ptr());
        } else {
            FFL_CODDING_ERROR_IF_NOT(nullptr == it.get_ptr() ||
                                     buffer_begin_ <= it.get_ptr() && it.get_ptr() <= last_element_);
        }
    }

    void validate_iterator_not_end(const_iterator const &it) const noexcept {
        //
        // Used in case when end iterator is meaningless.
        // Validates that container is not empty,
        // and iterator is pointing somewhere between
        // begin of the buffer and start of the first element
        //
        FFL_CODDING_ERROR_IF(nullptr == it.get_ptr());
        FFL_CODDING_ERROR_IF_NOT(buffer_begin_ <= it.get_ptr() && it.get_ptr() <= last_element_);
        validate_compare_to_all_valid_elements(it);
    }

    void validate_compare_to_all_valid_elements(const_iterator const &it) const noexcept {
#ifdef FFL_DBG_CHECK_ITERATOR_VALID
        //
        // If not end iterator then must point to one of the valid iterators.
        //
        if (it.get_ptr()) {
            bool found_match{ false };
            for (auto cur = cbegin(); cur != cend(); ++cur) {
                if (cur == it) {
                    found_match = true;
                    break;
                }
            }
            FFL_CODDING_ERROR_IF_NOT(found_match);
        }
#else //FFL_DBG_CHECK_ITERATOR_VALID
        it;
#endif //FFL_DBG_CHECK_ITERATOR_VALID
    }

    struct all_sizes {
        size_type last_element_offset{ 0 };
        size_type last_element_size{ 0 };
        size_type used_capacity{ 0 };
        size_type total_capacity{ 0 };
        size_type remaining_capacity{ 0 };
    };

    all_sizes get_all_sizes() const noexcept {
        all_sizes s;

        s.total_capacity = buffer_end_ - buffer_begin_;

        if (last_element_) {
            s.last_element_offset = last_element_ - buffer_begin_;
            s.last_element_size = element_traits_type::calculate_next_element_offset(last_element_);
            s.used_capacity = s.last_element_offset + s.last_element_size;
        }

        s.remaining_capacity = s.total_capacity - s.used_capacity;

        return s;
    }

    static void raise_invalid_buffer_exception(char const *message) {
        throw std::system_error(std::error_code{ ERROR_INVALID_PARAMETER , std::system_category() }, message);
    }

    //
    // pointer to the beginninng of buffer
    // and first element if we have any
    //
    char *buffer_begin_{ nullptr };
    char *last_element_{ nullptr };
    char *buffer_end_{ nullptr };
};

template <typename T,
          typename TT,
          typename A>
void swap(flat_forward_list<T, TT, A> &lhs, flat_forward_list<T, TT, A> &rhs) 
            noexcept (std::allocator_traits<A>::propagate_on_container_swap::value ||
                      std::allocator_traits<A>::propagate_on_container_move_assignment::value) {
    lhs.swap(rhs);
}

//
// Use this typedef if you want to use container with polimorfic allocator
//
template <typename T,
          typename TT = flat_forward_list_traits<T>>
 using pmr_flat_forward_list = flat_forward_list<T, 
                                                 TT, 
                                                 std::pmr::polymorphic_allocator<char>>;

//
// This callss can be used with Polimorfic Memory Allocator.
// Forward Linked List has typedef for PMR called pmr_flat_forward_list.
//
// Sample usage:
// iffl::debug_memory_resource dbg_memory_resource;
// iffl::pmr_flat_forward_list<FLAT_FORWARD_LIST_TEST> ffl{ &dbg_memory_resource };
// Code should guarantee that memory resource is destroyed after all containers that 
// use it are destroyed.
//
// This calss provides some basic buffer overrun and underrun detection, and
// also diring destruction checks that we did not leak any allocated blocks.
//
//   |---------------------|-------------------|----------------------------------|------------|----------------------------------|
//   sizeof(size_t) bytes  pointer to          sizeof(size_t) prefix               user data       sizeof(size_t) suffix
//   offset to the end of  memory resource     0xBEEFABCD - allocated block        filled with     0xDEADABCD - allocated block
//   allocation                                0xBEEFABCE - deallocated block      0xFE            0xDEADABCE - deallocated block
//
// On deallocation it checks that prefix and suffix contain expected pattern
// On allocation user buffer is filled with 0xFE to help detecting use of uninitialized memory
//
class debug_memory_resource 
    : public std::pmr::memory_resource {

public:

    debug_memory_resource() = default;

    ~debug_memory_resource() {
        validate_no_busy_blocks();
    }

    size_t get_busy_blocks_count() const {
        return busy_blocks_count_.load(std::memory_order_relaxed);
    }

    void validate_no_busy_blocks() const {
        FFL_CODDING_ERROR_IF(0 < get_busy_blocks_count());
    }

private:

    using pattern_type = size_t;   

    struct prefix_t {
        size_t size;
        size_t alignment;
        debug_memory_resource *memory_resource;
        pattern_type pattern;
    };

    struct suffix_t {
        pattern_type pattern;
    };

    constexpr static size_t min_allocation_size{ sizeof(prefix_t) + sizeof(suffix_t) };

    constexpr static pattern_type busy_block_prefix_pattern{ 0xBEEFABCD };
    constexpr static pattern_type free_block_prefix_pattern{ 0xBEEFABCE };
    constexpr static pattern_type busy_block_suffix_pattern{ 0xDEADABCD };
    constexpr static pattern_type free_block_suffix_pattern{ 0xDEADABCE };

    constexpr static int fill_pattern{ 0xFE };

    void* do_allocate(size_t bytes, size_t alignment) override {

        size_t bytes_with_debug_info = bytes + min_allocation_size;
        void *ptr = _aligned_malloc(bytes_with_debug_info, alignment);
        if (nullptr == ptr) {
            throw std::bad_alloc{};
        }

        prefix_t *prefix{ reinterpret_cast<prefix_t *>(ptr) };
        suffix_t *suffix{ reinterpret_cast<suffix_t *>( reinterpret_cast<char *>(ptr) + bytes_with_debug_info) - 1 };

        prefix->size = bytes_with_debug_info;
        prefix->alignment = alignment;
        prefix->memory_resource = this;
        prefix->pattern = busy_block_prefix_pattern;
        suffix->pattern = busy_block_suffix_pattern;

        increment_busy_block_count();

        char *user_buffer = reinterpret_cast<char *>(prefix + 1);
        fill_buffer(user_buffer, fill_pattern, bytes);

        return user_buffer;
    }

    void do_deallocate(void* p, size_t bytes, size_t alignment) noexcept override {

        decrement_busy_block_count();

        prefix_t *prefix{ reinterpret_cast<prefix_t *>(p) - 1 };
        FFL_CODDING_ERROR_IF_NOT(prefix->pattern == busy_block_prefix_pattern);
        FFL_CODDING_ERROR_IF_NOT(prefix->memory_resource == this);
        FFL_CODDING_ERROR_IF_NOT(prefix->size > min_allocation_size);
        size_t user_allocation_size = prefix->size - min_allocation_size;
        FFL_CODDING_ERROR_IF_NOT(user_allocation_size == bytes);
        FFL_CODDING_ERROR_IF_NOT(prefix->alignment == alignment);
        suffix_t *suffix{ reinterpret_cast<suffix_t *>(reinterpret_cast<char *>(prefix) + prefix->size) - 1 };
        FFL_CODDING_ERROR_IF_NOT(suffix->pattern == busy_block_suffix_pattern);
        prefix->pattern = free_block_prefix_pattern;
        suffix->pattern = free_block_suffix_pattern;

        _aligned_free(prefix);
    }

    bool do_is_equal(memory_resource const & other) const noexcept override {
        return (&other == static_cast<memory_resource const *>(this));
    }

    void increment_busy_block_count() {
        size_t prev_busy_blocks_count = busy_blocks_count_.fetch_add(1, std::memory_order_relaxed);
        FFL_CODDING_ERROR_IF_NOT(prev_busy_blocks_count >= 0);
    }

    void decrement_busy_block_count() {
        size_t prev_busy_blocks_count = busy_blocks_count_.fetch_sub(1, std::memory_order_relaxed);
        FFL_CODDING_ERROR_IF_NOT(prev_busy_blocks_count > 0);
    }

    //bool enable_detecting
    std::atomic<size_t> busy_blocks_count_{ 0 };
};

} // namespace iffl

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
