#pragma once
//!
//! @file iffl_list.h
//!
//! @author Vladimir Petter
//!
//! [iffl github](https://github.com/vladp72/iffl)
//!
//! @brief Implements intrusive flat forward list.
//!
//! @details
//!
//! This container is designed for POD types with 
//! following general structure: 
//! @code
//!                      ------------------------------------------------------------
//!                      |                                                          |
//!                      |                                                          V
//! | <fields> | offset to next element | <offsets of data> | [data] | [padding] || [next element] ...
//! |                        header                         | [data] | [padding] || [next element] ...
//! @endcode
//!
//! Examples:
//!
//! [FILE_FULL_EA_INFORMATION](https://docs.microsoft.com/en-us/windows-hardware/drivers/ddi/content/wdm/ns-wdm-_file_full_ea_information)
//! @code
//! typedef struct _FILE_FULL_EA_INFORMATION {
//!     ULONG  NextEntryOffset;
//!     UCHAR  Flags;
//!     UCHAR  EaNameLength;
//!     USHORT EaValueLength;
//!     CHAR   EaName[1];
//! } FILE_FULL_EA_INFORMATION, *PFILE_FULL_EA_INFORMATION;
//!
//! @endcode
//! [FILE_NOTIFY_EXTENDED_INFORMATION](https://docs.microsoft.com/en-us/windows/desktop/api/winnt/ns-winnt-_file_notify_extended_information)
//! @code
//! typedef struct _FILE_NOTIFY_EXTENDED_INFORMATION {
//!     DWORD         NextEntryOffset;
//!     DWORD         Action;
//!     LARGE_INTEGER CreationTime;
//!     LARGE_INTEGER LastModificationTime;
//!     LARGE_INTEGER LastChangeTime;
//!     LARGE_INTEGER LastAccessTime;
//!     LARGE_INTEGER AllocatedLength;
//!     LARGE_INTEGER FileSize;
//!     DWORD         FileAttributes;
//!     DWORD         ReparsePointTag;
//!     LARGE_INTEGER FileId;
//!     LARGE_INTEGER ParentFileId;
//!     DWORD         FileNameLength;
//!     WCHAR         FileName[1];
//! } FILE_NOTIFY_EXTENDED_INFORMATION, *PFILE_NOTIFY_EXTENDED_INFORMATION;
//!
//! @endcode
//! [FILE_INFO_BY_HANDLE_CLASS](https://msdn.microsoft.com/en-us/8f02e824-ca41-48c1-a5e8-5b12d81886b5)
//!   output for the following information classes
//! @code
//!        FileIdBothDirectoryInfo
//!        FileIdBothDirectoryRestartInfo
//!        FileIoPriorityHintInfo
//!        FileRemoteProtocolInfo
//!        FileFullDirectoryInfo
//!        FileFullDirectoryRestartInfo
//!        FileStorageInfo
//!        FileAlignmentInfo
//!        FileIdInfo
//!        FileIdExtdDirectoryInfo
//!        FileIdExtdDirectoryRestartInfo
//! @endcode
//!
//! Output of [NtQueryDirectoryFile](https://docs.microsoft.com/en-us/windows-hardware/drivers/ddi/content/ntifs/ns-ntifs-_file_both_dir_information)
//! @code
//! typedef struct _FILE_BOTH_DIR_INFORMATION {
//!     ULONG         NextEntryOffset;
//!     ULONG         FileIndex;
//!     LARGE_INTEGER CreationTime;
//!     LARGE_INTEGER LastAccessTime;
//!     LARGE_INTEGER LastWriteTime;
//!     LARGE_INTEGER ChangeTime;
//!     LARGE_INTEGER EndOfFile;
//!     LARGE_INTEGER AllocationSize;
//!     ULONG         FileAttributes;
//!     ULONG         FileNameLength;
//!     ULONG         EaSize;
//!     CCHAR         ShortNameLength;
//!     WCHAR         ShortName[12];
//!     WCHAR         FileName[1];
//! } FILE_BOTH_DIR_INFORMATION, *PFILE_BOTH_DIR_INFORMATION;
//! @endcode
//!
//! Or types that do not have next element offset, but it can be calculated.
//!
//! Offset of the next element is size of this element data, plus optional padding to keep
//! next element propertly alligned
//! @code
//!                      -----------------------------------
//!                      |                                 |
//!     (next element offset = sizeof(this element)        |
//!                      |   + padding)                    |
//!                      |                                 V
//! | <fields> | <offsets of data> | [data] | [padding] || [next element] ...
//! |       header                 | [data] | [padding] || [next element] ...
//! @endcode
//! Exanmples:
//!
//! CLUSPROP_SYNTAX
//!   [property list](https://docs.microsoft.com/en-us/previous-versions/windows/desktop/mscs/property-lists)
//!
//!   [data structures](https://docs.microsoft.com/en-us/previous-versions/windows/desktop/mscs/data-structures)
//!
//!   [cluster property syntax](https://docs.microsoft.com/en-us/windows/desktop/api/clusapi/ns-clusapi-clusprop_syntax)
//!
//! @code
//! typedef union CLUSPROP_SYNTAX {
//!   DWORD  dw;
//!   struct {
//!       WORD wFormat;
//!       WORD wType;
//!   } DUMMYSTRUCTNAME;
//! } CLUSPROP_SYNTAX;
//! @endcode
//!
//! [CLUSPROP_VALUE](https://docs.microsoft.com/en-us/windows/desktop/api/clusapi/ns-clusapi-clusprop_value)
//! @code
//! typedef struct CLUSPROP_VALUE {
//!     CLUSPROP_SYNTAX Syntax;
//!     DWORD           cbLength;
//! } CLUSPROP_VALUE;
//! @endcode
//!
//! This module implements 
//!
//!   function flat_forward_list_validate that 
//!   can be use to deal with untrusted buffers.
//!   You can use it to validate if untrusted buffer 
//!   contains a valid list, and to find entry at 
//!   which list gets invalid.
//!   It also can be used to enumerate over validated
//!   elements.
//!
//!   flat_forward_list_iterator and flat_forward_list_const_iterator 
//!   that can be used to enumerate over previously validated buffer.
//!
//!   flat_forward_list a container that provides a set of helper 
//!   algorithms and manages buffer lifetime while list changes.
//!
//!   pmr_flat_forward_list is a an aliase of flat_forward_list
//!   where allocator is polimorfic_allocator.
//!
//! Interface that user have to implement for a type that will be stored in the container:
//!
//!  Since we are implementing intrusive container, user have to give us a 
//!  helper class that implements folowing methods 
//!   - Specifies alignment requirements for the type
//!          @code constexpr static size_t const alignment{ TYPE_ALIGNMENT }; @endcode 
//!   - tell minimum required size am element must have to be able to query element size
//!          @code constexpr static size_t minimum_size() noexcept @endcode 
//!   - query offset to next element 
//!          @code  constexpr static size_t get_next_offset(char const *buffer) noexcept @endcode 
//!   - update offset to the next element
//!          @code  constexpr static void set_next_offset(char *buffer, size_t size) noexcept @endcode 
//!   - calculate element size from data
//!          @code  constexpr static size_t get_size(char const *buffer) noexcept @endcode 
//!   - validate that data fit into the buffer
//!          @code  constexpr static bool validate(size_t buffer_size, char const *buffer) noexcept @endcode 
//!
//!  By default algorithms and conteiners in this library are looking for specialization 
//!  of flat_forward_list_traits for the element type.
//!  For example:
//! @code
//!    namespace iffl {
//!        template <>
//!        struct flat_forward_list_traits<FLAT_FORWARD_LIST_TEST> {
//!            constexpr static size_t const alignment{ TYPE_ALIGNMENT };
//!            constexpr static size_t minimum_size() noexcept { <implementation> }
//!            constexpr static size_t get_next_offset(char const *buffer) noexcept { <implementation> }
//!            constexpr static void set_next_offset(char *buffer, size_t size) noexcept { <implementation> }
//!            constexpr static size_t get_size(char const *buffer) noexcept { <implementation> }
//!            constexpr static bool validate(size_t buffer_size, char const *buffer) noexcept {<implementation>}
//!        };
//!    }
//!  @endcode
//!
//!   for sample implementation see flat_forward_list_traits<FLAT_FORWARD_LIST_TEST> @ test\iffl_test_cases.cpp
//!   and addition documetation in this mode right before primary
//!   template for flat_forward_list_traits definition
//!
//!   If picking traits using partial specialization is not preferable then traits can be passed as
//!   an explicit template parameter. For example:
//! @code
//!   using ffl_iterator = iffl::flat_forward_list_iterator<FLAT_FORWARD_LIST_TEST, my_traits>;
//! @endcode
//! Debugging:
//!
//!   if you define FFL_DBG_CHECK_DATA_VALID then every method
//!   that modifies data in container will call flat_forward_list_validate
//!   at the end and makes sure that data are valid, and last element is
//!   where container believes it should be.
//!   Cost O(n).
//!
//!   If you define FFL_DBG_CHECK_ITERATOR_VALID then every input 
//!   iterator will be cheked to match to valid element in the container.
//!   Cost O(n)
//!
//!   You can use debug_memory_resource and pmr_flat_forward_list to validate
//!   that all allocations are freed and that there are no buffer overruns/underruns
//!

#include <iffl_config.h>
#include <iffl_common.h>
#include <iffl_mpl.h>
//!
//! @namespace iffl::mpl
//! @brief intrusive flat forward list
//!
namespace iffl {
//!
//! @class flat_forward_list_traits
//! @brief traits for an elements that are in the flat forward list
//! @details
//! Flat forward list element types are PODs defined in the headers that
//! we often times cannot change, like OS headers, but we need to be able
//! to define methods that would help container operate on them. For instance
//! we need a universal way how to find offset of the next element regardless
//! of how it is calculated for a particular type. Spacialization of this calss
//! provides a group of functions that help with that. User of the collection 
//! is responsible for defining traits for the type he wants to use in the
//! container.
//!
//! This is the only method required by flat_forward_list_iterator.
//! Returns offset to the next element or 0 if this is the last element.
//!
//! @code
//! constexpr static size_t get_next_offset(char const *buffer) noexcept;
//! @endcode
//!
//! This method is requiered for flat_forward_list_validate algorithm
//! Minimum number of bytes to be able to safely query offset to next 
//! and safely examine other fields.
//!
//! @code
//! constexpr static size_t minimum_size() noexcept;
//! @endcode
//!
//! This method is required for flat_forward_list_validate algorithm
//! Validates that variable data fits in the buffer
//!
//! @code
//! constexpr static bool validate(size_t buffer_size, char const *buffer) noexcept;
//! @endcode
//!
//! This method is used by flat_forward_list container to update offset to the
//! next element. size can be 0 if this element is last or above zero for any other element. 
//!
//! @code
//! constexpr static void set_next_offset(char *buffer, size_t size) noexcept
//! @endcode
//!
//! Specifies alignment requirements for the type. When provided then
//! every newly added element will have proper alignment. Elements that
//! already were in the buffer can be not propertly alligned.
//! If this member does not exist then alligned is assumed to be 1 byte
//!
//! @code 
//! constexpr static size_t const alignment{ TYPE_ALIGNMENT };
//! @endcode 
//!
//! This method is used by flat_forward_list. It calculates size of element, but it should
//! not use next element offset, and instead it should calculate size based on the data this 
//! element contains. It is used when we append new element to the container, and need to
//! update offset to the next on the element that used to be last. In that case offset to 
//! the next is determined by calling this method. 
//! Another example that uses this method is shrink_to_fit.
//!
//! @code
//! constexpr static size_t get_size(char const *buffer) noexcept 
//! @endcode
//!
//! Examples:
//! flat_forward_list_traits<FLAT_FORWARD_LIST_TEST> @ test\iffl_tests.cpp
//! flat_forward_list_traits<FILE_FULL_EA_INFORMATION> @ test\iffl_es_usecase.cpp
//! flat_forward_list_traits<long_long_array_list_entry> @ test\iffl_c_api_usecase.cpp
//!
template <typename T>
struct flat_forward_list_traits;

//!
//! @class flat_forward_list_traits_traits
//! @brief traits for flat_forward_list_traits
//! @tparam T - element header type
//! @tparam TT - type traits
//!
//! @details
//! I know, traits of traits does sounds redicules,
//! but this is exactly what this class is.
//! Given flat_forward_list_traits instantiation,
//! or a class that you want to use as traits for your
//! flat_forward_list element types, it detects what methods
//! are implemented by this trait so in the rest of algorithms
//! and containers we can use this helper, and avoid rewriting
//! same complicated and nusty machinery.
//! In other words traits might be missing some methods, and
//! traits_traits will implement these missing parts providing
//! a uniformal interface.
//!
//! How to use:
//!
//! @code
//! using my_traits_traits = flat_forward_list_traits_traits<my_type, my_traits>;
//! @endcode
//!
//! For example:
//!
//! If traits provide us a way to get value of the next element offset for a type
//! then use it, otherwise ask it to calculate next element offset from its own
//! size
//! If traits provides alignment member then we will use in calculation of padding, 
//! otherwise calculations of padding will use 1 byte alignment. 
//!
//! @code
//! if constexpr (my_traits_traits::has_next_offset_v) {
//!      my_traits::get_next_offset(buffer)
//! } else {
//!      my_traits::get_size(buffer)
//! }
//! @endcode
//!
template < typename T,
           typename TT = flat_forward_list_traits<T>>
struct flat_forward_list_traits_traits {

private:
    //
    // Metafunctions that we will use with detect-idiom to find
    // properties of TT without triggering a compile time error
    //
    //!
    //! @typedef has_minimum_size_mfn
    //! @tparam P - type we will evaluate this metafunction for. 
    //!             Traits type should be used here.
    //! !brief Metafunction that detects if traits include minimum_size
    //!
    template <typename P>
    using has_minimum_size_mfn = decltype(std::declval<P &>().minimum_size()); 
    //!
    //! @typedef has_get_size_mfn
    //! @tparam P - type we will evaluate this metafunction for. 
    //!             Traits type should be used here.
    //! !brief Metafunction that detects if traits include get_size
    //!
    template <typename P>
    using has_get_size_mfn = decltype(std::declval<P &>().get_size(std::declval<T const &>()));
    //!
    //! @typedef has_next_offset_mfn
    //! @tparam P - type we will evaluate this metafunction for. 
    //!             Traits type should be used here.
    //! !brief Metafunction that detects if traits include get_next_offset
    //!
    template <typename P>
    using has_next_offset_mfn = decltype(std::declval<P &>().get_next_offset(std::declval<T const &>()));
    //!
    //! @typedef can_set_next_offset_mfn
    //! @tparam P - type we will evaluate this metafunction for. 
    //!             Traits type should be used here.
    //! !brief Metafunction that detects if traits include set_next_offset
    //!
    template <typename P>
    using can_set_next_offset_mfn = decltype(std::declval<P &>().set_next_offset(std::declval<T &>(), size_t{ 0 }));
    //!
    //! @typedef can_validate_mfn
    //! @tparam P - type we will evaluate this metafunction for. 
    //!             Traits type should be used here.
    //! !brief Metafunction that detects if traits include validate
    //!
    template <typename P>
    using can_validate_mfn = decltype(std::declval<P &>().validate(size_t{0}, std::declval<T const &>()));
    //!
    //! @typedef has_alignment_mfn
    //! @tparam P - type we will evaluate this metafunction for. 
    //!             Traits type should be used here.
    //! !brief Metafunction that detects if traits define function that should be used to
    //! pad next element offset
    //!
    template <typename P>
    using has_alignment_mfn = decltype(std::declval<P &>().alignment);

public:
    //!
    //! @typedef value_type
    //! !brief alias for the value type
    //!
    using value_type = T;
    //!
    //! @typedef type_traits
    //! !brief alias for the traits
    //!
    using type_traits = TT;
    //!
    //! @typedef has_minimum_size_t
    //! @brief Uses detect idiom with has_minimum_size_mfn to
    //! find if traits implemented minimum_size
    //! @details If traits have minimum_size then 
    //! has_minimum_size_t is std::true_type otherwise std::false_type
    //!
    using has_minimum_size_t = iffl::mpl::is_detected < has_minimum_size_mfn, type_traits>;
    //!
    //! @brief Instance of has_minimum_size_t 
    //! @details has_minimum_size_v is std::true_type{} otherwise std::false_type{}
    //!
    constexpr static auto const has_minimum_size_v{ iffl::mpl::is_detected_v < has_minimum_size_mfn, type_traits> };
    //!
    //! @typedef has_get_size_t
    //! @brief Uses detect idiom with has_get_size_mfn to
    //! find if traits implemented get_size
    //! @details If traits have get_size then 
    //! has_get_size_t is std::true_type otherwise std::false_type
    //!
    using has_get_size_t = iffl::mpl::is_detected < has_get_size_mfn, type_traits>;
    //!
    //! @brief Instance of has_get_size_t 
    //! @details has_get_size_v is std::true_type{} otherwise std::false_type{}
    //!
    constexpr static auto const has_get_size_v{ iffl::mpl::is_detected_v < has_get_size_mfn, type_traits> };
    //!
    //! @typedef has_next_offset_t
    //! @brief Uses detect idiom with has_next_offset_mfn to
    //! find if traits implemented get_next_offset
    //! @details If traits have get_next_offset then 
    //! has_next_offset_t is std::true_type otherwise std::false_type
    //!
    using has_next_offset_t = iffl::mpl::is_detected < has_next_offset_mfn, type_traits>;
    //!
    //! @brief Instance of has_next_offset_t 
    //! @details has_next_offset_v is std::true_type{} otherwise std::false_type{}
    //!
    constexpr static auto const has_next_offset_v{ iffl::mpl::is_detected_v < has_next_offset_mfn, type_traits> };
    //!
    //! @typedef can_set_next_offset_t
    //! @brief Uses detect idiom with can_set_next_offset_mfn to
    //! find if traits implemented set_next_offset
    //! @details If traits have get_next_offset then 
    //! can_set_next_offset_t is std::true_type otherwise std::false_type
    //!
    using can_set_next_offset_t = iffl::mpl::is_detected < can_set_next_offset_mfn, type_traits>;
    //!
    //! @brief Instance of can_set_next_offset_t 
    //! @details can_set_next_offset_v is std::true_type{} otherwise std::false_type{}
    //!
    constexpr static auto const can_set_next_offset_v{ iffl::mpl::is_detected_v < can_set_next_offset_mfn, type_traits> };
    //!
    //! @typedef can_validate_t
    //! @brief Uses detect idiom with can_validate_mfn to
    //! find if traits implemented validate
    //! @details If traits have get_next_offset then 
    //! can_validate_t is std::true_type otherwise std::false_type
    //!
    using can_validate_t = iffl::mpl::is_detected < can_validate_mfn, type_traits>;
    //!
    //! @brief Instance of can_validate_t 
    //! @details can_validate_v is std::true_type{} otherwise std::false_type{}
    //!
    constexpr static auto const can_validate_v{ iffl::mpl::is_detected_v < can_validate_mfn, type_traits> };
    //!
    //! @typedef has_alignment_t
    //! @brief Uses detect idiom with has_alignment_mfn to
    //! find if traits implemented validate
    //! @details If traits have traits::alignment static variable then 
    //! has_alignment_t is std::true_type otherwise std::false_type
    //!
    using has_alignment_t = iffl::mpl::is_detected < has_alignment_mfn, type_traits>;
    //!
    //! @brief Instance of has_alignment_t 
    //! @details has_alignment_v is std::true_type{} otherwise std::false_type{}
    //!
    constexpr static auto const has_alignment_v{ iffl::mpl::is_detected_v < has_alignment_mfn, type_traits> };
    //!
    //! @brief Casts buffer pointer to a pointer to the element type
    //! @param ptr - pointer to a buffer
    //!
    static value_type* ptr_to_t(void *ptr) {
        return static_cast<value_type *>(ptr);
    }
    //!
    //! @brief Casts buffer pointer to a pointer to the element type
    //! @param ptr - pointer to a buffer
    //!
    static value_type const* ptr_to_t(void const *ptr) {
        return static_cast<value_type const *>(ptr);
    }
    //!
    //! @brief Casts buffer pointer to a pointer to the element type
    //! @param ptr - pointer to a buffer
    //!
    static value_type* ptr_to_t(char *ptr) {
        return reinterpret_cast<value_type *>(ptr);
    }
    //!
    //! @brief Casts buffer pointer to a pointer to the element type
    //! @param ptr - pointer to a buffer
    //!
    static value_type const* ptr_to_t(char const *ptr) {
        return reinterpret_cast<value_type const *>(ptr);
    }
    //!
    //! @brief Casts buffer pointer to a pointer to the element type
    //! @param ptr - pointer to a buffer
    //!
    static value_type* ptr_to_t(unsigned char *ptr) {
        return reinterpret_cast<value_type *>(ptr);
    }
    //!
    //! @brief Casts buffer pointer to a pointer to the element type
    //! @param ptr - pointer to a buffer
    //!
    static value_type const* ptr_to_t(unsigned char const *ptr) {
        return reinterpret_cast<value_type const *>(ptr);
    }
    //!
    //! @brief returns minimum valid element size
    //!
    constexpr static size_t minimum_size() noexcept {
        return type_traits::minimum_size();
    }
    //!
    //! @brief If traits defined allignment then size of
    //! alignment, and 1 otherwise
    //!
    constexpr static size_t get_alignment() noexcept {
        if constexpr (has_alignment_v) {
            return type_traits::alignment;
        } else {
            return 1;
        }
    }
    //!
    //! @brief Defines static constexpr member that
    //! with value that has alignment requirements of elements
    //!
    constexpr static size_t const alignment{ get_alignment() };
    //!
    //! @typedef range_t
    //! @brief Specialization of range_with_alighment fo this type alignment
    //!
    using range_t = range_with_alighment<alignment>;
    //!
    //! @typedef size_with_padding_t
    //! @brief Specialization of size_with_padding fo this type alignment
    //!
    using size_with_padding_t = size_with_padding<alignment>;
    //!
    //! @typedef offset_with_aligment_t
    //! @brief Specialization of offset_with_aligment fo this type alignment
    //!
    using offset_with_aligment_t = offset_with_aligment<alignment>;
    //!
    //! @brief If traits defined allignment then s padded
    //! to alignment, or unchanged value of s otherwise
    //! @param s - value that we are rounding up
    //! @return Value of s padded to alignment
    //!
    constexpr static size_t roundup_to_alignment(size_t s) noexcept {
        if constexpr (has_alignment_v && 0 != alignment) {
            return roundup_size_to_alignment(s, alignment);
        } else {
            return s;
        }
    }
    //!
    //! @brief Asks type traits to calculate element size
    //! @param buffer - pointer to the begining of the element
    //! @return element size wrapped into size_with_padding_t
    //!
    constexpr static size_with_padding_t get_size(char const *buffer) noexcept {
        return size_with_padding_t{ type_traits::get_size(*ptr_to_t(buffer)) };
    }
    //!
    //! @brief Asks type traits to validate element. 
    //! @details If traits do not have validate method
    //! @param buffer_size - size of the buffer used by this element
    //! @param buffer - pointer to the begining of the element
    //! @return element size wrapped into size_with_padding_t
    //!
    constexpr static bool validate(size_t buffer_size, T const &buffer) noexcept {
        if constexpr (can_validate_v) {
            return type_traits::validate(buffer_size, buffer);
        } else {
            return true;
        }
    }
    //!
    //! @brief Returns next element offset. 
    //! @param buffer - pointer to the begining of the element
    //! @return For the types that support query for the next element offset
    //! method returns get_next_offset, otherwise it returns element size
    //!
    constexpr static size_t get_next_offset(char const *buffer) noexcept {
        if constexpr (has_next_offset_v) {
            return type_traits::get_next_offset(*ptr_to_t(buffer));
        } else {
            size_with_padding_t s{ get_size(buffer) };
            return s.size_padded;
        }
    }
    //!
    //! @brief Sets next element offset. 
    //! @details Types that support setting next element offset
    //! must also support get_next_offset.
    //! @param buffer - pointer to the begining of the element
    //! @param size - offset of the next element
    //!
    constexpr static void set_next_offset(char *buffer, size_t size) noexcept {
        static_assert(has_next_offset_v,
                      "set_next_offset is not supported for type that does not have get_next_offset");
        if constexpr (has_alignment_v) {
            //
            // If traits specify alignment requirements then
            // assert when attempt to set unaligned offset to 
            // next element 
            //
            FFL_CODDING_ERROR_IF_NOT(size == roundup_to_alignment(size));
        }
        return type_traits::set_next_offset(*ptr_to_t(buffer), size);
    }
    //!
    //! @brief Prints information about what traits_traits discovered
    //! about traits class. 
    //!
    static void print_traits_info() noexcept {
        std::type_info const & ti = typeid(type_traits&);

        std::printf("type \"%s\" {\n", ti.name());

        if constexpr (has_minimum_size_v) {
            std::printf("  minimum_size    : yes -> %zu\n", minimum_size());
        } else {
            std::printf("  minimum_size    : no \n");
        }

        if constexpr (has_get_size_v) {
            std::printf("  get_size        : yes\n");
        } else {
            std::printf("  get_size        : no \n");
        }

        if constexpr (has_next_offset_v) {
            std::printf("  get_next_offset : yes\n");
        } else {
            std::printf("  get_next_offset : no \n");
        }

        if constexpr (can_set_next_offset_v) {
            std::printf("  set_next_offset : yes\n");
        } else {
            std::printf("  set_next_offset : no \n");
        }

        if constexpr (can_validate_v) {
            std::printf("  validate        : yes\n");
        } else {
            std::printf("  validate        : no \n");
        }

        if constexpr (has_alignment_v) {
            std::printf("  alignment       : yes -> %zu\n", alignment);
        } else {
            std::printf("  alignment       : no \n");
        }
        std::printf("}\n");
    }
};


//!
//! @details Forward declaration
//! of intrusive flat forward list container.
//!
template <typename T,
          typename TT,
          typename A>
class flat_forward_list;
//!
//! @details Forward declaration
//! of intrusive flat forward list reference.
//!
template <typename T,
          typename TT>
class flat_forward_list_ref;

//!
//! @class default_validate_element_fn
//! @tparam T - element type
//! @tparam TT - element type traits. Detaulted to 
//! specialization flat_forward_list_traits<T>
//! @details
//! Calls flat_forward_list_traits::validate(...)
//!
template<typename T,
         typename TT = flat_forward_list_traits<T>>
struct default_validate_element_fn {
    //!
    //! @brief Function call iterator that validates element
    //! @param buffer_size - size of the buffer used by the element
    //! @param e - pointer to the buffer used by the element
    //!
    bool operator() (size_t buffer_size, 
                     T const &e) const noexcept {
        return TT::validate(buffer_size, e);
    }
};
//!
//! @class noop_validate_element_fn
//! @details Does nothing
//!
template<typename T,
         typename TT = flat_forward_list_traits<T>>
struct noop_validate_element_fn {
    //!
    //! @brief Function call iterator that validates element
    //! @details This functor noops validation 
    //!
    bool operator() ([[maybe_unused]] size_t, 
                     [[maybe_unused]] T const &) const noexcept {
        return true;
    }
};
//!
//! @brief Forward declaration
//!
template<typename T,
         typename TT = flat_forward_list_traits<T>,
         typename F = default_validate_element_fn<T, TT>>
constexpr inline std::pair<bool, flat_forward_list_ref<T, TT>> flat_forward_list_validate_has_next_offset(char const *first,
                                                                                                          char const *end,
                                                                                                          F const &validate_element_fn = default_validate_element_fn<T, TT>{}) noexcept;
//!
//! @brief Forward declaration
//!
template<typename T,
         typename TT = flat_forward_list_traits<T>,
         typename F = default_validate_element_fn<T, TT>>
constexpr inline std::pair<bool, flat_forward_list_ref<T, TT>> flat_forward_list_validate_no_next_offset(char const *first,
                                                                                                         char const *end,
                                                                                                         F const &validate_element_fn = default_validate_element_fn<T, TT>{}) noexcept;
//!
//! @brief Forward declaration
//!
template<typename T,
         typename TT = flat_forward_list_traits<T>,
         typename F = default_validate_element_fn<T, TT>>
constexpr inline std::pair<bool, flat_forward_list_ref<T, TT>> flat_forward_list_validate(char const *first,
                                                                                          char const *end, 
                                                                                          F const &validate_element_fn = default_validate_element_fn<T, TT>{}) noexcept;
//!
//! @brief Forward declaration
//!
template<typename T,
         typename TT = flat_forward_list_traits<T>,
         typename F = default_validate_element_fn<T, TT>>
constexpr inline std::pair<bool, flat_forward_list_ref<T, TT>> flat_forward_list_validate(char *first,
                                                                                          char *end,
                                                                                          F const &validate_element_fn = default_validate_element_fn<T, TT>{}) noexcept;
//!
//! @brief Forward declaration
//!
template<typename T,
         typename TT = flat_forward_list_traits<T>,
         typename F = default_validate_element_fn<T, TT>>
constexpr inline std::pair<bool, flat_forward_list_ref<T, TT>> flat_forward_list_validate(T *first,
                                                                                          T *end,
                                                                                          F const &validate_element_fn = default_validate_element_fn<T, TT>{}) noexcept;
//!
//! @brief Forward declaration
//!
template<typename T,
         typename TT = flat_forward_list_traits<T>,
         typename F = default_validate_element_fn<T, TT>>
constexpr inline std::pair<bool, flat_forward_list_ref<T, TT>> flat_forward_list_validate(T const *first,
                                                                                          T const *end,
                                                                                          F const &validate_element_fn = default_validate_element_fn<T, TT>{}) noexcept;
//!
//! @brief Forward declaration
//!
template<typename T,
         typename TT = flat_forward_list_traits<T>,
         typename F = default_validate_element_fn<T, TT>>
constexpr inline std::pair<bool, flat_forward_list_ref<T, TT>> flat_forward_list_validate(unsigned char const *first,
                                                                                          unsigned char const *end,
                                                                                          F const &validate_element_fn = default_validate_element_fn<T, TT>{}) noexcept;
//!
//! @brief Forward declaration
//!
template<typename T,
         typename TT = flat_forward_list_traits<T>,
         typename F = default_validate_element_fn<T, TT>>
constexpr inline std::pair<bool, flat_forward_list_ref<T, TT>> flat_forward_list_validate(unsigned char *first,
                                                                                          unsigned char *end,
                                                                                          F const &validate_element_fn = default_validate_element_fn<T, TT>{}) noexcept;
//!
//! @brief Forward declaration
//!
template<typename T,
         typename TT = flat_forward_list_traits<T>,
         typename F = default_validate_element_fn<T, TT>>
constexpr inline std::pair<bool, flat_forward_list_ref<T, TT>> flat_forward_list_validate(void const *first,
                                                                                          void const *end,
                                                                                          F const &validate_element_fn = default_validate_element_fn<T, TT>{}) noexcept;
//!
//! @brief Forward declaration
//!
template<typename T,
         typename TT = flat_forward_list_traits<T>,
         typename F = default_validate_element_fn<T, TT>>
constexpr inline std::pair<bool, flat_forward_list_ref<T, TT>> flat_forward_list_validate(void *first,
                                                                                          void *end,
                                                                                          F const &validate_element_fn = default_validate_element_fn<T, TT>{}) noexcept;

//!
//! @class flat_forward_list_iterator_t
//! @brief THis class implements forward iterator for intrusive flat forward list
//! @tparam T  - type of element header
//! @tparam TT - type trait for T that is used to get offset to the
//!      next element in the flat list. It must implement
//!      get_next_offset method.
//!
//! @details Once iterator is incremented pass last element it becomes equal
//! to default initialized iterator so you can use
//! flat_forward_list_iterator_t{} as a sentinel that can be used as an
//! end iterator.
//!
template<typename T,
         typename TT = flat_forward_list_traits<T>>
class flat_forward_list_iterator_t final {

    //!
    //! @details Forward declaration
    //! of intrusive flat forward list container.
    //!
    template <typename TU,
              typename TTU,
              typename AU>
    friend class flat_forward_list;

    //!
    //! @details Forward declaration
    //! of intrusive flat forward list reference.
    //!
    template <typename TU,
              typename TTU>
    friend class flat_forward_list_ref;

public:
    //!
    //! @typedef iterator_category
    //! @brief Marks iterator as a forward iterator
    //!
    using iterator_category = std::forward_iterator_tag;
    //!
    //! @typedef value_type
    //! @brief Element value type
    //!
    using value_type = T;
    //!
    //! @typedef difference_type
    //! @brief Element pointers difference type
    //!
    using difference_type = ptrdiff_t;
    //!
    //! @typedef pointer
    //! @brief Pointer to element type
    //!
    using pointer = T*;
    //!
    //! @typedef reference
    //! @brief Reference to the element type
    //!
    using reference = T&;
    //!
    //! @typedef traits
    //! @brief Element traits type
    //!
    using traits = TT;
    //!
    //! @typedef traits_traits
    //! @brief Element traits traits type
    //!
    using traits_traits = flat_forward_list_traits_traits<T, TT>;
    //!
    //! @typedef buffer_char_pointer
    //! @details Selects between constant and non-constant pointer to the buffer
    //! depending if T is const
    //!
    using buffer_char_pointer = std::conditional_t<std::is_const_v<T>, char const *, char *>;
    //!
    //! @typedef buffer_unsigned_char_pointer
    //! @details Selects between constant and non-constant pointer to the buffer
    //! depending if T is const
    //!
    using buffer_unsigned_char_pointer = std::conditional_t<std::is_const_v<T>, unsigned char const *, unsigned char *>;
    //!
    //! @typedef buffer_void_pointer
    //! @details Selects between constant and non-constant pointer to the buffer
    //! depending if T is const
    //!
    using buffer_void_pointer = std::conditional_t<std::is_const_v<T>, void const *, void *>;
    //!
    //! @typedef size_with_padding_t
    //! @brief Vocabulary type used to describe size with padding
    //! 
    using size_with_padding_t = typename traits_traits::size_with_padding_t;
    //!
    //! @brief Default initialize iterator.
    //! @details For the types that support get_next_offset
    //! default initialized iterator is an end iterator.
    //! For the types that do not support get_next_offset
    //! it creates an invalid iterator.
    //!
    constexpr flat_forward_list_iterator_t() noexcept = default;
    //!
    //! @brief Copy constructs an iterator
    //!
    constexpr flat_forward_list_iterator_t(flat_forward_list_iterator_t const &) noexcept = default;
    //!
    //! @brief Move constructs iterator
    //! @param other - instance of iterator we are moving from
    //!
    constexpr flat_forward_list_iterator_t(flat_forward_list_iterator_t && other) noexcept
        : p_{other.p_} {
        other.p_ = nullptr;
    }
    //!
    //! @typedef const_iterator
    //! @brief Const iterator is an iterator for a T const
    //! @details This type is used as helper for SFINAE expressions
    //!
    using const_iterator = flat_forward_list_iterator_t< std::add_const_t<T>, TT>;
    //!
    //! @typedef non_const_iterator
    //! @brief [Non-const] iterator is an iterator for a T with no const qualifiers
    //! @details This type is used as a helper for SFINAE expressions
    //!
    using non_const_iterator = flat_forward_list_iterator_t< std::remove_cv_t<T>, TT>;
    //!
    //! @brief Is std::true_type{} if this iterator is an iterator for a T const,
    //! and std::false_type{} otherwise
    //!
    constexpr static auto const is_const_iterator{ std::is_const_v<T> };
    //!
    //! @tparam Iterator - any type
    //! @brief Is std::true_type{} if Iterator is same as non-const equivalent of the current iterator
    //! non-const equivalent is constructed by removing CV qualifiers from T.
    //! non_const_iterator defines non-const equivalent for this iterator so we just need
    //! to make sure Iterator is the same type as non_const_iterator
    //!
    template <typename Iterator>
    constexpr static auto const is_non_const_iterator_v{ std::is_same_v < std::remove_cv_t<Iterator>,
                                                                          non_const_iterator > };
    //!
    //! @tparam Iterator - any type
    //! @brief Is std::true_type{} if Iterator is same as const equivalent of the current iterator
    //! non-const equivalent is constructed by adding CV qualifiers to T.
    //! const_iterator defines const equivalent for this iterator so we just need
    //! to make sure Iterator is the same type as const_iterator
    //!
    template <typename Iterator>
    constexpr static auto const is_const_iterator_v{ std::is_same_v < std::remove_cv_t<Iterator>,
                                                                      const_iterator > };
    //!
    //! @tparam Iterator - any type
    //! @brief Is std::true_type{} if Iterator is a const or non-const equivalent of this
    //! iterator and std::false_type otherwise. Comparable iterator can be used in relation
    //! operators
    //!
    template <typename Iterator>
    constexpr static auto const is_comparable_iterator_v{ is_non_const_iterator_v<Iterator> ||
                                                          is_const_iterator_v<Iterator> };
    //!
    //! @brief Unittest that is_non_const_iterator_v returns expected result
    //!
    static_assert(!is_non_const_iterator_v<const_iterator>,
                  "This is a const iterator");
    //!
    //! @brief Unittest that is_non_const_iterator_v returns expected result
    //!
    static_assert(is_non_const_iterator_v<non_const_iterator>,
                  "This is a non-const iterator");

    //!
    //! @brief Copy constructor for const iterator from non-const iterator
    //! @tparam I - iterator type we are constructing from
    //! @param other - iterator we are constructing from
    //! @details We are relying on SFINAE to disable this constructor if
    //! - this is not a const iterator
    //! - I not a non-const iterator
    //!
    template< typename I,
              typename = std::enable_if_t<is_const_iterator && is_non_const_iterator_v<I>>>
    constexpr flat_forward_list_iterator_t(I const & other) noexcept
        : p_{ other.get_ptr() } {
    }   
    //!
    //! @brief Perfect forwarding constructor for const iterator from non-const iterator
    //! @tparam I - iterator type we are constructing from
    //! @param other - iterator we are constructing from
    //! @details We are relying on SFINAE to disable this constructor if
    //! - this is not a const iterator
    //! - I not a non-const iterator
    //! If other is an rvalue then it can play a role of move constructor.
    //! On move we will copy data other interator is pointing to, but we
    //! are not going to null it out. Nulling is not required because this type
    //! is not a RAII wrapper, and there is no harm in leaving other pointing to
    //! the same element.
    //!
    template< typename I,
              typename = std::enable_if_t<is_const_iterator && is_non_const_iterator_v<I>>>
    constexpr flat_forward_list_iterator_t(I && other) noexcept
        : p_{ other.get_ptr() } {
        //other.reset_ptr(nullptr);
    }
    //!
    //! @brief Default generated copy constructor
    //! @returns reference to this object
    //!
    constexpr flat_forward_list_iterator_t &operator= (flat_forward_list_iterator_t const &) noexcept = default;
    //!
    //! @brief Move constructor
    //! @param other - iterator we are moving from
    //! @returns reference to this object
    //!
    constexpr flat_forward_list_iterator_t & operator= (flat_forward_list_iterator_t && other) noexcept {
        if (this != &other) {
            p_ = other.p_;
            other.p_ = nullptr;
        }
        return *this;
    }
    //!
    //! @brief Assignment operator for const iterator from non-const iterator
    //! @tparam I - iterator type we are assigning from
    //! @param other - iterator we are constructing from
    //! @return reference to this object
    //! @details We are relying on SFINAE to disable this assignment operator if
    //! - this is not a const iterator
    //! - I not a non-const iterator
    //!
    template <typename I,
              typename = std::enable_if_t<is_const_iterator && is_non_const_iterator_v<I>>>
    constexpr flat_forward_list_iterator_t & operator= (I const & other) noexcept {
        p_ = other.get_ptr();
        return *this;
    }
    //!
    //! @brief Perfect forwarding assignment operator for const iterator from non-const iterator
    //! @tparam I - iterator type we are assigningg from
    //! @param other - iterator we are assigning from
    //! @return reference to this object
    //! @details We are relying on SFINAE to disable this assignment operator if
    //! - this is not a const iterator
    //! - I not a non-const iterator
    //! If other is an rvalue then it can play a role of move assignment operator.
    //! On move we will copy data other interator is pointing to, but we
    //! are not going to null it out. Nulling is not required because this type
    //! is not a RAII wrapper, and there is no harm in leaving other pointing to
    //! the same element.
    //!
    template <typename I,
              typename = std::enable_if_t<is_const_iterator && is_non_const_iterator_v<I>>>
    constexpr flat_forward_list_iterator_t & operator= (I && other) noexcept {
        p_ = other.get_ptr();
        //other.reset_ptr(nullptr);
        return *this;
    }
    //!
    //! @brief whaps this iterator and the other iterator
    //! @param other - reference to the other iterator
    //!
    constexpr void swap(flat_forward_list_iterator_t & other) noexcept {
        buffer_char_pointer *tmp = p_;
        p_ = other.p_;
        other.p_ = tmp;
    }
    //!
    //! @brief Explicit conversion iterator to bool
    //! @return false if it is null initialized and true otherwise
    //!
    constexpr explicit operator bool() const {
        return p_ != nullptr;
    }
    //!
    //! @brief Equals operator used to compare to another iterator
    //! @tparam I - type of the other iterator
    //! @param other - other iterator we are comparing to
    //! @return true if both iterators point to the 
    //! same element and false otherwise
    //! @details We are using SFINAE to enable this operator
    //! only for const and non-const iterator types for the same 
    //! element type
    //!
    template <typename I,
              typename = std::enable_if_t<is_comparable_iterator_v<I>>>
    constexpr bool operator == (I const &other) const noexcept {
        return p_ == other.get_ptr();
    }
    //!
    //! @brief Not equals operator used to compare to another iterator
    //! @tparam I - type of the other iterator
    //! @param other - other iterator we are comparing to
    //! @return false if both iterators point to the 
    //! same element and true otherwise
    //! @details We are using SFINAE to enable this operator
    //! only for const and non-const iterator types for the same 
    //! element type
    //!
    template <typename I,
              typename = std::enable_if_t<is_comparable_iterator_v<I>>>
    constexpr bool operator != (I const &other) const noexcept {
        return !this->operator==(other);
    }
    //!
    //! @brief Less than operator used to compare to another iterator
    //! @tparam I - type of the other iterator
    //! @param other - other iterator we are comparing to
    //! @return false if address of element pointed by this iterator is 
    //! less than address of element pointed by another iterator
    //! @details We are using SFINAE to enable this operator
    //! only for const and non-const iterator types for the same 
    //! element type
    //!
    template <typename I,
              typename = std::enable_if_t<is_comparable_iterator_v<I>>>
    constexpr bool operator < (I const &other) const noexcept {
        return p_ < other.get_ptr();
    }
    //!
    //! @brief Less than or equals operator used to compare to another iterator
    //! @tparam I - type of the other iterator
    //! @param other - other iterator we are comparing to
    //! @return false if address of element pointed by this iterator is 
    //! less or equal than address of element pointed by another iterator
    //! @details We are using SFINAE to enable this operator
    //! only for const and non-const iterator types for the same 
    //! element type
    //!
    template <typename I,
              typename = std::enable_if_t<is_comparable_iterator_v<I>>>
    constexpr bool operator <= (I const &other) const noexcept {
        return p_ <= other.get_ptr();
    }
    //!
    //! @brief Greater than operator used to compare to another iterator
    //! @tparam I - type of the other iterator
    //! @param other - other iterator we are comparing to
    //! @return true if address of element pointed by this iterator is 
    //! greater than address of element pointed by another iterator
    //! @details We are using SFINAE to enable this operator
    //! only for const and non-const iterator types for the same 
    //! element type
    //!
    template <typename I,
              typename = std::enable_if_t<is_comparable_iterator_v<I>>>
    constexpr bool operator > (I const &other) const noexcept {
        return !this->operator<=(other);
    }
    //!
    //! @brief Greater or equals than operator used to compare to another iterator
    //! @tparam I - type of the other iterator
    //! @param other - other iterator we are comparing to
    //! @return true if address of element pointed by this iterator is 
    //! greater or equals than address of element pointed by another iterator
    //! @details We are using SFINAE to enable this operator
    //! only for const and non-const iterator types for the same 
    //! element type
    //!
    template <typename I,
              typename = std::enable_if_t<is_comparable_iterator_v<I>>>
    constexpr bool operator >= (I const &other) const noexcept {
        return !this->operator<(other);
    }
    //!
    //! @brief Prefix increment operator
    //! @returns reference to this iterator
    //! @details Advances iterator to the next element
    //!
    constexpr flat_forward_list_iterator_t &operator++() noexcept {
        size_t const next_offset = traits_traits::get_next_offset(p_);
        if (0 == next_offset) {
            size_with_padding_t const element_size{ traits_traits::get_size(p_) };
            p_ += element_size.size_padded();
        } else {
            p_ += next_offset;
        }
        return *this;
    }
    //!
    //! @brief Postfix increment operator
    //! @return value of iterator before it was advanced
    //! to the next element
    //! @details Advances iterator to the next element
    //!
    constexpr flat_forward_list_iterator_t operator++(int) noexcept {
        flat_forward_list_iterator_t tmp{ p_ };
        size_t next_offset = traits_traits::get_next_offset(p_);
        if (0 == next_offset) {
            size_with_padding_t element_size{ traits_traits::get_size(p_) };
            p_ += element_size.size_padded();
        } else {
            p_ += next_offset;
        }
        return tmp;
    }
    //!
    //! @brief Add operator. Advances iterator specified number of times
    //! @param advance_by - number of times this iterator should be advanced
    //! @return value of iterator after it was advanced
    //! @details Advances iterator specified number of times
    //! caller is responsible for making sure iterator would not get
    //! advanced beyond container's end, if that happen then beheviour is 
    //! undefined.
    //!
    constexpr flat_forward_list_iterator_t operator+(unsigned int advance_by) const noexcept {
        flat_forward_list_iterator_t result{ get_ptr() };
        while (nullptr != result.get_ptr() && 0 != advance_by) {
            ++result;
            --advance_by;
        }
        return result;
    }
    //!
    //! @brief Dereference operator. 
    //! @return Returns a reference to the element pointed by iterator
    //!
    constexpr T &operator*() const noexcept {
        return *reinterpret_cast<T *>(p_);
    }
    //!
    //! @brief pointer operator. 
    //! @return Returns a pointer to the element pointed by iterator
    //!
    constexpr T *operator->() const noexcept {
        return reinterpret_cast<T *>(p_);
    }
    //!
    //! @return Returns a pointer to the buffer conteining element. 
    //!
    constexpr buffer_char_pointer get_ptr() const noexcept {
        return p_;
    }
    //!
    //! @brief Assigns iterator to point to the new element
    //! @param p - pointer to the new element 
    //! @return Returns pointer to the previous element. 
    //!
    constexpr buffer_char_pointer reset_ptr(buffer_char_pointer p) noexcept {
        buffer_char_pointer tmp{ p_ };
        p_ = p;
        return tmp;
    }

private:

    //!
    //! @brief Explicit constructor from char * or char const *, depending on T being const
    //! @param p - pointer to the buffer that contains element
    //!
    constexpr explicit flat_forward_list_iterator_t(buffer_char_pointer p) noexcept
        : p_(p) {
    }
    //!
    //! @brief Explicit constructor from T *
    //! @param p - pointer to element
    //!
    constexpr explicit flat_forward_list_iterator_t(T *p) noexcept
        : p_((buffer_char_pointer)p) {
    }
    //!
    //! @brief Explicit constructor from unsigned char * or unsigned char const *,
    //! depending on T being const
    //! @param p - pointer to the buffer that contains element
    //!
    constexpr explicit flat_forward_list_iterator_t(buffer_unsigned_char_pointer p) noexcept
        : p_((buffer_char_pointer )p) {
    }
    //!
    //! @brief Explicit constructor from void * or void const *,
    //! depending on T being const
    //! @param p - pointer to the buffer that contains element
    //!
    constexpr explicit flat_forward_list_iterator_t(buffer_void_pointer p) noexcept
        : p_((buffer_char_pointer )p) {
    }
    //!
    //! @brief assignment operator from char * 
    //! @param p - pointer to the buffer that contains element
    //! @return returns a reference to this object
    //!
    constexpr flat_forward_list_iterator_t &operator= (buffer_char_pointer p) noexcept {
        p_ = p;
        return *this;
    }
    //!
    //! @brief assignment operator from T * 
    //! @param p - pointer to the element
    //! @return returns a reference to this object
    //!
    constexpr flat_forward_list_iterator_t &operator= (T *p) noexcept {
        p_ = (buffer_char_pointer)p;
        return *this;
    }
    //!
    //! @brief assignment operator from unsigned char * 
    //! @param p - pointer to the buffer that contains element
    //! @return returns a reference to this object
    //!
    constexpr flat_forward_list_iterator_t &operator= (buffer_unsigned_char_pointer p) noexcept {
        p_ = (buffer_char_pointer )p;
        return *this;
    }
    //!
    //! @brief assignment operator from void * 
    //! @param p - pointer to the buffer that contains element
    //! @return returns a reference to this object
    //!
    constexpr flat_forward_list_iterator_t &operator= (buffer_void_pointer p) noexcept {
        p_ = (buffer_char_pointer )p;
        return *this;
    }
    //!
    //! @brief pointer to the current element
    //!
    buffer_char_pointer p_{ nullptr };
};

//!
//! @typedef flat_forward_list_iterator
//! @brief non-const iterator
//! @tparam T - element type
//! @tparam TT - element type traits
//! @details Default initialized to specialization 
//! of flat_forward_list_traits for T
//!
template<typename T,
         typename TT = flat_forward_list_traits<T>>
using flat_forward_list_iterator = flat_forward_list_iterator_t<T, TT>;

//!
//! @typedef flat_forward_list_const_iterator
//! @brief const iterator
//! @tparam T - element type
//! @tparam TT - element type traits
//! @details Default initialized to specialization 
//! of flat_forward_list_traits for T
//!
template<typename T,
         typename TT = flat_forward_list_traits<T>>
using flat_forward_list_const_iterator = flat_forward_list_iterator_t< std::add_const_t<T>, TT>;

//!
//! @class flat_forward_list_ref
//! @brief Non owning container for flat forward list
//! @tparam T - element type
//! @tparam TT - element type traits
//! Provides helpers accessing elements of the buffer
//! without taking ownership of the buffer. 
//!
template <typename T,
          typename TT = flat_forward_list_traits<T>>
class flat_forward_list_ref final {
public:

    //
    // Technically we need T to be 
    // - trivialy destructable
    // - trivialy constructable
    // - trivialy movable
    // - trivialy copyable
    //
    static_assert(std::is_pod_v<T>, "T must be a Plain Old Definition");
    //!
    //! @brief True if this is a ref and false if this is a view
    //!
    inline static bool const is_ref{ !std::is_const_v<T> };
    //!
    //! @typedef value_type
    //! @brief Element value type
    //!
    using value_type = T;
    //!
    //! @typedef pointer
    //! @brief Pointer to element type
    //!
    using pointer = T * ;
    //!
    //! @typedef const_pointer
    //! @brief Pointer to const element type
    //!
    using const_pointer = T const *;
    //!
    //! @typedef reference
    //! @brief Reference to the element type
    //!
    using reference = T & ;
    //!
    //! @typedef const_reference
    //! @brief Reference to the const element type
    //!
    using const_reference = T const &;
    //!
    //! @typedef size_type
    //! @brief Size type
    //!
    using size_type = std::size_t;
    //!
    //! @typedef difference_type
    //! @brief Element pointers difference type
    //!
    using difference_type = std::ptrdiff_t;
    //!
    //! @typedef traits
    //! @brief Element traits type
    //!
    using traits = TT;
    //!
    //! @typedef traits_traits
    //! @brief Element traits traits type
    //!
    using traits_traits = flat_forward_list_traits_traits<T, TT>;
    //!
    //! @typedef range_t
    //! @brief Vocabulary type used to describe
    //! buffer used by the element and how much
    //! of this buffer is used by the data.
    //! Type also includes information about 
    //! required alignment.
    //!
    using range_t = typename traits_traits::range_t;
    //!
    //! @typedef size_with_padding_t
    //! @brief Vocabulary type used to describe size with padding
    //! 
    using size_with_padding_t = typename traits_traits::size_with_padding_t;
    //!
    //! @typedef offset_with_aligment_t
    //! @brief Vocabulary type used to describe size with alignment
    //! 
    using offset_with_aligment_t = typename traits_traits::offset_with_aligment_t;
    //!
    //! @typedef sizes_t
    //! @brief Vocabulary type that contains information about buffer size, and last element range. 
    //!
    using sizes_t = flat_forward_list_sizes<traits_traits::alignment>;
    //!
    //! @typedef buffer_value_type
    //! @details Since we have variable size elementa,
    //! and we cannot express it in the C++ type system
    //! we treat buffer with elements as a bag of chars
    //! and cast to the element type when nessesary.
    //! @brief Depending if T is const we will
    //! use const or non-const buffer pointer
    //!
    using buffer_value_type = std::conditional_t<std::is_const_v<T>, char const, char>;
    //!
    //! @typedef const_buffer_value_type
    //! @brief When we do not intend to modify buffer 
    //! we can treat it as a bag of const characters
    //!
    using const_buffer_value_type = buffer_value_type const;
    //!
    //! @typedef buffer_pointer
    //! @brief Type used as a buffer pointer.
    //!
    using buffer_pointer = buffer_value_type *;
    //!
    //! @typedef const_buffer_pointer
    //! @brief Type used as a pointer ot the const buffer.
    //!
    using const_buffer_pointer = const_buffer_value_type *;
    //!
    //! @typedef iterator
    //! @brief Type of iterator
    //!
    using iterator = flat_forward_list_iterator<T, TT>;
    //!
    //! @typedef const_iterator
    //! @brief Type of const iterator
    //!
    using const_iterator = flat_forward_list_const_iterator<T, TT>;
    //!
    //! @typedef buffer_type
    //! @brief Pointers that describe buffer
    //! @details Depending if T is const buffer uses char * or char const *
    //!
    using buffer_type = buffer_t<buffer_value_type>;
    //!
    //! @brief Constant that represents and invalid 
    //! or non-existent position
    //!
    inline static size_type const npos = iffl::npos;
    //!
    //! @brief Default constructor
    //!
    flat_forward_list_ref() noexcept
        : buffer_() {
    }
    //!
    //! @brief Assignment operator to view from a ref
    //! @tparam V - For SFINAE we need to make this method a template
    //! Type const iterator is pointing to.
    //! @tparam VV - Type traits.
    //! @details Use SFINAE to enable it only on const instantiation
    //! to support assignment from a non-const instantiation of template
    //!
    template<typename V,
             typename VV,
             typename = std::enable_if<std::is_assignable_v<T*, V*>>>
    flat_forward_list_ref(flat_forward_list_ref<V, VV> const &other) noexcept
        : buffer_(other.buffer_) {
    }
    //!
    //! @brief Constructor that initializas view to point 
    //! to the buffer buffer
    //! @param other_buff - pointer to the start of the buffer
    //! that contains list.
    //! @details This constructor does not validate if this is
    //! a valid flat forward list. It assumes that
    //! caller validated buffer before using this constructor.
    //!
    explicit flat_forward_list_ref(buffer_type const &other_buff) noexcept
        : buffer_(other_buff) {
    }

    //!
    //! @brief Constructor that copies list from a buffer
    //! @param buffer_begin - pointer to the start of the buffer
    //! that contains list.
    //! @param buffer_end - pointer to the address right 
    //! after last byte in the buffer.
    //! @param last_element - pointers to the last element of the
    //! list in the buffer. If buffer does not contain any elements
    //! then this parameter must be nullptr.
    //! @details This constructor does not validate if this is
    //! a valid flat forward list. It assumes that
    //! caller validated buffer before using this constructor.
    //!
    flat_forward_list_ref(buffer_pointer buffer_begin,
                          buffer_pointer last_element,
                          buffer_pointer buffer_end) noexcept
        : buffer_(buffer_type{ buffer_begin, last_element, buffer_end }) {
    }
    //!
    //! @brief Constructor that creaates a view over a buffer described 
    //! by a pair of iterators
    //! @tparam V - For SFINAE we need to make this method a template
    //! Type const iterator is pointing to.
    //! @tparam VV - Type traits.
    //! @param begin - iterator for the buffer begin.
    //! @param last - last element that should be included in the view
    //! @details This constructor does not validate if this is
    //! a valid flat forward list. It assumes that
    //! caller validated buffer before using this constructor.
    //!
    template <typename V, 
              typename VV,
              typename = std::enable_if<std::is_assignable_v<T*, V*>>>
    flat_forward_list_ref(flat_forward_list_iterator<V, VV> const &begin,
                          flat_forward_list_iterator<V, VV> const &last) noexcept
        : buffer_(buffer_type{begin.get_ptr(), 
                              last.get_ptr(), 
                              reinterpret_cast<buffer_pointer>(last.get_ptr()) + 
                                                               traits_traits::get_size(last.get_ptr()).size }) {
    }
    //!
    //! @brief Constructor that checks if buffer contains a valid list
    //! and if it does then copies that list.
    //! @param buffer - pointer to the start of the buffer
    //! that might contains list.
    //! @param buffer_size - bufer size.
    //! @details This constructor searches for the last
    //! valid element in the buffer, and is buffer is valid then it
    //! copies elements to the new buffer.
    //!
    flat_forward_list_ref(buffer_pointer *buffer,
                          size_t buffer_size) noexcept {
        assign(buffer, buffer_size);
    }
    //!
    //! @brief Constructs view from container
    //! @tparam V - For SFINAE we need to make this method a template
    //! Type const iterator is pointing to.
    //! @tparam VV - Type traits.
    //! @tparam A - allocator type
    //! @param c - container we are constructing view from
    //!
    template <typename V,
              typename VV,
              typename A,
              typename UNUSED = std::enable_if<std::is_assignable_v<T*, V*>>>
    explicit flat_forward_list_ref(flat_forward_list<V, VV, A> const &c) noexcept;

    //!
    //! @brief Copy assignment operator.
    //! @param other - linked list we are moving from
    //!
    template <typename V,
              typename VV,
              typename = std::enable_if<std::is_assignable_v<T*, V*>>>
    flat_forward_list_ref &operator= (flat_forward_list_ref<V, VV> const & other) {
        buffer_ = other.buffer_;
        return *this;
    }
    //!
    //! @brief Copy assignment operator.
    //! @param other_buffer - linked list we are copying from
    //!
    flat_forward_list_ref &operator= (buffer_view const &other_buffer) {
        buffer_ = other_buffer;
        return *this;
    }
    //!
    //! @brief Constructs view from container
    //! @tparam V - For SFINAE we need to make this method a template
    //! Type const iterator is pointing to.
    //! @tparam VV - Type traits.
    //! @tparam A - allocator type
    //! @param c - container we are constructing view from
    //! @returns referencce to self
    //!
    template <typename V,
              typename VV,
              typename A,
              typename UNUSED = std::enable_if<std::is_assignable_v<T*, V*>>>
    flat_forward_list_ref &operator= (flat_forward_list<V, VV, A> const &c) noexcept;
    //!
    //! @brief Destructor.
    //! @details Deallocates buffer owned by container.
    //!
    ~flat_forward_list_ref() noexcept {
        buff().clear();
    }
    //!
    //! @brief Assigns view to point to the described buffer
    //! @param other_buff - pointer to the start of the buffer
    //! that contains list.
    //! @details This method does not validate if this is
    //! a valid flat forward list. It assumes that
    //! caller validated buffer before using this constructor.
    //!
    void assign(buffer_type const &other_buff) noexcept {
        buffer_ = other_buff;
    }
    //!
    //! @brief Assigns view to point to the described buffer
    //! @param buffer_begin - pointer to the start of the buffer
    //! that contains list.
    //! @param last_element - pointer to the last element of the list.
    //! @param buffer_end - pointer to the end of the buffer.
    //! @details This method does not validate if this is
    //! a valid flat forward list. It assumes that
    //! caller validated buffer before using this constructor.
    //!
    void assign(buffer_pointer buffer_begin,
                buffer_pointer last_element,
                buffer_pointer buffer_end) noexcept {
        buffer_ = buffer_type{ buffer_begin, last_element, buffer_end };
    }
    //!
    //! @brief Assigns view to point to the described buffer
    //! @tparam V - For SFINAE we need to make this method a template
    //! Type const iterator is pointing to.
    //! @tparam VV - Type traits.
    //! @param begin - iterator for the buffer begin.
    //! @param last - last element that should be included in the view
    //!
    template <typename V,
              typename VV,
              typename = std::enable_if<std::is_assignable_v<T*, V*>>>
    void assign(flat_forward_list_iterator<V, VV> const &begin,
                flat_forward_list_iterator<V, VV> const &last) noexcept {
        buffer_ = buffer_type{ begin.get_ptr(),
                              last.get_ptr(),
                              reinterpret_cast<buffer_pointer>(last.get_ptr()) +
                                                               traits_traits::get_size(last.get_ptr()).size };
    }
    //!
    //! @brief Assigns view to point to the described buffer
    //! @param buffer - pointer to the begin of a buffer.
    //! @param buffer_size - size of the buffer.
    //!
    void assign(buffer_pointer *buffer,
                size_t buffer_size) noexcept {

        auto[is_valid, buffer_view] = flat_forward_list_validate<T, TT>(buffer,
                                                                        buffer + buffer_size);

        buffer_ = buffer_type{ buffer,
                               is_valid ? buffer_view.last().get_ptr() : nullptr,
                               buffer + buffer_size };
    }
    //!
    //! @brief Swaps content of this container and the other container.
    //! @param other - reference to the other container
    //! @throws might throw std::bad_alloc if allocator swap throws or if
    //! allocators do not suport swap, and we need to make a copy of elements,
    //! which involves allocation.
    //!
    void swap(flat_forward_list_ref &other) noexcept {
        flat_forward_list_ref tmp{ std::move(other) };
        other = std::move(*this);
        *this = std::move(tmp);
    }
    //!
    //! @return Returns a reference to the first element in the container.
    //! @details Calling this method on a empty container will trigger fail-fast
    //!
    T& front() {
        validate_pointer_invariants();
        FFL_CODDING_ERROR_IF(buff().last == nullptr || buff().begin == nullptr);
        return *(T *)buff().begin;
    }
    //!
    //! @return Returns a const reference to the first element in the container.
    //! @details Calling this method on a empty container will trigger fail-fast
    //!
    T const &front() const {
        validate_pointer_invariants();
        FFL_CODDING_ERROR_IF(buff().last == nullptr || buff().begin == nullptr);
        return *(T *)buff().begin;
    }
    //!
    //! @return Returns a reference to the last element in the container.
    //! @details Calling this method on a empty container will trigger fail-fast
    //!
    T& back() {
        validate_pointer_invariants();
        FFL_CODDING_ERROR_IF(buff().last == nullptr);
        return *(T *)buff().last;
    }
    //!
    //! @return Returns a const reference to the last element in the container.
    //! @details Calling this method on a empty container will trigger fail-fast
    //!
    T const &back() const {
        validate_pointer_invariants();
        FFL_CODDING_ERROR_IF(buff().last == nullptr);
        return *(T *)buff().last;
    }
    //!
    //! @return Returns an iterator pointing to the first element of container.
    //! @details Calling on an empty container returns end iterator
    //!
    iterator begin() noexcept {
        validate_pointer_invariants();
        return buff().last ? iterator{ buff().begin }
        : end();
    }
    //!
    //! @return Returns a const iterator pointing to the first element of container.
    //! @details Calling on an empty container returns const end iterator
    //!
    const_iterator begin() const noexcept {
        validate_pointer_invariants();
        return buff().last ? const_iterator{ buff().begin }
        : cend();
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

    //!
    //! @return Returns an iterator pointing to the last element of container.
    //! @details Calling on an empty container returns end iterator
    //!
    iterator last() noexcept {
        validate_pointer_invariants();
        return buff().last ? iterator{ buff().last }
        : end();
    }
    //!
    //! @return Returns a const iterator pointing to the last element of container.
    //! @details Calling on an empty container returns end iterator
    //!
    const_iterator last() const noexcept {
        validate_pointer_invariants();
        return buff().last ? const_iterator{ buff().last }
        : cend();
    }
    //!
    //! @return Returns an end iterator.
    //! @details An end iterator is pointing pass the last
    //! element of container.
    //! View might be pointing in a middle of a flat forward list
    //! so our last element might be not a last element of list.
    //! It is arbitrary what we are using as a pointer to the last 
    //! element, as long as we are using it consistently. 
    //! This implementation uses
    //! - If next_offset is not 0 then location of next element in the list pass the view.
    //!   This iterator is also a valid non-end iterator of container.
    //! - In all other cases pointer to the buffer pass the end of last element
    //!
    iterator end() noexcept {
        validate_pointer_invariants();
        if (buff().last) {
            if (traits_traits::has_next_offset_v) {
                size_type next_offset = traits_traits::get_next_offset(buff().last);
                if (next_offset) {
                    return iterator{ buff().last + next_offset };
                } else {
                    size_with_padding_t const last_element_size{ traits_traits::get_size(buff().last) };
                    return iterator{ buff().last + last_element_size.size_padded() };
                }
            } else {
                size_with_padding_t const last_element_size{ traits_traits::get_size(buff().last) };
                return iterator{ buff().last + last_element_size.size_padded() };
            }
        } else {
            return iterator{ };
        }
    }
    //!
    //! @return Returns an end const_iterator.
    //! @details An end iterator is pointing pass the last
    //! element of container
    //! View might be pointing in a middle of a flat forward list
    //! so our last element might be not a last element of list.
    //! It is arbitrary what we are using as a pointer to the last 
    //! element, as long as we are using it consistently. 
    //! This implementation uses
    //! - If next_offset is not 0 then location of next element in the list pass the view.
    //!   This iterator is also a valid non-end iterator of container.
    //! - In all other cases pointer to the buffer pass the end of last element
    //!
    const_iterator end() const noexcept {
        validate_pointer_invariants();
        if (buff().last) {
            if (traits_traits::has_next_offset_v) {
                size_type const next_offset{ traits_traits::get_next_offset(buff().last) };
                if (next_offset) {
                    return const_iterator{ buff().last + next_offset };
                } else {
                    size_with_padding_t const last_element_size{ traits_traits::get_size(buff().last) };
                    return const_iterator{ buff().last + last_element_size.size_padded() };
                }
            } else {
                size_with_padding_t const last_element_size{ traits_traits::get_size(buff().last) };
                return const_iterator{ buff().last + last_element_size.size_padded() };
            }
        } else {
            return const_iterator{ };
        }
    }
    //!
    //! @return Returns a const iterator pointing to the first element of container.
    //! @details Calling on an empty container returns const end iterator
    //!
    const_iterator cbegin() const noexcept {
        return begin();
    }
    //!
    //! @return Returns a const iterator pointing to the last element of container.
    //! @details Calling on an empty container returns end iterator
    //!
    const_iterator clast() const noexcept {
        return last();
    }
    //!
    //! @return Returns an end const_iterator.
    //! @details For types that support get_next_offset offset or when container is empty, 
    //! the end iterator is const_iterator{}.
    //! For types that do not support get_next_offset an end iterator is pointing pass the last
    //! element of container
    //!
    const_iterator cend() const noexcept {
        return end();
    }
    //!
    //! @return Pointer to the begging of the buffer or nullptr 
    //! container has if no allocated buffer.
    //!
    char* data() noexcept {
        return buff().begin;
    }
    //!
    //! @return Const pointer to the begging of the buffer or nullptr 
    //! container has if no allocated buffer.
    //!
    char const *data() const noexcept {
        return buff().begin;
    }
    //!
    //! @brief Validates that buffer contains a valid list.
    //! @return true if valid list was found and false otherwise.
    //! @details You must call this method after passing pointer to 
    //! container's buffer to a function that might change buffer content.
    //! If valid list of found then buff().last will be pointing to the
    //! element element that was found. If no valid list was found then
    //! buff().last will be nullptr.
    //!
    bool revalidate_data() noexcept {
        auto[valid, buffer_view] = flat_forward_list_validate<T, TT>(buff().begin, 
                                                                     buff().end);
        if (valid) {
            buff().last = buffer_view.last().get_ptr();
        }
        return valid;
    }
    //!
    //! @brief Returns capacity used by the element's data.
    //! @param it - iterator pointing to the element we are returning size for.
    //! @returns Returns size of the element without paddint
    //!
    size_type required_size(const_iterator const &it) const noexcept {
        validate_pointer_invariants();
        validate_iterator_not_end(it);
        return size_unsafe(it).size;
    }
    //!
    //! @param it - iterator pointing to the element we are returning size for.
    //! @returns For any element except last returns distance from element start 
    //! to the start of the next element. For the last element it returns used_capacity
    //!
    size_type used_size(const_iterator const &it) const noexcept {
        validate_pointer_invariants();
        validate_iterator_not_end(it);

        return used_size_unsafe(it);
    }
    //!
    //! @brief Information about the buffer occupied by an element.
    //! @param it - iterator pointing to an element.
    //! @returns For any element except the last, it returns:
    //! - start element offset.
    //! - offset of element data end.
    //! - offset of element buffer end.
    //! For the last element data end and buffer end point to the 
    //! same position.
    //! @details All offsets values are relative to the buffer 
    //! owned by the container.
    //!
    range_t range(const_iterator const &it) const noexcept {
        validate_iterator_not_end(it);

        return range_unsafe(it);
    }
    //!
    //! @brief Information about the buffer occupied by elements in the range [begin, last].
    //! @param begin - iterator pointing to the first element.
    //! @param last - iterator pointing to the last element in the range.
    //! @returns For any case, except when last element of range is last element of the collection,
    //! it returns:
    //! - start of the first element.
    //! - offset of the last element data end.
    //! - offset of the last element buffer end.
    //! If range last is last element of collection then data end and
    //! buffer end point to the same position.
    //! @details All offsets values are relative to the buffer 
    //! owned by the container.
    //!
    range_t closed_range(const_iterator const &begin, const_iterator const &last) const noexcept {
        validate_iterator_not_end(begin);
        validate_iterator_not_end(last);

        return closed_range_unsafe(begin, last);
    }
    //!
    //! @brief Information about the buffer occupied by elements in the range [begin, end).
    //! @param begin - iterator pointing to the first element.
    //! @param end - iterator pointing to the past last element in the range.
    //! @returns For any case, except when end element of range is the end element of the collection,
    //! - start of the first element.
    //! - offset of the element before end data end.
    //! - offset of the element before end buffer end.
    //! If range end is colelction end then data end and
    //! buffer end point to the same position.
    //! @details All offsets values are relative to the buffer 
    //! owned by the container.
    //! Algorithm has complexity O(number of elements in collection) because to find 
    //! element before end we have to scan buffer from beginning.
    //!
    range_t half_open_range(const_iterator const &begin, const_iterator const &end) const noexcept {
        validate_iterator_not_end(begin);
        validate_iterator_not_end(end);

        return half_open_range_usafe(begin, end);
    }
    //!
    //! @brief Tells if given offset in the buffer falls into a 
    //! buffer used by the element.
    //! @param it - iterator pointing to an element
    //! @param position - offset in the container's buffer.
    //! @returns True if position is in the element's buffer. 
    //! and false otherwise. Element's buffer is retrieved using 
    //! range(it).
    //! When iterator referes to container end or when position is npos
    //! the result will be false.
    //!
    bool contains(const_iterator const &it, size_type position) const noexcept {
        validate_iterator(it);
        if (cend() == it || npos == position) {
            return false;
        }
        range_t const r{ range_unsafe(it) };
        return r.buffer_contains(position);
    }
    //!
    //! @brief Searches for an element before the element that containes 
    //! given position.
    //! @param position - offset in the conteiner's buffer
    //! @returns Const iterator to the found element, if it was found, and
    //! container's end const iterator otherwise.
    //! @details Cost of this algorithm is O(nuber of elements in container)
    //! because we have to performa linear search for an element from the start
    //! of container's buffer.
    //!
    const_iterator find_element_before(size_type position) const noexcept {
        validate_pointer_invariants();
        if (empty_unsafe()) {
            return end();
        }
        auto[is_valid, buffer_view] = flat_forward_list_validate<T, TT>(buff().begin,
                                                                        buff().begin + position);
        if (!buffer_view.empty()) {
            return const_iterator{ buffer_view.last().get_ptr() };
        }
        return end();
    }
    //!
    //! @brief Searches for an element that containes given position.
    //! @param position - offset in the conteiner's buffer
    //! @returns Const iterator to the found element, if it was found, and
    //! container's end const iterator otherwise.
    //! @details Cost of this algorithm is O(nuber of elements in container)
    //! because we have to performa linear search for an element from the start
    //! of container's buffer.
    //!
    const_iterator find_element_at(size_type position) const noexcept {
        const_iterator it = find_element_before(position);
        if (cend() != it) {
            ++it;
            if (cend() != it) {
                FFL_CODDING_ERROR_IF_NOT(contains(it, position));
                return it;
            }
        }
        return end();
    }
    //!
    //! @brief Searches for an element after the element that containes 
    //! given position.
    //! @param position - offset in the conteiner's buffer
    //! @returns Const iterator to the found element, if it was found, and
    //! container's end const iterator otherwise.
    //! @details Cost of this algorithm is O(nuber of elements in container)
    //! because we have to performa linear search for an element from the start
    //! of container's buffer.
    //!
    const_iterator find_element_after(size_type position) const noexcept {
        const_iterator it = find_element_at(position);
        if (cend() != it) {
            ++it;
            if (cend() != it) {
                return it;
            }
        }
        return end();
    }
    //!
    //! @brief Number of elements in the container.
    //! @returns Number of elements in the container.
    //! @details Cost of this algorithm is O(nuber of elements in container).
    //! Container does not actively cache/updates element count so we need to
    //! scan list to find number of elements.
    //!
    size_type size() const noexcept {
        validate_pointer_invariants();
        size_type s = 0;
        std::for_each(cbegin(), cend(), [&s](T const &) {
            ++s;
        });
        return s;
    }
    //!
    //! @brief Tells if container contains no elements.
    //! @returns False when container contains at least one element
    //! and true otherwise.
    //! @details Both container with no buffer as well as container
    //! that has buffer that does not contain any valid elements will
    //! return true.
    //!
    bool empty() const noexcept {
        validate_pointer_invariants();
        return  buff().last == nullptr;
    }
    //!
    //! @returns True when containe contains at least one element
    //! and false otherwise.
    //!
    explicit operator bool() const {
        return !empty();
    }
    //!
    //! @returns Number of bytes in the bufer used
    //! by existing elements.
    //!
    size_type used_capacity() const noexcept {
        validate_pointer_invariants();
        sizes_t s{ get_all_sizes() };
        return s.used_capacity().size;
    }
    //!
    //! @returns Buffer size.
    //!
    size_type total_capacity() const noexcept {
        validate_pointer_invariants();
        return buff().end - buff().begin;
    }
    //!
    //! @returns Number of bytes in the buffer not used
    //! by the existing elements.
    //!
    size_type remaining_capacity() const noexcept {
        validate_pointer_invariants();
        sizes_t s{ get_all_sizes() };
        return s.remaining_capacity;
    }

private:
    //!
    //! @brief Tells if container contains no elements.
    //! @returns False when containe contains at least one element
    //! and true otherwise.
    //! @details Both container with no buffer as well as container
    //! that has buffer that does not contain any valid elements will
    //! return true.
    //!
    bool empty_unsafe() const noexcept {
        return  buff().last == nullptr;
    }
    //!
    //! @brief Validates that container pointer invariants.
    //!
    void validate_pointer_invariants() const noexcept {
        buff().validate();
    }
    //!
    //! @brief Validates iterator invariants.
    //! @param it - iterator that we are verifying
    //!
    void validate_iterator(const_iterator const &it) const noexcept {
        //
        // If we do not have any valid elements then
        // only end iterator is valid.
        // Otherwise iterator should be an end or point somewhere
        // between begin of the buffer and start of the first element
        //
        if (empty_unsafe()) {
            FFL_CODDING_ERROR_IF_NOT(cend() == it);
        } else {
            FFL_CODDING_ERROR_IF_NOT(cend() == it ||
                                     (buff().begin <= it.get_ptr() && it.get_ptr() <= buff().last));
            validate_compare_to_all_valid_elements(it);
        }
    }
    //!
    //! @brief Validates iterator invariants, as well as asserts 
    //! that iterator is not an end iterator.
    //! @param it - iterator that we are verifying
    //!
    void validate_iterator_not_end(const_iterator const &it) const noexcept {
        //
        // Used in case when end iterator is meaningless.
        // Validates that container is not empty,
        // and iterator is pointing somewhere between
        // begin of the buffer and start of the first element
        //
        FFL_CODDING_ERROR_IF(cend() == it);
        FFL_CODDING_ERROR_IF(const_iterator{} == it);
        FFL_CODDING_ERROR_IF_NOT(buff().begin <= it.get_ptr() && it.get_ptr() <= buff().last);
        validate_compare_to_all_valid_elements(it);
    }
    //!
    //! @brief Validates iterator points to a valid element 
    //! in the container 
    //! @param it - iterator that we are verifying
    //!
    void validate_compare_to_all_valid_elements(const_iterator const &it) const noexcept {
#ifdef FFL_DBG_CHECK_ITERATOR_VALID
        //
        // If not end iterator then must point to one of the valid iterators.
        //
        if (end() != it) {
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
    //!
    //! @brief Size used by an element data
    //! @param it - iterator for the element user queries size for
    //! @returns Returns size that is required for element data
    //!
    size_with_padding_t size_unsafe(const_iterator const &it) const noexcept {
        return  traits_traits::get_size(it.get_ptr());
    }
    //!
    //! @brief Size used by an element data and padding
    //! @param it - iterator for the element user queries size for
    //! @returns For types that support get_next_offset it should
    //! return value of offset to the next element from the start of 
    //! the element pointer by the iterator. If this is last element of the 
    //! list then it returns element data size without padding.
    //! For types that do not support get_next_offset it returns padded
    //! element size, except the last element. For the last element we return
    //! size without padding.
    //! @details Last element is treated differently because container 
    //! buffer might not include space for the last element padding.
    //!
    size_type used_size_unsafe(const_iterator const &it) const noexcept {
        if constexpr (traits_traits::has_next_offset_v) {
            size_type next_offset = traits_traits::get_next_offset(it.get_ptr());
            if (0 == next_offset) {
                size_with_padding_t s{ traits_traits::get_size(it.get_ptr()) };
                //
                // Last element
                //
                next_offset = s.size;
            }
            return next_offset;
        } else {
            size_with_padding_t s{ traits_traits::get_size(it.get_ptr()) };
            //
            // buffer might end without including padding for the last element
            //
            if (last() == it) {
                return s.size;
            } else {
                return s.size_padded();
            }
        }
    }
    //!
    //! @brief Returns offsets in the container buffer that describe
    //! part of the buffer used by this element.
    //! @param it - iterator for the element user queries range for
    //! @returns Range that describes part of the container buffer used
    //! by the element
    //! @details For the last element of the buffer end of the data and
    //! end of the buffer have the same value.
    //! For any element data end is calculated using size of data used by element.
    //! For any element except last buffer size is calculated using
    //!  - Value of offset to the next element if type supports get_next_offset
    //!  - If get_next_offset is not suported then it is calculated as padded 
    //!    element data size
    //! This method assumes that iterator is pointing to an element, and is not an
    //! end iterator. It assumes that caller verified that iterator satisfies 
    //! these preconditions.
    //!
    range_t range_unsafe(const_iterator const &it) const noexcept {
        size_with_padding_t s{ traits_traits::get_size(it.get_ptr()) };
        range_t r{};
        r.buffer_begin = it.get_ptr() - buff().begin;
        r.data_end = r.begin() + s.size;
        if constexpr (traits_traits::has_next_offset_v) {
            size_type next_offset = traits_traits::get_next_offset(it.get_ptr());
            if (0 == next_offset) {
                FFL_CODDING_ERROR_IF(last() != it);
                r.buffer_end = r.begin() + s.size;
            } else {
                r.buffer_end = r.begin() + next_offset;
            }
        } else {
            if (last() == it) {
                r.buffer_end = r.begin() + s.size;
            } else {
                r.buffer_end = r.begin() + s.size_padded();
            }
        }
        return r;
    }
    //!
    //! @returns information about the pasrt of container 
    //! buffer used by elements in the range [first, last]
    //!
    range_t closed_range_unsafe(const_iterator const &first, const_iterator const &last) const noexcept {
        if (first == last) {
            return element_range_unsafe(first);
        } else {
            range_t first_element_range{ range_unsafe(first) };
            range_t last_element_range{ range_unsafe(last) };

            return range_t{ {first_element_range.begin,
                             last_element_range.data_end,
                             last_element_range.buffer_end } };
        }
    }
    //!
    //! @returns information about the pasrt of container 
    //! buffer used by elements in the range [first, end)
    //! @details Algorithm complexity is O(number of elements in the niffer)
    //! because we need to scan container from the beginning to find element 
    //! before end.
    //!
    range_t half_open_range_usafe(const_iterator const &first,
                                  const_iterator const &end) const noexcept {

        if (this->end() == end) {
            return closed_range_usafe(first, last());
        }

        size_t end_begin{ static_cast<size_t>(end->get_ptr() - buff().begin) };
        iterator last{ find_element_before(end_begin) };

        return closed_range_usafe(first, last);
    }
    //!
    //! @returns Information about containers buffer
    //! @details Most algorithms that modify container
    //! use this method to buffer information in a single 
    //! snapshot.
    //!
    sizes_t get_all_sizes() const noexcept {
        sizes_t s;

        s.total_capacity = buff().end - buff().begin;

        if (nullptr != buff().last) {
            s.last_element = range_unsafe(const_iterator{ buff().last });
        }
        FFL_CODDING_ERROR_IF(s.total_capacity < s.used_capacity().size);

        return s;
    }

    //!
    //! @brief Returns reference to the struct that 
    //! describes buffer.
    //! Helps abstracting out uglification that comes from
    //! using a compressed_pair.
    //!
    buffer_type &buff() noexcept {
        return buffer_;
    }
    //!
    //! @brief Returns const reference to the struct that 
    //! describes buffer.
    //! Helps abstracting out uglification that comes from
    //! using a compressed_pair.
    //!
    buffer_type const &buff() const noexcept {
        return buffer_;
    }

    buffer_type buffer_;
};
//!
//! @typedef flat_forward_list_view
//! @brief Constant view to flat forward list
//!
template <typename T,
          typename TT = flat_forward_list_traits<T>>
using flat_forward_list_view = flat_forward_list_ref<T const, TT>;

//!
//! @tparam T - element type
//! @tparam TT - element type traits
//! @tparam A - allocator type that should be used for this container
//! @param lhs - one of the containers we are swapping between
//! @param rhs - other container we are swapping between
//! @details 
//! TT is aefault initialized to specialization 
//! of flat_forward_list_traits for T
//! A is default initialized to std::allocator for T
//!
template <typename T,
          typename TT>
inline void swap(flat_forward_list_ref<T, TT> &lhs, flat_forward_list_ref<T, TT> &rhs) noexcept {
    lhs.swap(rhs);
}
//!
//! @tparam T - element type
//! @tparam TT - element type traits
//! @tparam A - allocator type that should be used for this container
//! @param c - container
//!
//! @details 
//! TT is aefault initialized to specialization 
//! of flat_forward_list_traits for T
//! A is default initialized to std::allocator for T
//!
template <typename T,
          typename TT>
inline typename flat_forward_list_ref<T, TT>::iterator begin(flat_forward_list_ref<T, TT> &c) noexcept {
    return c.begin();
}
//!
//! @tparam T - element type
//! @tparam TT - element type traits
//! @tparam A - allocator type that should be used for this container
//! @param c - container
//!
//! @details 
//! TT is aefault initialized to specialization 
//! of flat_forward_list_traits for T
//! A is default initialized to std::allocator for T
//!
template <typename T,
          typename TT>
inline typename flat_forward_list_ref<T, TT>::const_iterator begin(flat_forward_list_ref<T, TT> const &c) noexcept {
    return c.begin();
}
//!
//! @tparam T - element type
//! @tparam TT - element type traits
//! @tparam A - allocator type that should be used for this container
//! @param c - container
//!
//! @details 
//! TT is aefault initialized to specialization 
//! of flat_forward_list_traits for T
//! A is default initialized to std::allocator for T
//!
template <typename T,
          typename TT>
inline typename flat_forward_list_ref<T, TT>::const_iterator cbegin(flat_forward_list_ref<T, TT> const &c) noexcept {
    return c.cbegin();
}
//!
//! @tparam T - element type
//! @tparam TT - element type traits
//! @tparam A - allocator type that should be used for this container
//! @param c - container
//!
//! @details 
//! TT is aefault initialized to specialization 
//! of flat_forward_list_traits for T
//! A is default initialized to std::allocator for T
//!
template <typename T,
          typename TT>
inline typename flat_forward_list_ref<T, TT>::iterator end(flat_forward_list_ref<T, TT> &c) noexcept {
    return c.end();
}
//!
//! @tparam T - element type
//! @tparam TT - element type traits
//! @tparam A - allocator type that should be used for this container
//! @param c - container
//!
//! @details 
//! TT is aefault initialized to specialization 
//! of flat_forward_list_traits for T
//! A is default initialized to std::allocator for T
//!
template <typename T,
          typename TT>
inline typename flat_forward_list_ref<T, TT>::const_iterator end(flat_forward_list_ref<T, TT> const &c) noexcept {
    return c.end();
}
//!
//! @tparam T - element type
//! @tparam TT - element type traits
//! @tparam A - allocator type that should be used for this container
//! @param c - container
//!
//! @details 
//! TT is aefault initialized to specialization 
//! of flat_forward_list_traits for T
//! A is default initialized to std::allocator for T
//!
template <typename T,
          typename TT>
inline typename flat_forward_list_ref<T, TT>::const_iterator cend(flat_forward_list_ref<T, TT> const &c) noexcept {
    return c.cend();
}
//!
//! @tparam T - element type
//! @tparam TT - element type traits
//! @tparam A - allocator type that should be used for this container
//! @param c - container
//!
//! @details 
//! TT is aefault initialized to specialization 
//! of flat_forward_list_traits for T
//! A is default initialized to std::allocator for T
//!
template <typename T,
          typename TT>
inline typename flat_forward_list_ref<T, TT>::iterator last(flat_forward_list_ref<T, TT> &c) noexcept {
    return c.last();
}
//!
//! @tparam T - element type
//! @tparam TT - element type traits
//! @tparam A - allocator type that should be used for this container
//! @param c - container
//!
//! @details 
//! TT is aefault initialized to specialization 
//! of flat_forward_list_traits for T
//! A is default initialized to std::allocator for T
//!
template <typename T,
          typename TT>
inline typename flat_forward_list_ref<T, TT>::const_iterator last(flat_forward_list_ref<T, TT> const &c) noexcept {
    return c.last();
}
//!
//! @tparam T - element type
//! @tparam TT - element type traits
//! @tparam A - allocator type that should be used for this container
//! @param c - container
//!
//! @details 
//! TT is aefault initialized to specialization 
//! of flat_forward_list_traits for T
//! A is default initialized to std::allocator for T
//!
template <typename T,
          typename TT>
inline typename flat_forward_list_ref<T, TT>::const_iterator clast(flat_forward_list_ref<T, TT> const &c) noexcept {
    return c.clast();
}

//!
//! @class flat_forward_list
//! @brief Intrusive flat forward list container
//! @tparam T - element type
//! @tparam TT - element type traits
//! @tparam A - allocator type that should be used for this container
//! @details 
//! TT is aefault initialized to specialization 
//! of flat_forward_list_traits for T
//! A is default initialized to std::allocator for T
//! Container is inherited from allocator to utilize
//! Empty Base Class Optimization (EBCO).
//!
//! Iterator invalidation notes:
//! Begin iterator might get invalidated on any operation that
//! causes buffer reallocation or erases last element of container
//! Any other iterator, including an end iterator can get invalidated
//! on any operation that causes buffer reallocation or adds, removes
//! or resizes elements of container.
//! Methods that take iterator as an input, and can invalidate it
//! return new valid iterator as an output.
//! Caller must refresh end iterator by explicitely calling [c]end().
//!
//! Debugging notes:
//! Defining FFL_DBG_CHECK_ITERATOR_VALID will enable validation of
//! that iterators passed in input parameters are valid and are pointing
//! to an existing element. Cost of this validation is O(element count). 
//! Defining FFL_DBG_CHECK_DATA_VALID will enable validation of container
//! after every method that modifies container data.
//!
//! Iterator names notes:
//! begin or first - first element in a half-open [first, end) 
//!       or closed [first, last] range.
//! last - referes to the last element in half-closed (before_begin, last] 
//!       or closed [first, last] range.
//! end - referes to an element pass the last element. It is used with 
//!       half open [begin, end) or open (before_begin, end) range.
//! before_beging or before_first - referes to an element preceeding 
//!       begin or first element. Is used in halfope (before_begin, last]
//!       or open (before_begin, end) range.
//!
//! Security notes:
//! One of the common directions for attack is to leak to the attacher 
//! some uninitialized data that might contain addresses that might guide
//! attacker how to bypass technics like Address Space Layout Randomization (ASLR).
//! To prevent that consider using fill_padding that will fill any unused space
//! with given pattern making sure we are not leaking any unintended information
//! in the element padding and in the buffer's unused tail.
//!
//! Interop with C API notes:
//! When passing pointer to container's buffer to an API that can modify it
//! it will invalidate container invariants. For instance pointer to the last 
//! element will no longer be valid. User must call revalidate_data after this
//! call to make sure invariants are fixed. The cal revalidate_data will 
//! rescan buffer for valid list as if it is a non-trusted buffer.
//!
//! Allocator notes:
//! COntainer supports adopting (attaching to) buffer that was allocated by
//! someone else. This helps to minimize number of copies when interoping with 
//! C API. It is responsibility of user to make sure that container uses allocator
//! that is compatible with how adopted buffer was allocated.
//!
template <typename T,
          typename TT = flat_forward_list_traits<T>,
          typename A = std::allocator<T>>
class flat_forward_list final {
public:

    //!
    //! @details Give flat_forward_list_ref friend permissins
    //! so it can initialize itself from the buffer
    //!
    template <typename TU,
              typename TTU>
    friend class flat_forward_list_ref;

    //
    // Technically we need T to be 
    // - trivialy destructable
    // - trivialy constructable
    // - trivialy movable
    // - trivialy copyable
    //
    static_assert(std::is_pod_v<T>, "T must be a Plain Old Definition");
    //!
    //! @typedef value_type
    //! @brief Element value type
    //!
    using value_type = T;
    //!
    //! @typedef pointer
    //! @brief Pointer to element type
    //!
    using pointer = T * ;
    //!
    //! @typedef const_pointer
    //! @brief Pointer to const element type
    //!
    using const_pointer = T const *;
    //!
    //! @typedef reference
    //! @brief Reference to the element type
    //!
    using reference = T & ;
    //!
    //! @typedef const_reference
    //! @brief Reference to the const element type
    //!
    using const_reference = T const &;
    //!
    //! @typedef size_type
    //! @brief Size type
    //!
    using size_type = std::size_t;
    //!
    //! @typedef difference_type
    //! @brief Element pointers difference type
    //!
    using difference_type = std::ptrdiff_t;
    //!
    //! @typedef traits
    //! @brief Element traits type
    //!
    using traits = TT;
    //!
    //! @typedef traits_traits
    //! @brief Element traits traits type
    //!
    using traits_traits = flat_forward_list_traits_traits<T, TT>;
    //!
    //! @typedef range_t
    //! @brief Vocabulary type used to describe
    //! buffer used by the element and how much
    //! of this buffer is used by the data.
    //! Type also includes information about 
    //! required alignment.
    //!
    using range_t = typename traits_traits::range_t;
    //!
    //! @typedef size_with_padding_t
    //! @brief Vocabulary type used to describe size with padding
    //! 
    using size_with_padding_t = typename traits_traits::size_with_padding_t;
    //!
    //! @typedef offset_with_aligment_t
    //! @brief Vocabulary type used to describe size with alignment
    //! 
    using offset_with_aligment_t = typename traits_traits::offset_with_aligment_t;
    //!
    //! @typedef sizes_t
    //! @brief Vocabulary type that contains information about buffer size, and last element range. 
    //!
    using sizes_t = flat_forward_list_sizes<traits_traits::alignment>;
    //!
    //! @typedef allocator_type
    //! @brief Type of allocator
    //! 
    using allocator_type = typename std::allocator_traits<A>::template rebind_alloc<char>;
    //!
    //! @typedef allocator_type_traits
    //! @brief Type of allocator traits
    //! 
    using allocator_type_traits = std::allocator_traits<allocator_type>;
    //!
    //! @typedef buffer_value_type
    //! @brief Since we have variable size elementa,
    //! and we cannot express it in the C++ type system
    //! we treat buffer with elements as a bag of chars
    //! and cast to the element type when nessesary.
    //!
    using buffer_value_type = char;
    //!
    //! @typedef const_buffer_value_type
    //! @brief When we do not intend to modify buffer 
    //! we can treat it as a bag of const characters
    //!
    using const_buffer_value_type = char const;
    //!
    //! @typedef buffer_pointer
    //! @brief Type used as a buffer pointer.
    //!
    using buffer_pointer = char *;
    //!
    //! @typedef const_buffer_pointer
    //! @brief Type used as a pointer ot the const buffer.
    //!
    using const_buffer_pointer = char const *;

    //!
    //! @typedef iterator
    //! @brief Type of iterator
    //!
    using iterator = flat_forward_list_iterator<T, TT>;
    //!
    //! @typedef const_iterator
    //! @brief Type of const iterator
    //!
    using const_iterator = flat_forward_list_const_iterator<T, TT>;
    //!
    //! @typedef buffer_type
    //! @brief Pointers that describe buffer
    //! @details Depending if T is const buffer uses char * or char const *
    //!
    using buffer_type = buffer_t<buffer_value_type>;
    //!
    //! @brief Constant that represents and invalid 
    //! or non-existent position
    //!
    inline static size_type const npos = iffl::npos;
    //!
    //! @brief Default constructor for container
    //!
    flat_forward_list() noexcept 
        : buffer_( zero_then_variadic_args_t{} ) {
    }
    //!
    //! @brief Constructs an empty container with an instance of
    //! provided allocator
    //!
    explicit flat_forward_list(allocator_type a) noexcept
        : buffer_( one_then_variadic_args_t{}, a ) {
    }
    //!
    //! @brief Move constructor. Moves allocator and content
    //! of other container to this container
    //! @param other - container we are moving from
    //!
    flat_forward_list(flat_forward_list && other) noexcept
        : buffer_( one_then_variadic_args_t{}, std::move(other.alloc()) ) {
        move_from(std::move(other));
    }
    //!
    //! @brief Copy constructor. Copies allocator if supprted and 
    //! copies content of other container to this container
    //! @param other - container we are copying from
    //! @throw std::bad_alloc if buffer allocation fails
    //!
    flat_forward_list(flat_forward_list const &other)
        : buffer_( one_then_variadic_args_t{}, allocator_type_traits::select_on_container_copy_construction(other.get_allocator()) ) {
        copy_from(other);
    }
    //!
    //! @brief Constructor that takes ownership of a buffer
    //! @tparam A - type of allocator.
    //! @param other_buff - pointer to the start of the buffer
    //! that contains list.
    //! @param a - allocator that should be used by this container.
    //! @details This constructor does not validate if this is
    //! a valid flat forward list. It assumes that
    //! caller validated buffer before using this constructor.
    //! The first parameter is an empty vocabulary type to help
    //! with overload resolution and code redability.
    //! @code
    //! iffl::flat_forward_list<my_type> new_owner{iffl::attach_buffer{}, begin, last, end};
    //! @endcode
    //! After this call container is responsible for deallocating 
    //! the buffer.
    //! It is responsibility of the caller to make sure that buffer
    //! was allocated using method compatible with allocator used by 
    //! this container.
    //!
    template <typename AA = allocator_type>
    flat_forward_list(attach_buffer,
                      buffer_view const &other_buff,
                      AA &&a = AA{}) noexcept
        : buffer_( one_then_variadic_args_t{}, std::forward<AA>(a) ) {
        attach(other_buff);
    }
    //!
    //! @brief Constructor that copies list from a buffer
    //! @tparam AA - type of allocator.
    //! @param other_buff - pointer to the start of the buffer
    //! that contains list.
    //! @param a - allocator that should be used by this container.
    //! @throw std::bad_alloc if buffer allocation fails
    //! @details This constructor does not validate if this is
    //! a valid flat forward list. It assumes that
    //! caller validated buffer before using this constructor.
    //!
    template <typename AA = allocator_type>
    explicit flat_forward_list(buffer_view const &other_buff,
                               AA &&a = AA{})
        : buffer_( one_then_variadic_args_t{}, std::forward<AA>(a) ) {
        assign(other_buff);
    }
    //!
    //! @brief Constructor that takes ownership of a buffer
    //! @tparam AA - type of allocator.
    //! @param buffer_begin - pointer to the start of the buffer
    //! that contains list.
    //! @param buffer_end - pointer to the address right 
    //! after last byte in the buffer.
    //! @param last_element - pointers to the last element of the
    //! list in the buffer. If buffer does not contain any elements
    //! then this parameter must be nullptr.
    //! @param a - allocator that should be used by this container.
    //! @details This constructor does not validate if this is
    //! a valid flat forward list. It assumes that
    //! caller validated buffer before using this constructor.
    //! The first parameter is an empty vocabulary type to help
    //! with overload resolution and code redability.
    //! @code
    //! iffl::flat_forward_list<my_type> new_owner{iffl::attach_buffer{}, begin, last, end};
    //! @endcode
    //! After this call container is responsible for deallocating 
    //! the buffer.
    //! It is responsibility of the caller to make sure that buffer
    //! was allocated using method compatible with allocator used by 
    //! this container.
    //!
    template <typename AA = allocator_type>
    flat_forward_list(attach_buffer,
                      char *buffer_begin,
                      char *last_element,
                      char *buffer_end,
                      AA &&a = AA{}) noexcept
        : buffer_( one_then_variadic_args_t{}, std::forward<AA>(a) ) {
        attach(buffer_begin, last_element, buffer_end);
    }
    //!
    //! @brief Constructor that copies list from a buffer
    //! @tparam AA - type of allocator.
    //! @param buffer_begin - pointer to the start of the buffer
    //! that contains list.
    //! @param buffer_end - pointer to the address right 
    //! after last byte in the buffer.
    //! @param last_element - pointers to the last element of the
    //! list in the buffer. If buffer does not contain any elements
    //! then this parameter must be nullptr.
    //! @param a - allocator that should be used by this container.
    //! @throw std::bad_alloc if buffer allocation fails
    //! @details This constructor does not validate if this is
    //! a valid flat forward list. It assumes that
    //! caller validated buffer before using this constructor.
    //!
    template <typename AA = allocator_type>
    flat_forward_list(char const *buffer_begin,
                      char const *last_element,
                      char const *buffer_end,
                      AA &&a = AA{})
        : buffer_( one_then_variadic_args_t{}, std::forward<AA>(a) ) {
        assign(buffer_begin, last_element, buffer_end);
    }
    //!
    //! @brief Constructor that takes ownership of a buffer
    //! and attempts to find last element of the list.
    //! @tparam AA - type of allocator.
    //! @param buffer - pointer to the start of the buffer
    //! that might contains list.
    //! @param buffer_size - buffer size.
    //! @param a - allocator that should be used by this container.
    //! @details This constructor adopts buffer, and searches for the last
    //! valid element in the buffer. If buffer validation fails then
    //! container will treat it as if it has no elements.
    //! The first parameter is an empty vocabulary type to help
    //! with overload resolution and code redability.
    //! @code
    //! iffl::flat_forward_list<my_type> new_owner{iffl::attach_buffer{}, begin, size};
    //! @endcode
    //! After this call container is responsible for deallocating 
    //! the buffer.
    //! It is responsibility of the caller to make sure that buffer
    //! was allocated using method compatible with allocator used by 
    //! this container.
    //!
    template <typename AA = allocator_type>
    flat_forward_list(attach_buffer,
                      char *buffer,
                      size_t buffer_size,
                      AA &&a = AA{}) noexcept
        : buffer_( one_then_variadic_args_t{}, std::forward<AA>(a) ) {
        attach(buffer, buffer_size);
    }
    //!
    //! @brief Constructor that checks if buffer contains a valid list
    //! and if it does then copies that list.
    //! @tparam AA - type of allocator.
    //! @param buffer - pointer to the start of the buffer
    //! that might contains list.
    //! @param buffer_size - bufer size.
    //! @param a - allocator that should be used by this container.
    //! @throw std::bad_alloc if buffer allocation fails
    //! @details This constructor searches for the last
    //! valid element in the buffer, and is buffer is valid then it
    //! copies elements to the new buffer.
    //!
    template <typename AA = allocator_type>
    flat_forward_list(char const *buffer,
                      size_t buffer_size,
                      AA &&a = AA{})
        : buffer_( one_then_variadic_args_t{}, std::forward<AA>(a)) {
        assign(buffer, buffer_size);
    }
    //!
    //! @brief Copies element pointed by the iterators.
    //! @param begin - iterator for the buffer begin.
    //! @param last - last element that should be included in the view.
    //! @param a - allocator that should be used by this container.
    //! @throw std::bad_alloc if buffer allocation fails
    //!
    template <typename AA = allocator_type>
    flat_forward_list(const_iterator const &begin,
                      const_iterator const &last,
                      AA &&a = AA{})
        : buffer_(one_then_variadic_args_t{}, std::forward<AA>(a)) {
        assign(begin, last);
    }
    //!
    //! @brief Copies element from the view.
    //! @param view - view over a flat forward list buffer.
    //! @param a - allocator that should be used by this container.
    //! @throw std::bad_alloc if buffer allocation fails
    //!
    template <typename AA = allocator_type>
    flat_forward_list(flat_forward_list_view<T, TT> const &view,
                      AA &&a = AA{})
        : buffer_(one_then_variadic_args_t{}, std::forward<AA>(a)) {
        assign(view);
    }
    //!
    //! @brief Move assignment operator.
    //! @param other - linked list we are moving from
    //! @throw noexcept if allocator is propagate_on_container_move_assignment
    //! @details If allocator is propagate_on_container_move_assignment then
    //! it moves allocator, and moves buffer ownership.
    //! Otherwise if allocator are equivalent then data are moves.
    //! If allocators are not equivalent then new buffer is allocated and 
    //! list is copied. In the last case this function might throw std::bad_alloc
    //!
    flat_forward_list &operator= (flat_forward_list && other) noexcept (allocator_type_traits::propagate_on_container_move_assignment::value) {
        if (this != &other) {
            if constexpr (allocator_type_traits::propagate_on_container_move_assignment::value) {
                move_allocator_from(std::move(other));
                move_from(std::move(other));
            } else {
                try_move_from(std::move(other));
            }
        }
        return *this;
    }
    //!
    //! @brief Copy assignment operator.
    //! @param other - linked list we are copying from
    //! @throw std::bad_alloc when allocating buffer fails
    //! @details This function first attempts to copy allocator, 
    //! if it is supported, and after that allocates new buffer
    //! and copies all elements to the new buffer.
    //!
    flat_forward_list &operator= (flat_forward_list const &other) {
        if (this != &other) {
            //
            // If we are propagating allocator then delete old data
            // before changing allocator
            //
            if constexpr (allocator_type_traits::propagate_on_container_copy_assignment::value) {
                clear();
                *static_cast<allocator_type *>(this) = allocator_type_traits::select_on_container_copy_construction(other.get_allocator());
            }
            
            copy_from(other);
        }
        return *this;
    }
    //!
    //! @brief Copy assignment operator.
    //! @param other_buff - linked list we are copying from
    //! @throw std::bad_alloc when allocating buffer fails
    //! @details This function first attempts to copy allocator, 
    //! if it is supported, and after that allocates new buffer
    //! and copies all elements to the new buffer.
    //!
    flat_forward_list &operator= (buffer_view const &other_buff) {
        assign(other_buff);
        return *this;
    }
    //!
    //! @brief Destructor.
    //! @details Deallocates buffer owned by container.
    //!
    ~flat_forward_list() noexcept {
        clear();
    }
    //!
    //! @brief Container releases ownership of the buffer.
    //! @details After the call completes, container is empty.
    //! @return Returns buffer information to the caller.
    //! Caller is responsible for deallocating returned buffer.
    //!
    buffer_ref detach() noexcept {
        buffer_ref tmp{ buff() };
        buff().clear();
        return tmp;
    }
    //!
    //! @brief Takes ownership of a buffer
    //! @param other_buff - describes the buffer we are attaching.
    //! @details This method first clears existing content of 
    //! the container. It does not validate if buffer contains
    //! a valid flat forward list. It assumes that
    //! caller validated buffer before using this constructor.
    //! After this call container is responsible for deallocating 
    //! the buffer.
    //! It is responsibility of the caller to make sure that buffer
    //! was allocated using method compatible with allocator used by 
    //! this container.
    //!
    void attach(buffer_view const &other_buff) {
        FFL_CODDING_ERROR_IF(buff().begin == other_buff.begin);
        other_buff.validate();
        clear();
        buff() = other_buff;
    }
    //!
    //! @brief Takes ownership of a buffer
    //! @param buffer_begin - pointer to the start of the buffer
    //! that contains list.
    //! @param buffer_end - pointer to the address right 
    //! after last byte in the buffer.
    //! @param last_element - pointers to the last element of the
    //! list in the buffer. If buffer does not contain any elements
    //! then this parameter must be nullptr.
    //! @details This method first clears existing content of 
    //! the container. It does not validate if buffer contains
    //! a valid flat forward list. It assumes that
    //! caller validated buffer before using this constructor.
    //! After this call container is responsible for deallocating 
    //! the buffer.
    //! It is responsibility of the caller to make sure that buffer
    //! was allocated using method compatible with allocator used by 
    //! this container.
    //!
    void attach(char *buffer_begin,
                char *last_element,
                char *buffer_end) noexcept {

        FFL_CODDING_ERROR_IF(buff().begin == buffer_begin);
        if (last_element) {
            FFL_CODDING_ERROR_IF_NOT(buffer_begin < last_element && last_element < buffer_end);
        } else {
            FFL_CODDING_ERROR_IF_NOT(buffer_begin < buffer_end);
        }
        clear();
        buff().begin = buffer_begin;
        buff().end = buffer_end;
        buff().last = last_element;
    }
    //!
    //! @brief Takes ownership of a buffer
    //! and attempts to find last element of the list.
    //! @param buffer - pointer to the start of the buffer
    //! that might contains list.
    //! @param buffer_size - buffer size.
    //! @details This method adopts buffer, and searches for the last
    //! valid element in the buffer. If buffer validation fails then
    //! container will treat it as if it has no elements.
    //! After this call container is responsible for deallocating 
    //! the buffer.
    //! It is responsibility of the caller to make sure that buffer
    //! was allocated using method compatible with allocator used by 
    //! this container.
    //!
    bool attach(char *buffer,
                size_t buffer_size) noexcept {
        FFL_CODDING_ERROR_IF(buff().begin == buffer);
        auto [is_valid, buffer_view] = flat_forward_list_validate<T, TT>(buffer, 
                                                                         buffer + buffer_size);
        attach(buffer, 
               is_valid ? buffer_view.last().get_ptr() : nullptr,
               buffer + buffer_size);
        return is_valid;
    }
    //!
    //! @brief Copies list from a buffer
    //! @param other_buff - describes the other buffer.
    //! @throw std::bad_alloc if buffer allocation fails
    //! @details This method does not validate if this is
    //! a valid flat forward list. It assumes that
    //! caller validated buffer before using this constructor.
    //!
    void assign(buffer_view const &other_buff) {
        flat_forward_list l(get_allocator());
        other_buff.validate();

        l.buff().begin = allocate_buffer(other_buff.size());
        copy_data(l.buff().begin, other_buff.begin, other_buff.size());
        l.buff().end = l.buff().begin + other_buff.size();
        l.buff().last = l.buff().begin + other_buff.last_offset();
        swap(l);
    }
    //!
    //! @brief Copies list from a buffer
    //! @param buffer_begin - pointer to the start of the buffer
    //! that contains list.
    //! @param buffer_end - pointer to the address right 
    //! after last byte in the buffer.
    //! @param last_element - pointers to the last element of the
    //! list in the buffer. If buffer does not contain any elements
    //! then this parameter must be nullptr.
    //! @throw std::bad_alloc if buffer allocation fails
    //! @details This method does not validate if this is
    //! a valid flat forward list. It assumes that
    //! caller validated buffer before using this constructor.
    //!
    void assign(char const *buffer_begin, 
                char const *last_element, 
                char const *buffer_end) {
        flat_forward_list l(get_allocator());

        if (last_element) {
            FFL_CODDING_ERROR_IF_NOT(buffer_begin < last_element && last_element < buffer_end);
        } else {
            FFL_CODDING_ERROR_IF_NOT(buffer_begin < buffer_end);
        }

        size_type buffer_size = buffer_end - buffer_begin;
        size_type last_element_offset = last_element - buffer_begin;

        l.buff().begin = allocate_buffer(buffer_size);
        copy_data(l.buff().begin, buffer_begin, buffer_size);
        l.buff().end = l.buff().begin + buffer_size;
        l.buff().last = l.buff().begin + last_element_offset;
        swap(l);
    }
    //!
    //! @brief Checks if buffer contains a valid list
    //! and if it does then copies that list.
    //! @param buffer_begin - pointer to the start of the buffer
    //! that might contains list.
    //! @param buffer_end - pointer to the buffer end.
    //! @throw std::bad_alloc if buffer allocation fails
    //! @details This method searches for the last
    //! valid element in the buffer, and is buffer is valid then it
    //! copies elements to the new buffer.
    //!
    bool assign(char const *buffer_begin, 
                char const *buffer_end) {

        auto[is_valid, buffer_view] = flat_forward_list_validate<T, TT>(buffer_begin,
                                                                        buffer_end);
        assign(buffer_begin,
               is_valid ? buffer_view.last().get_ptr() : nullptr,
               buffer_end);

        return is_valid;
    }
    //!
    //! @brief Checks if buffer contains a valid list
    //! and if it does then copies that list.
    //! @param buffer - pointer to the start of the buffer
    //! that might contains list.
    //! @param buffer_size - bufer size.
    //! @throw std::bad_alloc if buffer allocation fails
    //! @details This method searches for the last
    //! valid element in the buffer, and is buffer is valid then it
    //! copies elements to the new buffer.
    //!
    bool assign(char const *buffer,
                size_type buffer_size) {

        return assign(buffer, buffer + buffer_size);
    }
    //!
    //! @brief Copies element pointed by the iterators.
    //! @param begin - iterator for the buffer begin.
    //! @param last - last element that should be included in the view.
    //!
    void assign(const_iterator const &begin,
                const_iterator const &last) {
        
        validate_iterator_not_end(begin);
        validate_iterator_not_end(last);

        flat_forward_list new_list(get_allocator());
        new_list.resize_buffer(distance(begin.get_ptr(),
                                        last.get_ptr()) + traits_traits::get_size(last.get_ptr()).size);
        const_iterator const end = last + 1;
        for (const_iterator const it = begin; it != end; ++it) {
            new_list.push_back(used_size(it), it.get_ptr());
        }
        //
        // Swap with new list
        //
        swap(new_list);
    }
    //!
    //! @brief Copies elements from the view
    //! @param view - view over the flat forward list
    //!
    void assign(flat_forward_list_view<T, TT> const &view) {
        if (!view.empty()) {
            const_iterator const &begin{ view.begin() };
            const_iterator const &last{ view.end() };
            flat_forward_list new_list(get_allocator());
            new_list.resize_buffer(distance(view.begin().get_ptr(),
                                            last.get_ptr()) + traits_traits::get_size(last.get_ptr()).size);
            const_iterator const end = last + 1;
            for (const_iterator const it = begin; it != end; ++it) {
                new_list.push_back(used_size(it), it.get_ptr());
            }
            //
            // Swap with new list
            //
            swap(new_list);
        } else {
            clear();
        }
    }
    //!
    //! @brief Compares if allocator used by container is 
    //! equivalent to the other alocator.
    //! @param other_allocator - allocator we are comparing to
    //! @return True if allocators are equivalent, and  false otherwise.
    //!
    template<typename AA>
    bool is_compatible_allocator(AA const &other_allocator) const noexcept {
        return other_allocator == get_allocator();
    }
    //!
    //! @brief Returns reference to the allocator used by this container
    //! @return Allocator reference.
    //!
    allocator_type &get_allocator() & noexcept {
        return alloc();
    }
    //!
    //! @brief Returns const reference to the allocator used by this container
    //! @return Allocator const reference.
    //!
    allocator_type const &get_allocator() const & noexcept {
        return alloc();
    }
    //!
    //! @brief Returns maximum size.
    //! @return Maximum size.
    //! @details For this container it is maximum number of bytes that can 
    //! be allocated through allocator used by this container
    //!
    size_type max_size() const noexcept {
        return allocator_type_traits::max_size(get_allocator());
    }
    //!
    //! @brief Deallocated buffer owned by this container.
    //! @details After this call container is empty
    //!
    void clear() noexcept {
        validate_pointer_invariants();
        if (buff().begin) {
            deallocate_buffer(buff().begin, total_capacity());
            buff().clear();
        }
        validate_pointer_invariants();
    }
    //!
    //! @brief Resizes buffer to the used capacity.
    //! @details Allocates new buffer of exact size 
    //! that is required to keep all elements, moves
    //! all elements there and deallocates old buffer.
    //! If buffer already has no unused capacity then
    //! this call is noop.
    //!
    void tail_shrink_to_fit() {
        resize_buffer(used_capacity());
    }
    //!
    //! @brief Resizes buffer.
    //! @param size - new buffer size
    //!               Passing 0 has same effect as clearing container
    //!               Setting buffer size to the value smaller than first element size
    //!               will produce empty list.
    //! @throw std::bad_alloca if allocating new buffer fails
    //! @details Allocates new buffer of specified size, and
    //! copies all elements that can fit to the new buffer size.
    //! all other elements will be erased.
    //! Resizing buffer to the larger capacity will add unused capacity.
    //! This can help reduce buffer reallocations when adding new elements to
    //! the buffer.
    //! Resizing to capacity smaller than used capacity will end up erasing
    //! elements that do not fit to the new buffer size.
    //!
    void resize_buffer(size_type size) {
        validate_pointer_invariants();

        sizes_t const prev_sizes{ get_all_sizes() };

        char *new_buffer{ nullptr };
        size_t new_buffer_size{ 0 };
        auto deallocate_buffer{ make_scoped_deallocator(&new_buffer, &new_buffer_size) };
        //
        // growing capacity
        //
        if (prev_sizes.total_capacity < size) {
            new_buffer = allocate_buffer(size);
            new_buffer_size = size;
            if (nullptr != buff().last) {
                copy_data(new_buffer, buff().begin, prev_sizes.used_capacity().size);
                buff().last = new_buffer + prev_sizes.last_element.begin();
            }
            commit_new_buffer(new_buffer, new_buffer_size);
            buff().end = buff().begin + size;
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

            bool is_valid{ true };
            char *last_valid{ buff().last };
            //
            // If we are shrinking below used capacity then last element will
            // be removed, and we need to do linear search for the new last element 
            // that would fit new buffer size.
            //
            if (prev_sizes.used_capacity().size > size) {
                flat_forward_list_ref<T, TT> buffer_view;
                std::tie(is_valid, buffer_view) = flat_forward_list_validate<T, TT>(buff().begin,
                                                                                    buff().begin + size);
                if (is_valid) {
                    FFL_CODDING_ERROR_IF_NOT(buffer_view.last().get_ptr() == buff().last);
                } else {
                    FFL_CODDING_ERROR_IF_NOT(buffer_view.last().get_ptr() != buff().last);
                }
                last_valid = buffer_view.last().get_ptr();
            }

            if (last_valid) {
                size_type new_last_element_offset = last_valid - buff().begin;

                size_with_padding_t const last_valid_element_size{ traits_traits::get_size(last_valid) };
                size_type const new_used_capacity{ new_last_element_offset + last_valid_element_size.size };
                
                set_no_next_element(last_valid);
                
                copy_data(new_buffer, buff().begin, new_used_capacity);
                buff().last = new_buffer + new_last_element_offset;
            } else {
                buff().last = nullptr;
            }

            commit_new_buffer(new_buffer, new_buffer_size);
            buff().end = buff().begin + size;
        }

        validate_pointer_invariants();
        validate_data_invariants();
    }
    //!
    //! @brief Adds new elemnt to the end of the list. 
    //! Element is initialized by coppying provided buffer.
    //! @param init_buffer_size - size of the buffer that will be used for initialization 
    //!                           size smaller than minimum element size will trigger a fail-fast.
    //! @param init_buffer - a poiner to the buffer. If pointer to the buffer is nullptr then
    //!                      element data are zero initialized.
    //! @throw std::bad_alloca if allocating new buffer fails
    //! @details New element becomes a new last element and, if set_next offset is supported,
    //! then it is called on the previous last element such that it points to the new one.
    //!
    void push_back(size_type init_buffer_size,
                   char const *init_buffer = nullptr) {

        emplace_back(init_buffer_size,
                      [init_buffer_size, init_buffer](T &buffer,
                                                      size_type element_size) {
                        FFL_CODDING_ERROR_IF_NOT(init_buffer_size == element_size);
                        if (init_buffer) {
                            copy_data(reinterpret_cast<char *>(&buffer), init_buffer, element_size);
                        } else {
                            zero_buffer(reinterpret_cast<char *>(&buffer), element_size);
                        }
                     });
    }
    //!
    //! @brief Adds new elemnt to the end of the list. 
    //! Element is initialized by coppying provided buffer.
    //! @param init_buffer_size - size of the buffer that will be used for initialization 
    //!                           size smaller than minimum element size will trigger a fail-fast.
    //! @param init_buffer - a poiner to the buffer. If pointer to the buffer is nullptr then
    //!                      element data are zero initialized.
    //! @throw std::bad_alloca if allocating new buffer fails
    //! @details New element becomes a new last element and, if set_next offset is supported,
    //! then it is called on the previous last element such that it points to the new one.
    //! @returns true if element was placed in the existing buffer and false if
    //! buffer does not have enough capacity for the new element.
    //!
    bool try_push_back(size_type init_buffer_size,
                       char const *init_buffer = nullptr) {

        return try_emplace_back(init_buffer_size,
                                [init_buffer_size, init_buffer](T &buffer,
                                                                size_type element_size) {
                                  FFL_CODDING_ERROR_IF_NOT(init_buffer_size == element_size);
                                  if (init_buffer) {
                                      copy_data(reinterpret_cast<char *>(&buffer), init_buffer, element_size);
                                  } else {
                                      zero_buffer(reinterpret_cast<char *>(&buffer), element_size);
                                  }
                               });
    }
    //!
    //! @brief Constructs new element at the end of the list. 
    //! @tparam F - type of a functor
    //! Element is initialized with a help of the functor passed as a parameter.
    //! @param element_size - number of bytes required for the new element.
    //! @param fn - a functor used to construct new element.
    //! @throw std::bad_alloca if allocating new buffer fails.
    //!        Any exceptions that might be raised by the functor.
    //!        If functor raises then container remains in the state as if 
    //!        call did not happen
    //! @details Constructed element does not have to use up the entire buffer
    //! unused space will become unused buffer capacity.
    //!
    template <typename F>
    void emplace_back(size_type element_size,
                      F const &fn) {
        bool const result{ try_emplace_back_impl(can_reallocate::yes, element_size, fn) };
        FFL_CODDING_ERROR_IF(!result);
    }

    //!
    //! @brief Constructs new element at the end of the list if it fits 
    //! in the existing free capacity.
    //! @tparam F - type of a functor
    //! Element is initialized with a help of the functor passed as a parameter.
    //! @param element_size - number of bytes required for the new element.
    //! @param fn - a functor used to construct new element.
    //! @returns true if element was placed in the existing buffer and false if
    //! buffer does not have enough capacity for the new element.
    //! @details Constructed element does not have to use up the entire buffer
    //! unused space will become unused buffer capacity.
    //!
    template <typename F>
    bool try_emplace_back(size_type element_size,
                          F const &fn) {
        return try_emplace_back_impl(can_reallocate::no, element_size, fn);
    }

    //!
    //! @brief Removes last element from the list. 
    //! @details Tnis method has cost O(number of elements). 
    //! Cost comes from scanning buffer from beginning to find
    //! element before the current last element. Since this is
    //! a single linked list we have no faster way to locate it.
    //! Element before current last element will become new last 
    //! element.
    //!
    void pop_back() noexcept {
        validate_pointer_invariants();

        FFL_CODDING_ERROR_IF(empty_unsafe());
        
        if (has_exactly_one_entry()) {
            //
            // The last element is also the first element
            //
            buff().last = nullptr;
        } else {
            //
            // Find element before last
            //
            size_t const last_element_start_offset{ static_cast<size_t>(buff().last - buff().begin) };
            iterator element_before_it{ find_element_before(last_element_start_offset) };
            //
            // We already handled the case when last element is first element.
            // In this branch we must find an element before last.
            //
            FFL_CODDING_ERROR_IF(end() == element_before_it);
            //
            // Element before last is the new last
            //
            set_no_next_element(element_before_it.get_ptr());
            buff().last = element_before_it.get_ptr();
        }
        
        validate_pointer_invariants();
        validate_data_invariants();
    }
    //!
    //! @brief Inserts new element at the position described by iterator. 
    //! Element is initialized by coppying provided buffer.
    //! @param it - iterator pointing to the position new element should be inserte to.
    //!             Iterator value must be a valid iterator pointing to one of the elements
    //!             or an end iterator.
    //!             If iterator is pointing to an element that is not propertly alligned then
    //!             new element will not be propertly alligned.
    //! @param init_buffer_size - size of the buffer that will be used for initialization 
    //!                           size smaller than minimum element size will trigger a fail-fast.
    //!                           When iterator in not then end then size is padded to make sure that if
    //!                           this element is propertly alligned then next element is also property 
    //!                           aligned.
    //! @param init_buffer - a poiner to the buffer. If pointer to the buffer is nullptr then
    //!                      element data are zero initialized.
    //! @throw std::bad_alloca if allocating new buffer fails.
    //! @details New element becomes a new last element and, if set_next offset is supported,
    //! then it is called on the previous last element such that it points to the new one.
    //!
    iterator insert(iterator const &it, size_type init_buffer_size, char const *init_buffer = nullptr) {
        emplace(it, 
                init_buffer_size,
                [init_buffer_size, init_buffer](T &buffer,
                                                size_type element_size) {
                    FFL_CODDING_ERROR_IF_NOT(init_buffer_size == element_size);

                    if (init_buffer) {
                        copy_data(reinterpret_cast<char *>(&buffer), init_buffer, element_size);
                    } else {
                        zero_buffer(reinterpret_cast<char *>(&buffer), element_size);
                    }
                });
    }
    //!
    //! @brief Inserts new element at the position described by iterator. 
    //! Element is initialized by coppying provided buffer.
    //! @param it - iterator pointing to the position new element should be inserte to.
    //!             Iterator value must be a valid iterator pointing to one of the elements
    //!             or an end iterator.
    //!             If iterator is pointing to an element that is not propertly alligned then
    //!             new element will not be propertly alligned.
    //! @param init_buffer_size - size of the buffer that will be used for initialization 
    //!                           size smaller than minimum element size will trigger a fail-fast.
    //!                           When iterator in not then end then size is padded to make sure that if
    //!                           this element is propertly alligned then next element is also property 
    //!                           aligned.
    //! @param init_buffer - a poiner to the buffer. If pointer to the buffer is nullptr then
    //!                      element data are zero initialized.
    //! @returns true if element was placed in the existing buffer and false if
    //! buffer does not have enough capacity for the new element.
    //! @details New element becomes a new last element and, if set_next offset is supported,
    //! then it is called on the previous last element such that it points to the new one.
    //!
    bool try_insert(iterator const &it, size_type init_buffer_size, char const *init_buffer = nullptr) {
        return try_emplace(it, 
                           init_buffer_size,
                           [init_buffer_size, init_buffer](T &buffer,
                                                           size_type element_size) {
                               FFL_CODDING_ERROR_IF_NOT(init_buffer_size == element_size);
                               if (init_buffer) {
                                   copy_data(reinterpret_cast<char *>(&buffer), init_buffer, element_size);
                               } else {
                                   zero_buffer(reinterpret_cast<char *>(&buffer), element_size);
                               }
                           });
    }
    //!
    //! @brief Constructs new element at the position described by iterator. 
    //! Element is initialized with a help of the functor passed as a parameter.
    //! @tparam F - type of a functor
    //! @param it - iterator pointing to the position new element should be inserte to.
    //!             Iterator value must be a valid iterator pointing to one of the elements
    //!             or an end iterator.
    //!             If iterator is pointing to an element that is not propertly alligned then
    //!             new element will not be propertly alligned.
    //!             When iterator in not then end then size is padded to make sure that if
    //!             this element is propertly alligned then next element is also property 
    //!             aligned.
    //! @param new_element_size - number of bytes required for the new element.
    //! @param fn - a functor used to construct new element.
    //! @returns Returns a pair of boolean that tells caller if element was inserted, and 
    //! iterator to the position where it should be inserted in case of failure or was 
    //! inserted in case of success.
    //! @throw std::bad_alloca if allocating new buffer fails.
    //!        Any exceptions that might be raised by the functor.
    //!        If functor raises then container remains in the state as if 
    //!        call did not happen
    //! @details Constructed element does not have to use up the entire buffer
    //! unused space will become unused buffer capacity.
    //!
    template <typename F>
    iterator emplace(iterator const &it,
                     size_type new_element_size,
                     F const &fn) {
        auto [result, new_it] =  try_emplace_impl(can_reallocate::yes,
                                                  it,
                                                  new_element_size,
                                                  fn);
        FFL_CODDING_ERROR_IF(!result);
        return new_it;
    }
    //!
    //! @brief Constructs new element at the position described by iterator. 
    //! Element is initialized with a help of the functor passed as a parameter.
    //! @tparam F - type of a functor
    //! @param it - iterator pointing to the position new element should be inserte to.
    //!             Iterator value must be a valid iterator pointing to one of the elements
    //!             or an end iterator.
    //!             If iterator is pointing to an element that is not propertly alligned then
    //!             new element will not be propertly alligned.
    //!             When iterator in not then end then size is padded to make sure that if
    //!             this element is propertly alligned then next element is also property 
    //!             aligned.
    //! @param new_element_size - number of bytes required for the new element.
    //! @param fn - a functor used to construct new element.
    //! @returns Returns a pair of boolean that tells caller if element was inserted, and 
    //! iterator to the position where it should be inserted in case of failure or was 
    //! inserted in case of success.
    //! @returns true if element was placed in the existing buffer and false if
    //! buffer does not have enough capacity for the new element.
    //! @details Constructed element does not have to use up the entire buffer
    //! unused space will become unused buffer capacity.
    //!
    template <typename F>
    bool try_emplace(iterator const &it,
                     size_type new_element_size,
                     F const &fn) {
        auto[result, new_it] = try_emplace_impl(can_reallocate::no,
                                                it,
                                                new_element_size,
                                                fn);
        FFL_CODDING_ERROR_IF_NOT(it == new_it);
        return result;
    }
    //!
    //! @brief Constructs new element at the beginnig of the container. 
    //! Element is initialized by coppying provided buffer.
    //! @param init_buffer_size - size of the buffer that will be used for initialization 
    //!                           size smaller than minimum element size will trigger a fail-fast.
    //!                           When container is not empty then size is padded to make sure that
    //!                           next element is also property aligned.
    //! @param init_buffer - a poiner to the buffer. If pointer to the buffer is nullptr then
    //!                      element data are zero initialized.
    //! @throw std::bad_alloca if allocating new buffer fails.
    //!
    void push_front(size_type init_buffer_size, char const *init_buffer = nullptr) {
        emplace(begin(),
                init_buffer_size,
                [init_buffer_size, init_buffer](T &buffer,
                                                size_type element_size) {
                    FFL_CODDING_ERROR_IF_NOT(init_buffer_size == element_size);

                    if (init_buffer) {
                        copy_data(reinterpret_cast<char *>(&buffer), init_buffer, element_size);
                    } else {
                        zero_buffer(reinterpret_cast<char *>(&buffer), element_size);
                    }
                });
    }
    //!
    //! @brief Constructs new element at the beginnig of the container. 
    //! Element is initialized by coppying provided buffer.
    //! @param init_buffer_size - size of the buffer that will be used for initialization 
    //!                           size smaller than minimum element size will trigger a fail-fast.
    //!                           When container is not empty then size is padded to make sure that
    //!                           next element is also property aligned.
    //! @param init_buffer - a poiner to the buffer. If pointer to the buffer is nullptr then
    //!                      element data are zero initialized.
    //! @returns true if element was placed in the existing buffer and false if
    //! buffer does not have enough capacity for the new element.
    //!
    bool try_push_front(size_type init_buffer_size, char const *init_buffer = nullptr) {
        return try_emplace(begin(),
                           init_buffer_size,
                           [init_buffer_size, init_buffer](T &buffer,
                                                           size_type element_size) {
                               FFL_CODDING_ERROR_IF_NOT(init_buffer_size == element_size);
                               if (init_buffer) {
                                   copy_data(reinterpret_cast<char *>(&buffer), init_buffer, element_size);
                               } else {
                                   zero_buffer(reinterpret_cast<char *>(&buffer), element_size);
                               }
                           });
    }
    //!
    //! @brief Inserts new element at the beginnig of the container. 
    //! Element is initialized with a help of the functor passed as a parameter.
    //! @tparam F - type of a functor
    //! @param element_size - size of the buffer that will be used for initialization 
    //!                           size smaller than minimum element size will trigger a fail-fast.
    //!                           When container is not empty then size is padded to make sure that
    //!                           next element is also property aligned.
    //! @param fn - a functor used to construct new element.
    //! @throw std::bad_alloca if allocating new buffer fails.
    //!        Any exceptions that might be raised by the functor.
    //!        If functor raises then container remains in the state as if 
    //!        call did not happen
    //!
    template <typename F>
    void emplace_front(size_type element_size, 
                       F const &fn) {
        emplace(begin(), element_size, fn);
    }
    //!
    //! @brief Inserts new element at the beginnig of the container. 
    //! Element is initialized with a help of the functor passed as a parameter.
    //! @tparam F - type of a functor
    //! @param element_size - size of the buffer that will be used for initialization 
    //!                           size smaller than minimum element size will trigger a fail-fast.
    //!                           When container is not empty then size is padded to make sure that
    //!                           next element is also property aligned.
    //! @param fn - a functor used to construct new element.
    //! @returns true if element was placed in the existing buffer and false if
    //! buffer does not have enough capacity for the new element.
    //!
    template <typename F>
    bool try_emplace_front(size_type element_size, 
                           F const &fn) {
        return try_emplace(begin(), element_size, fn);
    }
    //!
    //! @brief Removes first element from the container. 
    //! @details Calling on empty container will trigger fail-fast
    //!
    void pop_front() noexcept {
        validate_pointer_invariants();

        FFL_CODDING_ERROR_IF(empty_unsafe());
        //
        // If we have only one element then simply forget it
        //
        if (has_one_or_no_entry()) {
            buff().last = nullptr;
            return;
        }
        //
        // Otherwise calculate sizes and offsets
        //
        sizes_t const prev_sizes{ get_all_sizes() };
        iterator const begin_it{ iterator{ buff().begin } };
        iterator const secont_element_it{ begin_it + 1 };
        range_t const second_element_range{ this->range_unsafe(secont_element_it) };
        size_type const bytes_to_copy{ prev_sizes.used_capacity().size - second_element_range.begin() };
        //
        // Shift all elements after the first element
        // to the start of buffer
        //
        move_data(buff().begin, buff().begin + second_element_range.begin(), bytes_to_copy);

        buff().last -= second_element_range.begin();

        validate_pointer_invariants();
        validate_data_invariants();
    }
    //!
    //! @brief Erases element after the element pointed by the iterator
    //! @param it - iterator pointing to an element before the element
    //! that will be erased. If iterator is pointing to the last element
    //! of container then it will trigger fail-fast.
    //!
    void erase_after(iterator const &it) noexcept {
        validate_pointer_invariants();
        //
        // Canot erase after end. Should we noop or fail?
        //
        validate_iterator_not_end(it);
        //
        // There is no elements after last
        //
        FFL_CODDING_ERROR_IF( last() == it);
        FFL_CODDING_ERROR_IF(empty_unsafe());
        //
        // Find pointer to the element that we are erasing
        //
        iterator element_to_erase_it = it;
        ++element_to_erase_it;
        bool const erasing_last_element = (last() == element_to_erase_it);

        if (erasing_last_element) {
            //
            // trivial case when we are deleting last element and this element 
            // is becoming last
            //
            set_no_next_element(it.get_ptr());
            buff().last = it.get_ptr();
        } else {
            //
            // calculate sizes and offsets
            //
            sizes_t const prev_sizes{ get_all_sizes() };
            range_t const element_to_erase_range{ this->range_unsafe(element_to_erase_it) };
            size_type const tail_size{ prev_sizes.used_capacity().size - element_to_erase_range.buffer_end };
            //
            // Shift all elements after the element that we are erasing
            // to the position where erased element used to be
            //
            move_data(buff().begin + element_to_erase_range.begin(),
                      buff().begin + element_to_erase_range.buffer_end, tail_size
            );
            buff().last -= element_to_erase_range.buffer_size();
        }

        validate_pointer_invariants();
        validate_data_invariants();
    }

    //!
    //! @brief Erases element in the range (before_start, last]
    //! @param before_start - iterator pointing to an element before the first element
    //! that will be erased. If iterator is pointing to the last element
    //! of container then it will trigger fail-fast.
    //! @param last - iterator pointing to the last element to be erased.
    //! if this iterator is end then it will irase all elements after before_start.
    //! @details Usualy pair of iterators define aa half opened range [start, end)
    //! Note that in this case our range is (start, last]. In other words
    //! we will erase all elements after start, including last.
    //!
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
        FFL_CODDING_ERROR_IF(before_start == this->last());
        //
        // If we are truncating entire tail
        //
        if (end() == last) {
            erase_all_after(before_start);
            return;
        }
        //
        // We can support before_start == last by simply calling erase(last).
        // That will change complexity of algorithm by adding extra O(N).
        //
        FFL_CODDING_ERROR_IF_NOT(before_start < last);

        //
        // Find pointer to the element that we are erasing
        //
        iterator first_element_to_erase_it = before_start;
        ++first_element_to_erase_it;
        //
        // calculate sizes and offsets
        //
        sizes_t const prev_sizes{ get_all_sizes() };

        range_t const first_element_to_erase_range{ this->range_unsafe(first_element_to_erase_it) };
        range_t const last_element_to_erase_range{ this->range_unsafe(last) };

        //
        // Note that element_range returns element size adjusted with padding
        // that is required for the next element to be propertly aligned.
        //
        size_type const bytes_to_copy{ prev_sizes.used_capacity().size - last_element_to_erase_range.buffer_end };
        size_type const bytes_erased{ last_element_to_erase_range.buffer_end - first_element_to_erase_range.begin() };
        //
        // Shift all elements after the last element that we are erasing
        // to the position where first erased element used to be
        //
        move_data(buff().begin + first_element_to_erase_range.begin(),
                  buff().begin + last_element_to_erase_range.buffer_end,
                  bytes_to_copy);
        buff().last -= bytes_erased;

        validate_pointer_invariants();
        validate_data_invariants();
    }
    //!
    //! @brief Erases element in the range (before_start, end)
    //! @param it - iterator pointing to an element before the first element
    //! that will be erased. If iterator is pointing to the last element
    //! of container then it will trigger fail-fast.
    //!
    void erase_all_after(iterator const &it) noexcept {
        validate_pointer_invariants();
        validate_iterator(it);

        //
        // erasing after end iterator is a noop
        //
        if (end() != it) {
            buff().last = it.get_ptr();
            set_no_next_element(buff().last);

            validate_pointer_invariants();
            validate_data_invariants();
        }
    }
    //!
    //! @brief Erases element in the range [it, end)
    //! @param it - iterator to the first element to be erased
    //! @details This algorithm has complexity O(element count) because
    //! to erase element pointed by it we need to locate element before 
    //! and make it new last element.
    //!
    iterator erase_all_from(iterator const &it) noexcept {
        validate_pointer_invariants();
        validate_iterator(it);

        //
        // erasing after end iterator is a noop
        //
        if (end() != it) {

            if (it == begin()) {
                buff().last = nullptr;
                return end();
            }

            range_t const element_rtange{ range_unsafe(it) };
            iterator const element_before = find_element_before(element_rtange.begin());
            FFL_CODDING_ERROR_IF(end() == element_before);

            erase_all_after(element_before);

            set_no_next_element(element_before.get_ptr());

            return element_before;
        }

        return end();
    }
    //!
    //! @brief Erases all elements in the buffer without deallocating buffer
    //!
    void erase_all() noexcept {
        validate_pointer_invariants();
        buff().last = nullptr;
    }

    //!
    //! @brief Erases element pointed by iterator.
    //! @param it - iterator pointing to the element being erased.
    //! @details This gives us erase semantics everyone is used to, but algorithm
    //! complexity is O(element count) if we are erasing last element because we need to find
    //! element before the element being erased, and make it last.
    //!
    iterator erase(iterator const &it) noexcept {
        validate_pointer_invariants();
        validate_iterator_not_end(it);

        if (begin() == it) {
            pop_front();
            return begin();
        }

        if (last() == it) {
            pop_back();
            return end();
        } 

        sizes_t const prev_sizes{ get_all_sizes() };
        range_t const element_range{ range_unsafe(it) };
        //
        // Size of all elements after element that we are erasing,
        // not counting padding after last element.
        //
        size_type const tail_size{ prev_sizes.used_capacity().size - element_range.buffer_end };
        //
        // Shifting remaining elements will erase this element.
        //
        move_data(buff().begin + element_range.begin(),
                  buff().begin + element_range.buffer_end,
                  tail_size);

        buff().last -= element_range.buffer_size();

        validate_pointer_invariants();
        validate_data_invariants();

        return it;
    }
    //!
    //! @brief Erases element in the range [start, end).
    //! @param start - first element to be erased.
    //! @param end - one pass the last element that is being erased.
    //! @details This gives us erase semantics everyone is used to, but algorithm
    //! complexity is O(element count) if we are erasing all element till the end of container,
    //! because we need to find element before the element being erased, and make it last.
    //!
    iterator erase(iterator const &start, iterator const &end) noexcept {
        validate_pointer_invariants();
        validate_iterator_not_end(start);
        validate_iterator(end);
        //
        // If it is an empty range then we are done
        //
        if (start == end) {
            return start;
        }
        //
        // If we are erasing all elements after start
        //
        if (this->end() == end) {
            return erase_all_from(start);
        }
        //
        // The rest of function deals with erasing from start to some existing element.
        // We need to shift all elements that we are keeping in place where start is.
        //
        sizes_t const prev_sizes{ get_all_sizes() };
        
        range_t const start_range = range_unsafe(start);
        range_t const end_range = range_unsafe(end);
        size_type const bytes_to_copy{ prev_sizes.used_capacity().size - end_range.buffer_end };
        size_type const bytes_erased{ end_range.begin() - start_range.begin() };

        move_data(buff().begin + start_range.begin(),
                  buff().begin + end_range.begin(),
                  bytes_to_copy);

        buff().last -= bytes_erased;

        validate_pointer_invariants();
        validate_data_invariants();

        return start;
    }
    //!
    //! @brief Swaps content of this container and the other container.
    //! @param other - reference to the other container
    //! @throws might throw std::bad_alloc if allocator swap throws or if
    //! allocators do not suport swap, and we need to make a copy of elements,
    //! which involves allocation.
    //!
    void swap(flat_forward_list &other) noexcept (allocator_type_traits::propagate_on_container_swap::value ||
                                                  allocator_type_traits::propagate_on_container_move_assignment::value) {
        if constexpr (allocator_type_traits::propagate_on_container_swap::value) {
            std::swap(get_allocator(), other.get_allocator());
            std::swap(buff().begin, other.buff().begin);
            std::swap(buff().end, other.buff().end);
            std::swap(buff().last, other.buff().last);
        } else {
            flat_forward_list tmp{ std::move(other) };
            other = std::move(*this);
            *this = std::move(tmp);
        }
    }
    //!
    //! @brief Sorts elements of container using comparator passed as a parameter
    //! @tparam LESS_F - type of comparator
    //! @param fn - instance of comparator
    //! @details Algorithm complexity is O(n*log(n)+2*n)
    //! @throws Might throw std::bad_alloc when allocation fails
    //!
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
            sorted_list.push_back(used_size(i), i.get_ptr());
        }

        //
        // Swap with sorted list
        //
        swap(sorted_list);
    }
    //!
    //! @brief Reverses elements of the list
    //! @throws Might throw std::bad_alloc when allocation fails
    //!
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
    //!
    //! @brief Merges two linked list ordering lists using comparison functor
    //! @tparam F - type of comparison functor
    //! @param fn - comparison functor
    //! @param other - the other list we are merging with
    //! @throws Might throw std::bad_alloc when allocation fails.
    //! @details if both lists are sorted, and we are merging using the same
    //! compare functor, then merged list will be sorted.
    //!
    template<class F>
    void merge(flat_forward_list &other, 
               F const &fn) {

        flat_forward_list merged_list(get_allocator());

        iterator this_start = begin();
        iterator const this_end = end();
        iterator other_start = other.begin();
        iterator const other_end = other.end();

        for ( ; this_start != this_end && other_start != other_end;) {
            if (fn(*this_start, *other_start)) {
                merged_list.push_back(required_size(this_start), this_start.get_ptr());
                ++this_start;
            } else {
                merged_list.push_back(other.required_size(other_start), other_start.get_ptr());
                ++other_start;
            }
        }

        for (; this_start != this_end; ++this_start) {
            merged_list.push_back(required_size(this_start), this_start.get_ptr());
        }

        for (; other_start != other_end; ++other_start) {
            merged_list.push_back(other.required_size(other_start), other_start.get_ptr());
        }

        swap(merged_list);
        other.clear();
    }
    //!
    //! @brief If there are multiple equivalent elements next to each other
    //! then first one is kept, and all other are deleted.
    //! @tparam F - type of comparison functor
    //! @param fn - comparison functor
    //!
    template<typename F>
    void unique(F const &fn) noexcept {
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
    //!
    //! @brief Removes all elements that satisfy predicate.
    //! @tparam F - type of predicate
    //! @param fn - predicate functor
    //!
    template<typename F>
    void remove_if(F const &fn) noexcept {
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
    //!
    //! @return Returns a reference to the first element in the container.
    //! @details Calling this method on a empty container will trigger fail-fast
    //!
    T &front() noexcept {
        validate_pointer_invariants();
        FFL_CODDING_ERROR_IF(buff().last == nullptr || buff().begin == nullptr);
        return *reinterpret_cast<T *>(buff().begin);
    }
    //!
    //! @return Returns a const reference to the first element in the container.
    //! @details Calling this method on a empty container will trigger fail-fast
    //!
    T const &front() const noexcept {
        validate_pointer_invariants();
        FFL_CODDING_ERROR_IF(buff().last == nullptr || buff().begin == nullptr);
        return *reinterpret_cast<T *>(buff().begin);
    }
    //!
    //! @return Returns a reference to the last element in the container.
    //! @details Calling this method on a empty container will trigger fail-fast
    //!
    T &back() noexcept {
        validate_pointer_invariants();
        FFL_CODDING_ERROR_IF(buff().last == nullptr);
        return *reinterpret_cast<T *>(buff().last);
    }
    //!
    //! @return Returns a const reference to the last element in the container.
    //! @details Calling this method on a empty container will trigger fail-fast
    //!
    T const &back() const noexcept {
        validate_pointer_invariants();
        FFL_CODDING_ERROR_IF(buff().last == nullptr);
        return *reinterpret_cast<T *>(buff().last);
    }
    //!
    //! @return Returns an iterator pointing to the first element of container.
    //! @details Calling on an empty container returns end iterator
    //!
    iterator begin() noexcept {
        validate_pointer_invariants();
        return buff().last ? iterator{ buff().begin }
                           : end();
    }
    //!
    //! @return Returns a const iterator pointing to the first element of container.
    //! @details Calling on an empty container returns const end iterator
    //!
    const_iterator begin() const noexcept {
        validate_pointer_invariants();
        return buff().last ? const_iterator{ buff().begin }
                           : cend();
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

    //!
    //! @return Returns an iterator pointing to the last element of container.
    //! @details Calling on an empty container returns end iterator
    //!
    iterator last() noexcept {
        validate_pointer_invariants();
        return buff().last ? iterator{ buff().last }
                           : end();
    }
    //!
    //! @return Returns a const iterator pointing to the last element of container.
    //! @details Calling on an empty container returns end iterator
    //!
    const_iterator last() const noexcept {
        validate_pointer_invariants();
        return buff().last ? const_iterator{ buff().last }
                           : cend();
    }
    //!
    //! @return Returns an end iterator.
    //! @details An end iterator is pointing pass the last
    //! element of container
    //!
    iterator end() noexcept {
        validate_pointer_invariants();
        if (buff().last) {
            size_with_padding_t const last_element_size{ traits_traits::get_size(buff().last) };
            return iterator{ buff().last + last_element_size.size_padded() };
        } else {
            return iterator{ };
        }
    }
    //!
    //! @return Returns an end const_iterator.
    //! @details An end iterator is pointing pass the last
    //! element of container
    //!
    const_iterator end() const noexcept {
        validate_pointer_invariants();
        if (buff().last) {
            size_with_padding_t const last_element_size{ traits_traits::get_size(buff().last) };
            return const_iterator{ buff().last + last_element_size.size_padded() };
        } else {
            return const_iterator{ };
        }
    }
    //!
    //! @return Returns a const iterator pointing to the first element of container.
    //! @details Calling on an empty container returns const end iterator
    //!
    const_iterator cbegin() const noexcept {
        return begin();
    }
    //!
    //! @return Returns a const iterator pointing to the last element of container.
    //! @details Calling on an empty container returns end iterator
    //!
    const_iterator clast() const noexcept {
        return last();
    }
    //!
    //! @return Returns an end const_iterator.
    //! @details For types that support get_next_offset offset or when container is empty, 
    //! the end iterator is const_iterator{}.
    //! For types that do not support get_next_offset an end iterator is pointing pass the last
    //! element of container
    //!
    const_iterator cend() const noexcept {
        return end();
    }
    //!
    //! @return Pointer to the begging of the buffer or nullptr 
    //! container has if no allocated buffer.
    //!
    char *data() noexcept {
        return buff().begin;
    }
    //!
    //! @return Const pointer to the begging of the buffer or nullptr 
    //! container has if no allocated buffer.
    //!
    char const *data() const noexcept {
        return buff().begin;
    }
    //!
    //! @brief Validates that buffer contains a valid list.
    //! @return true if valid list was found and false otherwise.
    //! @details You must call this method after passing pointer to 
    //! container's buffer to a function that might change buffer content.
    //! If valid list of found then buff().last will be pointing to the
    //! element element that was found. If no valid list was found then
    //! buff().last will be nullptr.
    //!
    bool revalidate_data() noexcept {
        auto[valid, buffer_view] = flat_forward_list_validate<T, TT>(buff().begin, 
                                                                     buff().end);
        if (valid) {
            buff().last = buffer_view.last().get_ptr();
        }
        return valid;
    }
    //!
    //! @brief Removes unused padding of each element and at the tail.
    //! @throws std::bad_alloc if allocating new buffer fails.
    //! @details This method also fixes alignment of each element and 
    //! adds missing padding, so at the end used capacity might grow.
    //! Once unused capacity of each element is removed, and missing 
    //! padding added, this method method reallocates buffer to remove
    //! any unused capacity at the tail.
    //!
    void shrink_to_fit() {
        shrink_to_fit(begin(), end());
        tail_shrink_to_fit();
    }
    //!
    //! @brief Removes unused padding of each element in the range [first, end).
    //! @param first - iterator that points to the first element to shrink.
    //! @param end - iterator that points passt the last element.
    //! @throws std::bad_alloc if allocating new buffer fails.
    //! @details This method also fixes alignment of each element and 
    //! adds missing padding, so at the end used capacity might grow.
    //!
    void shrink_to_fit(iterator const &first, iterator const &end) {
        for (iterator i = first; i != end; ++i) {
            shrink_to_fit(i);
        }
    }
    //!
    //! @brief Removes unused padding of an element.
    //! @param it - iterator that points to the element to shrink.
    //! @throws std::bad_alloc if allocating new buffer fails.
    //! @details This method also fixes alignment of element and 
    //! adds missing padding, so at the end used capacity might grow.
    //!
    void shrink_to_fit(iterator const &it) {
        validate_pointer_invariants();
        validate_iterator_not_end(it);

        size_type new_element_size{ required_size(it) };
        //
        // Make sure that any element, except the last one is padded at the end
        //
        if (last() != it) {
            new_element_size = traits_traits::roundup_to_alignment(new_element_size);
        } 

        iterator const ret_it = element_resize(it,
                                               new_element_size,
                                               [new_element_size] ([[maybe_unused]] T &buffer,
                                                                   size_type old_size, 
                                                                   size_type new_size) {
                                                       //
                                                       // might extend if element was not propertly alligned. 
                                                       // must be shrinking, and
                                                       // data must fit new buffer
                                                       //
                                                       FFL_CODDING_ERROR_IF_NOT(new_size <= new_element_size);
                                                       //
                                                       // Cannot assert it here sice we have not changed next element offset yet
                                                       // we will validate element at the end
                                                       //
                                                       //FFL_CODDING_ERROR_IF_NOT(traits_traits::validate(new_size, buffer));
                                               });
        //
        // We are either shrinking or not changing size 
        // so there should be no reallocation 
        //
        FFL_CODDING_ERROR_IF_NOT(it == ret_it);
    }
    //!
    //! @brief Adds unused capacity to the element.
    //! @param it - iterator that points to the element to grow.
    //! @param size_to_add - bytes to add to the current element size
    //! @throws std::bad_alloc if allocating new buffer fails.
    //! @details This method also fixes alignment of element and 
    //! adds missing padding.
    //! added capacity becomes unused capacity that is reserved 
    //! for future use. Unused capacity is zero initialized.
    //!
    iterator element_add_size(iterator const &it,
                              size_type size_to_add) {
        validate_pointer_invariants();
        validate_iterator_not_end(it);

        return element_resize(it, 
                              traits_traits::roundup_to_alignment(used_size(it) + size_to_add),
                              [] (T &buffer,  
                                  size_type old_size, 
                                  size_type new_size) {
                                  //
                                  // must be extending, and
                                  // data must fit new buffer
                                  //
                                  FFL_CODDING_ERROR_IF_NOT(old_size <= new_size);
                                  zero_buffer(reinterpret_cast<char *>(&buffer) + old_size, new_size - old_size);
                                  FFL_CODDING_ERROR_IF_NOT(traits_traits::validate(new_size, buffer));
                              });
    }
    //!
    //! @brief Adds unused capacity to the element.
    //! @param it - iterator that points to the element to grow.
    //! @param size_to_add - bytes to add to the current element size
    //! @returns true if element was placed in the existing buffer and false if
    //! buffer does not have enough capacity for the new element.
    //! @details This method also fixes alignment of element and 
    //! adds missing padding.
    //! added capacity becomes unused capacity that is reserved 
    //! for future use. Unused capacity is zero initialized.
    //!
    bool try_element_add_size(iterator const &it,
                              size_type size_to_add) {
        validate_pointer_invariants();
        validate_iterator_not_end(it);

        return try_element_resize(it, 
                                  traits_traits::roundup_to_alignment(used_size(it) + size_to_add),
                                  [] (T &buffer,  
                                      size_type old_size, 
                                      size_type new_size) {
                                      //
                                      // must be extending, and
                                      // data must fit new buffer
                                      //
                                      FFL_CODDING_ERROR_IF_NOT(old_size <= new_size);
                                      zero_buffer(reinterpret_cast<char *>(&buffer) + old_size, new_size - old_size);
                                      FFL_CODDING_ERROR_IF_NOT(traits_traits::validate(new_size, buffer));
                                  });
    }
    //!
    //! @brief Resizes element.
    //! @tparam F - type of functor user to update element.
    //! @param it - iterator that points to the element that is being resized.
    //! @param new_size - new element size.
    //! @param fn - functor used to update element after it was resized.
    //! @throws std::bad_alloc if allocating new buffer fails.
    //! @details Resizing to 0 will delete element.
    //! Resizing to size smaller than minimum_size will trigger fail-fast.
    //! Resizing to any other size also fixes alignment of element and 
    //! adds missing padding.
    //!
    template <typename F>
    iterator element_resize(iterator const &it, 
                            size_type new_size, 
                            F const &fn) {
        auto[result, new_it] = element_resize_impl(can_reallocate::yes,
                                                   it,
                                                   new_size,
                                                   fn);
        FFL_CODDING_ERROR_IF(!result);
        return new_it;
    }
    //!
    //! @brief Resizes element.
    //! @tparam F - type of functor user to update element.
    //! @param it - iterator that points to the element that is being resized.
    //! @param new_size - new element size.
    //! @param fn - functor used to update element after it was resized.
    //! @returns true if element was placed in the existing buffer and false if
    //! buffer does not have enough capacity for the new element.
    //! @details Resizing to 0 will delete element.
    //! Resizing to size smaller than minimum_size will trigger fail-fast.
    //! Resizing to any other size also fixes alignment of element and 
    //! adds missing padding.
    //!
    template <typename F>
    bool try_element_resize(iterator const &it, 
                            size_type new_size, 
                            F const &fn) {
        auto[result, new_it] = element_resize_impl(can_reallocate::no,
                                                   it,
                                                   new_size,
                                                   fn);
        FFL_CODDING_ERROR_IF_NOT(new_it == it);
        return result;
    }
    //!
    //! @brief Returns capacity used by the element's data.
    //! @param it - iterator pointing to the element we are returning size for.
    //! @returns Returns size of the element without paddint
    //!
    size_type required_size(const_iterator const &it) const noexcept {
        validate_pointer_invariants();
        validate_iterator_not_end(it);
        return size_unsafe(it).size;
    }
    //!
    //! @param it - iterator pointing to the element we are returning size for.
    //! @returns For any element except last returns distance from element start 
    //! to the start of the next element. For the last element it returns used_capacity
    //!
    size_type used_size(const_iterator const &it) const noexcept {
        validate_pointer_invariants();
        validate_iterator_not_end(it);

        return used_size_unsafe(it);
    }
    //!
    //! @brief Information about the buffer occupied by an element.
    //! @param it - iterator pointing to an element.
    //! @returns For any element except the last, it returns:
    //! - start element offset.
    //! - offset of element data end.
    //! - offset of element buffer end.
    //! For the last element data end and buffer end point to the 
    //! same position.
    //! @details All offsets values are relative to the buffer 
    //! owned by the container.
    //!
    range_t range(const_iterator const &it) const noexcept {
        validate_iterator_not_end(it);

        return range_unsafe(it);
    }

    //!
    //! @brief Information about the buffer occupied by elements in the range [begin, last].
    //! @param begin - iterator pointing to the first element.
    //! @param last - iterator pointing to the last element in the range.
    //! @returns For any case, except when last element of range is last element of the collection,
    //! it returns:
    //! - start of the first element.
    //! - offset of the last element data end.
    //! - offset of the last element buffer end.
    //! If range last is last element of collection then data end and
    //! buffer end point to the same position.
    //! @details All offsets values are relative to the buffer 
    //! owned by the container.
    //!
    range_t closed_range(const_iterator const &begin, const_iterator const &last) const noexcept {
        validate_iterator_not_end(begin);
        validate_iterator_not_end(last);

        return closed_range_unsafe(begin, last);
    }
    //!
    //! @brief Information about the buffer occupied by elements in the range [begin, end).
    //! @param begin - iterator pointing to the first element.
    //! @param end - iterator pointing to the past last element in the range.
    //! @returns For any case, except when end element of range is the end element of the collection,
    //! - start of the first element.
    //! - offset of the element before end data end.
    //! - offset of the element before end buffer end.
    //! If range end is colelction end then data end and
    //! buffer end point to the same position.
    //! @details All offsets values are relative to the buffer 
    //! owned by the container.
    //! Algorithm has complexity O(number of elements in collection) because to find 
    //! element before end we have to scan buffer from beginning.
    //!
    range_t half_open_range(const_iterator const &begin, const_iterator const &end) const noexcept {
        validate_iterator_not_end(begin);
        validate_iterator_not_end(end);

        return half_open_range_usafe(begin, end);
    }
    //!
    //! @brief Tells if given offset in the buffer falls into a 
    //! buffer used by the element.
    //! @param it - iterator pointing to an element
    //! @param position - offset in the container's buffer.
    //! @returns True if position is in the element's buffer. 
    //! and false otherwise. Element's buffer is retrieved using 
    //! range(it).
    //! When iterator referes to container end or when position is npos
    //! the result will be false.
    //!
    bool contains(const_iterator const &it, size_type position) const noexcept {
        validate_iterator(it);
        if (cend() == it || npos == position) {
            return false;
        }
        range_t const r{ range_unsafe(it) };
        return r.buffer_contains(position);
    }
    //!
    //! @brief Searches for an element before the element that containes 
    //! given position.
    //! @param position - offset in the conteiner's buffer
    //! @returns Iterator to the found element, if it was found, and
    //! container's end iterator otherwise.
    //! @details Cost of this algorithm is O(nuber of elements in container)
    //! because we have to performa linear search for an element from the start
    //! of container's buffer.
    //!
    iterator find_element_before(size_type position) noexcept {
        validate_pointer_invariants();
        if (empty_unsafe()) {
            return end();
        }
        auto const [is_valid, buffer_view] = flat_forward_list_validate<T, TT>(buff().begin,
                                                                               buff().begin + position);
        unused_variable(is_valid);

        if (!buffer_view.empty()) {
            return iterator{ const_cast<char *>(buffer_view.last().get_ptr()) };
        }
        return end();
    }
    //!
    //! @brief Searches for an element before the element that containes 
    //! given position.
    //! @param position - offset in the conteiner's buffer
    //! @returns Const iterator to the found element, if it was found, and
    //! container's end const iterator otherwise.
    //! @details Cost of this algorithm is O(nuber of elements in container)
    //! because we have to performa linear search for an element from the start
    //! of container's buffer.
    //!
    const_iterator find_element_before(size_type position) const noexcept {
        validate_pointer_invariants();
        if (empty_unsafe()) {
            return end();
        }
        auto[is_valid, buffer_view] = flat_forward_list_validate<T, TT>(buff().begin,
                                                                        buff().begin + position);
        if (!buffer_view.empty()) {
            return const_iterator{ buffer_view.last() };
        }
        return end();
    }
    //!
    //! @brief Searches for an element that containes given position.
    //! @param position - offset in the conteiner's buffer
    //! @returns Iterator to the found element, if it was found, and
    //! container's end iterator otherwise.
    //! @details Cost of this algorithm is O(nuber of elements in container)
    //! because we have to performa linear search for an element from the start
    //! of container's buffer.
    //!
    iterator find_element_at(size_type position) noexcept {
        iterator it = find_element_before(position);
        if (end() != it) {
            ++it;
            if (end() != it) {
                FFL_CODDING_ERROR_IF_NOT(contains(it, position));
                return it;
            }
        }
        return end();
    }
    //!
    //! @brief Searches for an element that containes given position.
    //! @param position - offset in the conteiner's buffer
    //! @returns Const iterator to the found element, if it was found, and
    //! container's end const iterator otherwise.
    //! @details Cost of this algorithm is O(nuber of elements in container)
    //! because we have to performa linear search for an element from the start
    //! of container's buffer.
    //!
    const_iterator find_element_at(size_type position) const noexcept {
        const_iterator it = find_element_before(position);
        if (cend() != it) {
            ++it;
            if (cend() != it) {
                FFL_CODDING_ERROR_IF_NOT(contains(it, position));
                return it;
            }
        }
        return end();
    }
    //!
    //! @brief Searches for an element after the element that containes 
    //! given position.
    //! @param position - offset in the conteiner's buffer
    //! @returns Iterator to the found element, if it was found, and
    //! container's end iterator otherwise.
    //! @details Cost of this algorithm is O(nuber of elements in container)
    //! because we have to performa linear search for an element from the start
    //! of container's buffer.
    //!
    iterator find_element_after(size_type position) noexcept {
        iterator it = find_element_at(position);
        if (end() != it) {
            ++it;
            if (end() != it) {
                return it;
            }
        }
        return end();
    }
    //!
    //! @brief Searches for an element after the element that containes 
    //! given position.
    //! @param position - offset in the conteiner's buffer
    //! @returns Const iterator to the found element, if it was found, and
    //! container's end const iterator otherwise.
    //! @details Cost of this algorithm is O(nuber of elements in container)
    //! because we have to performa linear search for an element from the start
    //! of container's buffer.
    //!
    const_iterator find_element_after(size_type position) const noexcept {
        const_iterator it = find_element_at(position);
        if (cend() != it) {
            ++it;
            if (cend() != it) {
                return it;
            }
        }
        return end();
    }
    //!
    //! @brief Number of elements in the container.
    //! @returns Number of elements in the container.
    //! @details Cost of this algorithm is O(nuber of elements in container).
    //! Container does not actively cache/updates element count so we need to
    //! scan list to find number of elements.
    //!
    size_type size() const noexcept {
        validate_pointer_invariants();
        size_type s = 0;
        std::for_each(cbegin(), cend(), [&s](T const &) noexcept {
            ++s;
        });
        return s;
    }
    //!
    //! @brief Tells if container contains no elements.
    //! @returns False when containe contains at least one element
    //! and true otherwise.
    //! @details Both container with no buffer as well as container
    //! that has buffer that does not contain any valid elements will
    //! return true.
    //!
    bool empty() const noexcept {
        validate_pointer_invariants();
        return  buff().last == nullptr;
    }
    //!
    //! @returns Number of bytes in the bufer used
    //! by existing elements.
    //!
    size_type used_capacity() const noexcept {
        validate_pointer_invariants();
        sizes_t const s{ get_all_sizes() };
        return s.used_capacity().size;
    }
    //!
    //! @returns Buffer size.
    //!
    size_type total_capacity() const noexcept {
        validate_pointer_invariants();
        return buff().end - buff().begin;
    }
    //!
    //! @returns Number of bytes in the buffer not used
    //! by the existing elements.
    //!
    size_type remaining_capacity() const noexcept {
        validate_pointer_invariants();
        sizes_t const s{ get_all_sizes() };
        return s.remaining_capacity;
    }
    //!
    //! @brief Fills parts of the buffer not used by element data with 
    //! fill_byte.
    //! @param fill_byte - pattern to use
    //! @param zero_unused_capacity - also fill container unused capacity
    //!                               true by default.
    //!
    void fill_padding(int fill_byte = 0, 
                      bool zero_unused_capacity = true) noexcept {
        validate_pointer_invariants();
        //
        // Fill gaps between any elements up to the last
        //
        auto const last{ this->last() };
        //
        // zero padding of each element
        //
        for (auto it = begin(); last != it; ++it) {
            range_t const element_range{ range_unsafe(it) };
            element_range.fill_unused_capacity_data_ptr(it.get_ptr(), fill_byte);
        }     
        //
        // If we told so then zero tail
        //
        if (zero_unused_capacity) {
            sizes_t const prev_sizes{ get_all_sizes() };
            if (prev_sizes.used_capacity().size > 0) {
                size_type const last_element_end{ prev_sizes.last_element.begin() + prev_sizes.last_element.data_size() };
                size_type const unuset_tail_size{ prev_sizes.total_capacity - last_element_end };
                fill_buffer(buff().begin + last_element_end,
                            fill_byte,
                            unuset_tail_size);
            }
        }
        
        validate_pointer_invariants();
        validate_data_invariants();
    }

private:

    //!
    //! @class can_reallocate.
    //! @brief Enumiration used to express per call
    //! buffer reallocation policy.
    //!
    enum class can_reallocate : bool {
        //!
        //! @brief Buffer reallocation is not allowed
        //!
        no,
        //!
        //! @brief Buffer reallocation is allowed
        //!
        yes,
    };

    //!
    //! @brief Constructs new element at the end of the list. 
    //! @tparam F - type of a functor
    //! Element is initialized with a help of the functor passed as a parameter.
    //! @param reallocation_policy - can_reallocate::no tells that method should 
    //! fail if new element does not fit free capacity that we have in the buffer.
    //! can_reallocate::no tells that method is allowed to reallocate buffer to 
    //! fit additional element.
    //! @param element_size - number of bytes required for the new element.
    //! @param fn - a functor used to construct new element.
    //! @returns false if element was not added because reallocation is not allowed
    //! and true otherwise.
    //! @throw std::bad_alloca if allocating new buffer fails.
    //!        Any exceptions that might be raised by the functor.
    //!        If functor raises then container remains in the state as if 
    //!        call did not happen
    //! @details Constructed element does not have to use up the entire buffer
    //! unused space will become unused buffer capacity.
    //!
    template <typename F>
    bool try_emplace_back_impl(can_reallocate reallocation_policy,
                               size_type element_size,
                               F const &fn) {
        validate_pointer_invariants();

        FFL_CODDING_ERROR_IF(element_size < traits_traits::minimum_size());

        char *new_buffer{ nullptr };
        size_t new_buffer_size{ 0 };
        auto deallocate_buffer{ make_scoped_deallocator(&new_buffer, &new_buffer_size) };

        sizes_t const prev_sizes{ get_all_sizes() };

        char *cur{ nullptr };
        //
        // We are appending. New last element does not have to be padded 
        // since there will be no elements after. We do need to padd the 
        // previous last element so the new last element will be padded
        //
        if (prev_sizes.remaining_capacity_for_append() < element_size) {
            if (reallocation_policy == can_reallocate::no) {
                return false;
            }
            new_buffer_size = traits_traits::roundup_to_alignment(prev_sizes.total_capacity) +
                              (element_size - prev_sizes.remaining_capacity_for_append());
            new_buffer = allocate_buffer(new_buffer_size);
            cur = new_buffer + prev_sizes.used_capacity().size_padded();
        } else {
            cur = buff().begin + prev_sizes.used_capacity().size_padded();
        }

        fn(*traits_traits::ptr_to_t(cur), element_size);

        set_no_next_element(cur);
        //
        // After element was constructed it cannot be larger than 
        // size requested for this element.
        //
        size_with_padding_t const cur_element_size{ traits_traits::get_size(cur) };
        FFL_CODDING_ERROR_IF(element_size < cur_element_size.size);
        //
        // element that used to be last becomes
        // an element in the middle so we need to change 
        // its next element pointer
        //
        if (buff().last) {
            set_next_offset(buff().last, prev_sizes.last_element.data_size_padded());
        }
        //
        // swap new buffer and new buffer
        //
        if (new_buffer != nullptr) {
            //
            // If we are reallocating buffer then move existing 
            // elements to the new buffer. 
            //
            if (buff().begin) {
                copy_data(new_buffer, buff().begin, prev_sizes.used_capacity().size);
            }
            commit_new_buffer(new_buffer, new_buffer_size);
        }
        //
        // Element that we've just addede is the new last element
        //
        buff().last = cur;

        validate_pointer_invariants();
        validate_data_invariants();

        return true;
    }
    //!
    //! @brief Constructs new element at the position described by iterator. 
    //! Element is initialized with a help of the functor passed as a parameter.
    //! @tparam F - type of a functor
    //! @param reallocation_policy - can_reallocate::no tells that method should 
    //! fail if new element does not fit free capacity that we have in the buffer.
    //! can_reallocate::no tells that method is allowed to reallocate buffer to 
    //! fit additional element.
    //! @param it - iterator pointing to the position new element should be inserte to.
    //!             Iterator value must be a valid iterator pointing to one of the elements
    //!             or an end iterator.
    //!             If iterator is pointing to an element that is not propertly alligned then
    //!             new element will not be propertly alligned.
    //!             When iterator in not then end then size is padded to make sure that if
    //!             this element is propertly alligned then next element is also property 
    //!             aligned.
    //! @param new_element_size - number of bytes required for the new element.
    //! @param fn - a functor used to construct new element.
    //! @returns Returns a pair of boolean that tells caller if element was inserted, and 
    //! iterator to the position where it should be inserted in case of failure or was 
    //! inserted in case of success.
    //! @throw std::bad_alloca if allocating new buffer fails.
    //!        Any exceptions that might be raised by the functor.
    //!        If functor raises then container remains in the state as if 
    //!        call did not happen
    //! @details Constructed element does not have to use up the entire buffer
    //! unused space will become unused buffer capacity.
    //!
    template <typename F>
    std::pair<bool, iterator> try_emplace_impl(can_reallocate reallocation_policy,
                                               iterator const &it,
                                               size_type new_element_size, 
                                               F const &fn) {

        validate_pointer_invariants();
        validate_iterator(it);

        FFL_CODDING_ERROR_IF(new_element_size < traits_traits::minimum_size());

        if (end() == it) {
            bool result{ try_emplace_back_impl(reallocation_policy,
                                               new_element_size,
                                               fn) };
            return std::make_pair(result, last());
        }
        //
        // When implacing in the middle of list we need to preserve alignment of the 
        // next element so we need to make sure we add padding to the requested size.
        // An alternative would be to rely on the caller to make sure size is padded
        // and call out codding error if it is not.
        //
        size_t new_element_size_aligned = traits_traits::roundup_to_alignment(new_element_size);

        char *new_buffer{ nullptr };
        size_t new_buffer_size{ 0 };
        auto deallocate_buffer{ make_scoped_deallocator(&new_buffer, &new_buffer_size) };

        sizes_t const prev_sizes{ get_all_sizes() };
        range_t const element_range{ this->range_unsafe(it) };
        //
        // Note that in this case old element that was in that position
        // is a part of the tail
        //
        size_type const tail_size{ prev_sizes.used_capacity().size - element_range.begin() };

        char *begin{ nullptr };
        char *cur{ nullptr };
        //
        // We are inserting new element in the middle. 
        // We do not need make last eleement aligned. 
        // We need to add padding for the element that we are inserting to
        // keep element that we are shifting right propertly aligned
        //
        if (prev_sizes.remaining_capacity_for_insert() < new_element_size_aligned) {
            if (can_reallocate::no == reallocation_policy) {
                return std::make_pair(false, it);
            }
            new_buffer_size = traits_traits::roundup_to_alignment(prev_sizes.total_capacity) + 
                              (new_element_size_aligned - prev_sizes.remaining_capacity_for_insert());
            new_buffer = allocate_buffer(new_buffer_size);
            cur = new_buffer + element_range.begin();
            begin = new_buffer;
        } else {
            cur = it.get_ptr();
            begin = buff().begin;
        }
        char *new_tail_start{ nullptr };
        //
        // Common part where we construct new part of the buffer
        //
        // Move tail forward. 
        // This will free space to insert new element
        //
        if (!new_buffer) {
            new_tail_start = begin + element_range.begin() + new_element_size_aligned;
            move_data(new_tail_start, it.get_ptr(), tail_size);
        }
        //
        // construct
        //
        try {
            //
            // Pass to the constructor requested size, not padded size
            //
            fn(*traits_traits::ptr_to_t(cur), new_element_size);
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

        set_next_offset(cur, new_element_size_aligned);
        //
        // For types with next element pointer data can consume less than requested size. 
        // for types that do not have next element pointer size must match exactly, otherwise
        // we would not be able to get to the next element
        //
        size_with_padding_t const cur_element_size{ traits_traits::get_size(cur) };
        if constexpr (traits_traits::has_next_offset_v) {
            FFL_CODDING_ERROR_IF(new_element_size < cur_element_size.size ||
                                 new_element_size_aligned < cur_element_size.size_padded());
        } else {
            FFL_CODDING_ERROR_IF_NOT(new_element_size == cur_element_size.size &&
                                     new_element_size_aligned == cur_element_size.size_padded());
        }
        //
        // now see if we need to update existing 
        // buffer or swap with the new buffer
        //
        if (new_buffer != nullptr) {
            //
            // If we are reallocating buffer then move all elements
            // before and after position that we are inserting to
            //
            if (buff().begin) {
                copy_data(new_buffer, buff().begin, element_range.begin());
                copy_data(cur + new_element_size_aligned, it.get_ptr(), tail_size);
            }
            commit_new_buffer(new_buffer, new_buffer_size);
        }
        //
        // Last element moved ahead by the size of the new inserted element
        //
        buff().last = buff().begin + prev_sizes.last_element.begin() + new_element_size_aligned;

        validate_pointer_invariants();
        validate_data_invariants();
        //
        // Return iterator pointing to the new inserted element
        //
        return std::make_pair(true, iterator{ cur });
    }
    //!
    //! @brief Tells if container contains no elements.
    //! @returns False when containe contains at least one element
    //! and true otherwise.
    //! @details Both container with no buffer as well as container
    //! that has buffer that does not contain any valid elements will
    //! return true.
    //!
    bool empty_unsafe() const noexcept {
        return  buff().last == nullptr;
    }

    //!
    //! @brief Resizes last element of the container.
    //! @tparam F - type of functor user to update element.
    //! @param new_size - new element size.
    //! @param fn - functor used to update element after it was resized.
    //! @throws std::bad_alloc if allocating new buffer fails.
    //! @details Resizing to 0 will delete element.
    //! Resizing to size smaller than minimum_size will trigger fail-fast.
    //! Resizing of the last element does not add extra padding to the buffer 
    //! since there is no next element that needs to be padded.
    //! 
    //!
    template <typename F>
    std::pair<bool, iterator> resize_last_element(can_reallocate reallocation_policy,
                                                  size_type new_size,
                                                  F const &fn) {

        validate_pointer_invariants();

        sizes_t const prev_sizes{ get_all_sizes() };
        difference_type const element_size_diff{ static_cast<difference_type>(new_size - prev_sizes.last_element.data_size()) };
        //
        // If last element is shrinking or 
        // if it does not reach capacity available in the buffer 
        // for growing without need to worry about padding
        // then just call functor to modify element data.
        // there is no additional post processing after success
        // of failure.
        //
        // Otherwise allocate new buffer, copy element there,
        // modify it, and on success copy head
        //
        if (element_size_diff < 0 ||
            prev_sizes.remaining_capacity_for_insert() >= static_cast<size_type>(element_size_diff)) {

            fn(*traits_traits::ptr_to_t(buff().last),
               prev_sizes.last_element.data_size(),
               new_size);

        } else {

            if (can_reallocate::no == reallocation_policy) {
                return std::make_pair(false, last());
            }

            char *new_buffer{ nullptr };
            size_type new_buffer_size{ 0 };
            auto deallocate_buffer{ make_scoped_deallocator(&new_buffer, &new_buffer_size) };

            new_buffer_size = prev_sizes.used_capacity().size + new_size - prev_sizes.last_element.data_size();
            new_buffer = allocate_buffer(new_buffer_size);

            char *new_last_ptr{ new_buffer + prev_sizes.last_element.begin() };
            //
            // copy element
            //
            copy_data(new_last_ptr,
                      buff().begin + prev_sizes.last_element.begin(),
                      prev_sizes.last_element.data_size());
            //
            // change element
            //
            fn(*traits_traits::ptr_to_t(new_last_ptr),
               prev_sizes.last_element.data_size(),
               new_size);
            //
            // copy head
            //
            copy_data(new_buffer, buff().begin, prev_sizes.last_element.begin());
            //
            // commit mew buffer
            //
            commit_new_buffer(new_buffer, new_buffer_size);
            buff().last = new_last_ptr;
        }

        validate_pointer_invariants();
        validate_data_invariants();

        return std::make_pair(true, last());
    }
    //!
    //! @brief Resizes element.
    //! @tparam F - type of functor user to update element.
    //! @param it - iterator that points to the element that is being resized.
    //! @param new_size - new element size.
    //! @param fn - functor used to update element after it was resized.
    //! @throws std::bad_alloc if allocating new buffer fails.
    //! @details Resizing to 0 will delete element.
    //! Resizing to size smaller than minimum_size will trigger fail-fast.
    //! Resizing to any other size also fixes alignment of element and 
    //! adds missing padding.
    //!
    template <typename F>
    std::pair<bool, iterator> element_resize_impl(can_reallocate reallocation_policy, 
                                                  iterator const &it,
                                                  size_type new_size, 
                                                  F const &fn) {
        //
        // Resize to 0 is same as erase
        //
        if (0 == new_size) {
            return std::make_pair(true, erase(it));
        }

        FFL_CODDING_ERROR_IF(new_size < traits_traits::minimum_size());
        //
        // Separately handle case of resizing the last element 
        // we do not need to deal with tail and padding
        // for the next element
        //
        if (last() == it) {
            return resize_last_element(reallocation_policy, new_size, fn);
        }
        //
        // Now deal with resizing element in the middle
        // or at the head
        //
        validate_pointer_invariants();
        validate_iterator_not_end(it);

        iterator result_it;

        sizes_t prev_sizes{ get_all_sizes() };
        range_t element_range_before{ range_unsafe(it) };
        //
        // We will change element size by padded size to make sure
        // that next element stays padded
        //
        size_type new_size_padded{ traits_traits::roundup_to_alignment(new_size) };
        //
        // Calculate tail size
        //
        size_type const tail_size{ prev_sizes.used_capacity().size - element_range_before.buffer_end };
        //
        // By how much element size is changing
        //
        difference_type const element_size_diff{ static_cast<difference_type>(new_size_padded - element_range_before.buffer_size()) };
        //
        // This branch handles case when we do not need to reallocate buffer
        // and we have sufficiently large buffer.
        //
        if (element_size_diff < 0 ||
            prev_sizes.remaining_capacity_for_insert() >= static_cast<size_type>(element_size_diff)) {

            size_type tail_start_offset = element_range_before.buffer_end;
            //
            // If element is getting extended in size then shift tail to the 
            // right to make space for element data, and remember new tail 
            // position
            //
            if (new_size_padded > element_range_before.buffer_size()) {
                move_data(buff().begin + element_range_before.begin() + new_size_padded,
                          buff().begin + tail_start_offset,
                          tail_size);

                tail_start_offset = element_range_before.begin() + new_size_padded;
            }
            //
            // Regardless how  we are exiting scope, by exception or not,
            // shift tail left so it would be right after the new end of 
            // the element
            //
            auto fix_tail{ make_scope_guard([this, 
                                            it, 
                                            new_size, 
                                            new_size_padded, 
                                            tail_start_offset, 
                                            tail_size,
                                            &element_range_before] {
                //
                // See how much space elements consumes now
                //
                size_with_padding_t const element_size_after { this->size_unsafe(it)};
                //
                // New element size must not be larget than size that it is 
                // allowed to grow by
                //
                FFL_CODDING_ERROR_IF(element_size_after.size > new_size);
                //
                // If we have next pointer then next element should start after new_size_padded
                // otherwise if should start after padded size of the element
                //
                range_t element_range_after{ {element_range_before.begin(),
                                              element_range_before.begin() + element_size_after.size,
                                              element_range_before.begin() + new_size_padded} };
                element_range_after.verify();
                //
                // New evement end must not pass current tail position
                //
                FFL_CODDING_ERROR_IF(element_range_after.buffer_end > tail_start_offset);
                //
                // if size changed, then shift tal to the left
                //
                if (element_range_after.buffer_end != element_range_before.buffer_end) {

                    move_data(buff().begin + element_range_after.buffer_end,
                              buff().begin + tail_start_offset,
                              tail_size);
                    //
                    // calculate by how much tail moved
                    //
                    difference_type tail_shift{ static_cast<difference_type>(element_range_after.buffer_size() -
                                                                             element_range_before.buffer_size()) };
                    //
                    // Update pointer to last element
                    //
                    buff().last += tail_shift;
                }

                this->set_next_offset(it.get_ptr(), element_range_after.buffer_size());
            }) };

            fn(*traits_traits::ptr_to_t(it.get_ptr()),
               element_range_before.buffer_size(),
               new_size);

            result_it = it;

        } else {
            if (can_reallocate::no == reallocation_policy) {
                return std::make_pair(false, it);
            }
            //
            // Buffer is increasing in size so we will need to reallocate buffer
            //
            char *new_buffer{ nullptr };
            size_type new_buffer_size{ 0 };
            auto deallocate_buffer{ make_scoped_deallocator(&new_buffer, &new_buffer_size) };

            new_buffer_size = prev_sizes.used_capacity().size + new_size_padded - element_range_before.buffer_size();
            new_buffer = allocate_buffer(new_buffer_size);
            //
            // copy element that we are changing to the new buffer
            //
            copy_data(new_buffer + element_range_before.begin(),
                      buff().begin + element_range_before.begin(),
                      element_range_before.buffer_size());
            //
            // change element
            //
            fn(*traits_traits::ptr_to_t(new_buffer + element_range_before.begin()),
               element_range_before.buffer_size(),
               new_size);
            //
            // fn did not throw an exception, and remainder of 
            // function is noexcept
            //
            result_it = iterator{ new_buffer + element_range_before.begin() };
            //
            // copy head
            //
            copy_data(new_buffer, buff().begin, element_range_before.begin());
            //
            // See how much space elements consumes now
            //
            size_with_padding_t const element_size_after = this->size_unsafe(result_it);
            //
            // New element size must not be larget than size that it is 
            // allowed to grow by
            //
            FFL_CODDING_ERROR_IF(element_size_after.size > new_size);
            //
            // If we have next pointer then next element should start after new_size_padded
            // otherwise if should start after padded size of the element
            //
            range_t element_range_after{ { element_range_before.begin(),
                                           element_range_before.begin() + element_size_after.size,
                                           element_range_before.begin() + new_size_padded } };
            element_range_after.verify();
            //
            // Copy tail
            //
            move_data(new_buffer + element_range_after.buffer_end,
                      buff().begin + element_range_before.buffer_end,
                      tail_size);
            //
            // commit mew buffer
            //
            commit_new_buffer(new_buffer, new_buffer_size);
            //
            // calculate by how much tail moved
            //
            difference_type tail_shift{ static_cast<difference_type>(element_range_after.buffer_size() -
                                                                     element_range_before.buffer_size()) };
            //
            // Update pointer to last element
            //
            buff().last = buff().begin + prev_sizes.last_element.begin() + tail_shift;
            //
            // fix offset to the next element
            //
            this->set_next_offset(result_it.get_ptr(), element_range_after.buffer_size());
        }

        validate_pointer_invariants();
        validate_data_invariants();
        //
        // Return iterator pointing to the new inserted element
        //
        return std::make_pair(true, result_it);
    }
    //!
    //! @brief Returns true when container has no or exactly one entry
    //! and false otherwise.
    //!
    constexpr bool has_one_or_no_entry() const noexcept {
        return buff().last == buff().begin;
    }
    //!
    //! @brief Returns true when container has exactly one entry
    //! and false otherwise.
    //!
    constexpr bool has_exactly_one_entry() const noexcept {
        return nullptr != buff().last &&
            buff().last == buff().begin;
    }
    //!
    //! @brief Helper routine that is used on element that just became
    //! new last element of the container to set pointer to the next 
    //! element to 0.
    //! @param buffer - pointer to the element buffer start.
    //!
    constexpr static void set_no_next_element(char *buffer) noexcept {
        set_next_offset(buffer, 0);
    }
    //!
    //! @brief Sets offset ot the next element.
    //! @param buffer - pointer to the element buffer start.
    //! @param size - new element size
    //! @details This call is a noop for the types that do not support 
    //! next element offset 
    //!
    constexpr static void set_next_offset([[maybe_unused]] char *buffer, [[maybe_unused]] size_t size) noexcept {
        if constexpr (traits_traits::has_next_offset_v) {
            traits_traits::set_next_offset(buffer, size);
        }
    }
    //!
    //! @brief Moves allocator from the other instance of container.
    //! @details Note that after we move allocator other container will
    //! not be able to deallocate its buffer so this call can be used only
    //! along with methods that also adopt other container data.
    //! @param other - other container we are movig allocator from.
    //!
    void move_allocator_from(flat_forward_list &&other) {
        alloc() = std::move(other.alloc());
    }
    //!
    //! @brief Cleans this container, and takes ownership of data
    //! from the other container. Caller must make sure that this 
    //! container has a compatible allocator.
    //! @param other - other container we are movig data from.
    //!
    void move_from(flat_forward_list &&other) noexcept {
        clear();
        buff().begin = other.buff().begin;
        buff().end = other.buff().end;
        buff().last = other.buff().last;
        other.buff().begin = nullptr;
        other.buff().end = nullptr;
        other.buff().last = nullptr;
    }
    //!
    //! @brief Cleans this container, and if it is safe then moves 
    //! data from the other container, otherwise it copies data.
    //! @param other - other container we are movig or copying data from.
    //! @details Data are moped if containers have equivalent allocators.
    //! otherwise data are copied
    //!
    void try_move_from(flat_forward_list &&other) {
        if (other.get_allocator() == get_allocator()) {
            move_from(std::move(other));
        } else {
            copy_from(other);
        }
    }
    //!
    //! @brief Copies data from the other container
    //! @param other - other container we are movig or copying data from.
    //! 
    void copy_from(flat_forward_list const &other) {
        clear();
        if (other.buff().last) {
            sizes_t const other_sizes{ other.get_all_sizes() };
            buff().begin = allocate_buffer(other_sizes.used_capacity().size);
            copy_data(buff().begin, other.buff().begin, other_sizes.used_capacity().size);
            buff().end = buff().begin + other_sizes.used_capacity().size;
            buff().last = buff().begin + other_sizes.last_element.begin();
        }
    }
    //!
    //! @brief Allocates buffer of requested size using allocator owned 
    //! by this container
    //! @param buffer_size - size of the buffer that we are allocating
    //!
    char *allocate_buffer(size_t buffer_size) {
        char *ptr{ allocator_type_traits::allocate(alloc(), buffer_size) };
        FFL_CODDING_ERROR_IF(nullptr == ptr);
        return ptr;
    }
    //!
    //! @brief Deallocates buffer using allocator owned 
    //! by this container.
    //! @param buffer - pointer to the buffer we are deallocating.
    //! @param buffer_size - size of the buffer that we are deallocating
    //!
    void deallocate_buffer(char *buffer, size_t buffer_size) noexcept {
        FFL_CODDING_ERROR_IF(0 == buffer_size || nullptr == buffer);
        try {
            allocator_type_traits::deallocate(alloc(), buffer, buffer_size);
        } catch (...) {
            FFL_CRASH_APPLICATION();
        }
    }
    //!
    //! @brief Swaps buffer owned by container with a new buffer.
    //! @param buffer - Reference to the pointer to the new buffer
    //! @param buffer_size - Reference to the buffer size
    //! Both parameters are inout parameters. On input they describe
    //! new buffer, and on output then contain information about the
    //! old buffer.
    //!
    void commit_new_buffer(char *&buffer, size_t &buffer_size) noexcept {
        char *old_begin = buff().begin;
        FFL_CODDING_ERROR_IF(buff().end < buff().begin);
        size_type old_size = buff().end - buff().begin;
        FFL_CODDING_ERROR_IF(buffer == nullptr && buffer_size != 0);
        FFL_CODDING_ERROR_IF(buffer != nullptr && buffer_size == 0);
        buff().begin = buffer;
        buff().end = buff().begin + buffer_size;
        buffer = old_begin;
        buffer_size = old_size;
    }
    //!
    //! @brief Creates scoped deallocator that frees buffer described by
    //! the pair **buffer and *buffer_size.
    //! @param buffer - pointer to the pointer to the buffer.
    //! @param buffer_size - pointer to the buffer size.
    //! Both parameters are in-out . On input they contain information
    //! about the buffer we are deallocating. Buffer can be nullptr.
    //! On output they are nulled out, because theya re not pointing to a valid 
    //! buffer any longer.
    //!
    auto make_scoped_deallocator(char **buffer, size_t *buffer_size) noexcept {
        return make_scope_guard([this, buffer, buffer_size]() {
            if (*buffer) {
                deallocate_buffer(*buffer, *buffer_size);
                *buffer = nullptr;
                *buffer_size = 0;
            }
        });
    }
    //!
    //! @brief Validates that container information about buffer
    //! matches to the actual buffer content.
    //!
    void validate_data_invariants() const noexcept {
#ifdef FFL_DBG_CHECK_DATA_VALID
        if (buff().last) {
            //
            // For element types that have offset of next element use entire buffer for validation
            // for element types that do not we have to limit by the end of the last element.
            // If there is sufficient reservation after last element then validate would not know 
            // where to stop, and might fail.
            //
            size_type buffer_lenght{ static_cast<size_type>(buff().end - buff().begin) };
            if constexpr (!traits_traits::has_next_offset_v) {
                auto[last_element_size, last_element_size_padded] = traits_traits::get_size(buff().last);
                buffer_lenght = last_element_size;
            }
            auto const [valid, buffer_view] = flat_forward_list_validate<T, TT>(buff().begin, buff().last + buffer_lenght);
            FFL_CODDING_ERROR_IF_NOT(valid);
            FFL_CODDING_ERROR_IF_NOT(buffer_view.last().get_ptr() == buff().last);

            if (buff().last) {
                size_with_padding_t const last_element_size = traits_traits::get_size(buff().last);
                size_type const last_element_offset{ static_cast<size_type>(buff().last - buff().begin) };
                FFL_CODDING_ERROR_IF(buffer_lenght < (last_element_offset + last_element_size.size));
            }
        }
#endif //FFL_DBG_CHECK_DATA_VALID
    }
    //!
    //! @brief Validates that container pointer invariants.
    //!
    void validate_pointer_invariants() const noexcept {
        buff().validate();
    }
    //!
    //! @brief Validates iterator invariants.
    //! @param it - iterator that we are verifying
    //!
    void validate_iterator(const_iterator const &it) const noexcept {
        //
        // If we do not have any valid elements then
        // only end iterator is valid.
        // Otherwise iterator should be an end or point somewhere
        // between begin of the buffer and start of the first element
        //
        if (empty_unsafe()) {
            FFL_CODDING_ERROR_IF_NOT(cend() == it);
        } else {
            FFL_CODDING_ERROR_IF_NOT(cend() == it ||
                                     (buff().begin <= it.get_ptr() && it.get_ptr() <= buff().last));
            validate_compare_to_all_valid_elements(it);
        }
    }
    //!
    //! @brief Validates iterator invariants, as well as asserts 
    //! that iterator is not an end iterator.
    //! @param it - iterator that we are verifying
    //!
    void validate_iterator_not_end(const_iterator const &it) const noexcept {
        //
        // Used in case when end iterator is meaningless.
        // Validates that container is not empty,
        // and iterator is pointing somewhere between
        // begin of the buffer and start of the first element
        //
        FFL_CODDING_ERROR_IF(cend() == it);
        FFL_CODDING_ERROR_IF(const_iterator{} == it);
        FFL_CODDING_ERROR_IF_NOT(buff().begin <= it.get_ptr() && it.get_ptr() <= buff().last);
        validate_compare_to_all_valid_elements(it);
    }
    //!
    //! @brief Validates iterator points to a valid element 
    //! in the container 
    //! @param it - iterator that we are verifying
    //!
    void validate_compare_to_all_valid_elements([[maybe_unused]] const_iterator const &it) const noexcept {
#ifdef FFL_DBG_CHECK_ITERATOR_VALID
        //
        // If not end iterator then must point to one of the valid iterators.
        //
        if (end() != it) {
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
    //!
    //! @brief Size used by an element data
    //! @param it - iterator for the element user queries size for
    //! @returns Returns size that is required for element data
    //!
    size_with_padding_t size_unsafe(const_iterator const &it) const noexcept {
        return  traits_traits::get_size(it.get_ptr());
    }
    //!
    //! @brief Size used by an element data and padding
    //! @param it - iterator for the element user queries size for
    //! @returns For types that support get_next_offset it should
    //! return value of offset to the next element from the start of 
    //! the element pointer by the iterator. If this is last element of the 
    //! list then it returns element data size without padding.
    //! For types that do not support get_next_offset it returns padded
    //! element size, except the last element. For the last element we return
    //! size without padding.
    //! @details Last element is treated differently because container 
    //! buffer might not include space for the last element padding.
    //!
    size_type used_size_unsafe(const_iterator const &it) const noexcept {
        if constexpr (traits_traits::has_next_offset_v) {
            size_type next_offset = traits_traits::get_next_offset(it.get_ptr());
            if (0 == next_offset) {
                size_with_padding_t const s{ traits_traits::get_size(it.get_ptr()) };
                //
                // Last element
                //
                next_offset = s.size;
            }
            return next_offset;
        } else {
            size_with_padding_t s{ traits_traits::get_size(it.get_ptr()) };
            //
            // buffer might end without including padding for the last element
            //
            if (last() == it) {
                return s.size;
            } else {
                return s.size_padded();
            }
        }
    }
    //!
    //! @brief Returns offsets in the container buffer that describe
    //! part of the buffer used by this element.
    //! @param it - iterator for the element user queries range for
    //! @returns Range that describes part of the container buffer used
    //! by the element
    //! @details For the last element of the buffer end of the data and
    //! end of the buffer have the same value.
    //! For any element data end is calculated using size of data used by element.
    //! For any element except last buffer size is calculated using
    //!  - Value of offset to the next element if type supports get_next_offset
    //!  - If get_next_offset is not suported then it is calculated as padded 
    //!    element data size
    //! This method assumes that iterator is pointing to an element, and is not an
    //! end iterator. It assumes that caller verified that iterator satisfies 
    //! these preconditions.
    //!
    range_t range_unsafe(const_iterator const &it) const noexcept {
        size_with_padding_t const s{ traits_traits::get_size(it.get_ptr()) };
        range_t r{};
        r.buffer_begin = it.get_ptr() - buff().begin;
        r.data_end = r.begin() + s.size;
        if constexpr (traits_traits::has_next_offset_v) {
            size_type const next_offset = traits_traits::get_next_offset(it.get_ptr());
            if (0 == next_offset) {
                FFL_CODDING_ERROR_IF(last() != it);
                r.buffer_end = r.begin() + s.size;
            } else {
                r.buffer_end = r.begin() + next_offset;
            }
        } else {
            if (last() == it) {
                r.buffer_end = r.begin() + s.size;
            } else {
                r.buffer_end = r.begin() + s.size_padded();
            }
        }
        return r;
    }
    //!
    //! @returns information about the pasrt of container 
    //! buffer used by elements in the range [first, last]
    //!
    range_t closed_range_unsafe(const_iterator const &first, const_iterator const &last) const noexcept {
        if (first == last) {
            return element_range_unsafe(first);
        } else {
            range_t first_element_range{ range_unsafe(first) };
            range_t last_element_range{ range_unsafe(last) };

            return range_t{ {first_element_range.begin,
                             last_element_range.data_end,
                             last_element_range.buffer_end } };
        }
    }
    //!
    //! @returns information about the pasrt of container 
    //! buffer used by elements in the range [first, end)
    //! @details Algorithm complexity is O(number of elements in the niffer)
    //! because we need to scan container from the beginning to find element 
    //! before end.
    //!
    range_t half_open_range_usafe(const_iterator const &first,
                                  const_iterator const &end) const noexcept {

        if (this->end() == end) {
            return closed_range_usafe(first, last());
        }

        size_t end_begin{ static_cast<size_t>(end->get_ptr() - buff().begin) };
        iterator last{ find_element_before(end_begin) };

        return closed_range_usafe(first, last);
    }
    //!
    //! @returns Information about containers buffer
    //! @details Most algorithms that modify container
    //! use this method to buffer information in a single 
    //! snapshot.
    //!
    sizes_t get_all_sizes() const noexcept {
        sizes_t s;

        s.total_capacity = buff().end - buff().begin;

        if (nullptr != buff().last) {
            s.last_element = range_unsafe(const_iterator{ buff().last });
        }
        FFL_CODDING_ERROR_IF(s.total_capacity < s.used_capacity().size);

        return s;
    }
    //!
    //! @brief Returns reference to the struct that 
    //! describes buffer.
    //! Helps abstracting out uglification that comes from
    //! using a compressed_pair.
    //!
    buffer_type &buff() noexcept {
        return buffer_.get_second();
    }
    //!
    //! @brief Returns const reference to the struct that 
    //! describes buffer.
    //! Helps abstracting out uglification that comes from
    //! using a compressed_pair.
    //!
    buffer_type const &buff() const noexcept {
        return buffer_.get_second();
    }

    //!
    //! @brief Returns reference to the allocator.
    //! Helps abstracting out uglification that comes from
    //! using a compressed_pair.
    //!
    allocator_type &alloc() noexcept {
        return buffer_.get_first();
    }
    //!
    //! @brief Returns const reference to the allocator.
    //! Helps abstracting out uglification that comes from
    //! using a compressed_pair.
    //!
    allocator_type const &alloc() const noexcept {
        return buffer_.get_first();
    }
    //!
    //! @brief Set of pointers describing state of
    //! of the buffer
    //!
    compressed_pair<allocator_type, buffer_type> buffer_;
};
//!
//! @tparam T - element type
//! @tparam TT - element type traits
//! @tparam A - allocator type that should be used for this container
//! @param lhs - one of the containers we are swapping between
//! @param rhs - other container we are swapping between
//! @details 
//! TT is aefault initialized to specialization 
//! of flat_forward_list_traits for T
//! A is default initialized to std::allocator for T
//!
template <typename T,
          typename TT,
          typename A>
inline void swap(flat_forward_list<T, TT, A> &lhs, flat_forward_list<T, TT, A> &rhs) 
            noexcept (std::allocator_traits<A>::propagate_on_container_swap::value ||
                      std::allocator_traits<A>::propagate_on_container_move_assignment::value) {
    lhs.swap(rhs);
}
//!
//! @tparam T - element type
//! @tparam TT - element type traits
//! @tparam A - allocator type that should be used for this container
//! @param c - container
//!
//! @details 
//! TT is aefault initialized to specialization 
//! of flat_forward_list_traits for T
//! A is default initialized to std::allocator for T
//!
template <typename T,
          typename TT,
          typename A>
inline typename flat_forward_list<T, TT, A>::iterator begin(flat_forward_list<T, TT, A> &c) noexcept {
    return c.begin();
}
//!
//! @tparam T - element type
//! @tparam TT - element type traits
//! @tparam A - allocator type that should be used for this container
//! @param c - container
//!
//! @details 
//! TT is aefault initialized to specialization 
//! of flat_forward_list_traits for T
//! A is default initialized to std::allocator for T
//!
template <typename T,
          typename TT,
          typename A>
inline typename flat_forward_list<T, TT, A>::const_iterator begin(flat_forward_list<T, TT, A> const &c) noexcept {
    return c.begin();
}
//!
//! @tparam T - element type
//! @tparam TT - element type traits
//! @tparam A - allocator type that should be used for this container
//! @param c - container
//!
//! @details 
//! TT is aefault initialized to specialization 
//! of flat_forward_list_traits for T
//! A is default initialized to std::allocator for T
//!
template <typename T,
          typename TT,
          typename A>
inline typename flat_forward_list<T, TT, A>::const_iterator cbegin(flat_forward_list<T, TT, A> const &c) noexcept {
    return c.cbegin();
}
//!
//! @tparam T - element type
//! @tparam TT - element type traits
//! @tparam A - allocator type that should be used for this container
//! @param c - container
//!
//! @details 
//! TT is aefault initialized to specialization 
//! of flat_forward_list_traits for T
//! A is default initialized to std::allocator for T
//!
template <typename T,
          typename TT,
          typename A>
inline typename flat_forward_list<T, TT, A>::iterator end(flat_forward_list<T, TT, A> &c) noexcept {
    return c.end();
}
//!
//! @tparam T - element type
//! @tparam TT - element type traits
//! @tparam A - allocator type that should be used for this container
//! @param c - container
//!
//! @details 
//! TT is aefault initialized to specialization 
//! of flat_forward_list_traits for T
//! A is default initialized to std::allocator for T
//!
template <typename T,
          typename TT,
          typename A>
inline typename flat_forward_list<T, TT, A>::const_iterator end(flat_forward_list<T, TT, A> const &c) noexcept {
    return c.end();
}
//!
//! @tparam T - element type
//! @tparam TT - element type traits
//! @tparam A - allocator type that should be used for this container
//! @param c - container
//!
//! @details 
//! TT is aefault initialized to specialization 
//! of flat_forward_list_traits for T
//! A is default initialized to std::allocator for T
//!
template <typename T,
          typename TT,
          typename A>
inline typename flat_forward_list<T, TT, A>::const_iterator cend(flat_forward_list<T, TT, A> const &c) noexcept {
    return c.cend();
}
//!
//! @tparam T - element type
//! @tparam TT - element type traits
//! @tparam A - allocator type that should be used for this container
//! @param c - container
//!
//! @details 
//! TT is aefault initialized to specialization 
//! of flat_forward_list_traits for T
//! A is default initialized to std::allocator for T
//!
template <typename T,
          typename TT,
          typename A>
inline typename flat_forward_list<T, TT, A>::iterator last(flat_forward_list<T, TT, A> &c) noexcept {
    return c.last();
}
//!
//! @tparam T - element type
//! @tparam TT - element type traits
//! @tparam A - allocator type that should be used for this container
//! @param c - container
//!
//! @details 
//! TT is aefault initialized to specialization 
//! of flat_forward_list_traits for T
//! A is default initialized to std::allocator for T
//!
template <typename T,
          typename TT,
          typename A>
inline typename flat_forward_list<T, TT, A>::const_iterator last(flat_forward_list<T, TT, A> const &c) noexcept {
    return c.last();
}
//!
//! @tparam T - element type
//! @tparam TT - element type traits
//! @tparam A - allocator type that should be used for this container
//! @param c - container
//!
//! @details 
//! TT is aefault initialized to specialization 
//! of flat_forward_list_traits for T
//! A is default initialized to std::allocator for T
//!
template <typename T,
          typename TT,
          typename A>
inline typename flat_forward_list<T, TT, A>::const_iterator clast(flat_forward_list<T, TT, A> const &c) noexcept {
    return c.clast();
}
//!
//! @typedef pmr_flat_forward_list
//! @brief Use this typedef if you want to use container with polimorfic allocator
//! @tparam T - element type
//! @tparam TT - element type traits
//!
template <typename T,
          typename TT = flat_forward_list_traits<T>>
 using pmr_flat_forward_list = flat_forward_list<T, 
                                                 TT, 
                                                 FFL_PMR::polymorphic_allocator<char>>;
//!
//! @brief Constructs ref or view from container
//! @tparam T - element type
//! @tparam TT - element type traits
//! @tparam A - allocator type
//!
template <typename T,
          typename TT>
template <typename V,
          typename VV,
          typename A,
          typename UNUSED>
flat_forward_list_ref<T, TT>::flat_forward_list_ref(flat_forward_list<V, VV, A> const &c) noexcept 
:   buffer_(c.buff()) {
}
//!
//! @brief Constructs ref or view from container
//! @tparam T - element type
//! @tparam TT - element type traits
//! @tparam A - allocator type
//! @returns referencce to self
//!
template <typename T,
          typename TT>
template <typename V,
          typename VV,
          typename A,
          typename UNUSED>
flat_forward_list_ref<T, TT> &flat_forward_list_ref<T, TT>::operator= (flat_forward_list<V, VV, A> const &c) noexcept {
    buffer_ = c.buff();
    return *this;
}

//!
//! @brief flat_forward_list_validate_has_next_offset
//! @tparam T - element type
//! @tparam TT - element type traits. Detaulted to 
//! specialization flat_forward_list_traits<T>
//! @tparam F - functor used to validate element
//! by default uses default_validate_element_fn<T, TT>
//! @details
//! Validates if buffer contains a valid intrusive flat forward list
//! See commment for flat_forward_list_validate.
//! Users are not expected to use this function directly,
//! instead use flat_forward_list_validate, which will call
//! flat_forward_list_validate_has_next_offset if TT::get_next_offset
//! is defined.
//!
template<typename T,
         typename TT,
         typename F>
constexpr inline std::pair<bool, flat_forward_list_ref<T, TT>> flat_forward_list_validate_has_next_offset(char const *first,
                                                                                                          char const *end,
                                                                                                          F const &validate_element_fn) noexcept {
    using traits_traits = flat_forward_list_traits_traits<T, TT>;
    constexpr auto const type_has_next_offset{ traits_traits::has_next_offset_v };
    static_assert(type_has_next_offset, "traits type must define get_next_offset");

    char const *begin{ first };
    //
    // by default we did not found any valid elements
    //
    char const *last_valid = nullptr;
    //
    // null buffer is defined as valid
    //
    if (first == nullptr) {
        FFL_CODDING_ERROR_IF_NOT(nullptr == end);
        return std::make_pair(true, flat_forward_list_ref<T, TT>{ cast_to_char_ptr(begin),
                                                                  cast_to_char_ptr(last_valid),
                                                                  cast_to_char_ptr(end) });
    }
    //
    // empty buffer is defined as valid
    //
    if (first == end) {
        return std::make_pair(true, flat_forward_list_ref<T, TT>{ cast_to_char_ptr(begin),
                                                                  cast_to_char_ptr(last_valid),
                                                                  cast_to_char_ptr(end) });
    }
    //
    // Can we safely subtract pointers?
    //
    FFL_CODDING_ERROR_IF(end < first);

    size_t remaining_length{ static_cast<size_t>(end - first) };
    bool result = false;
    for (; remaining_length > 0;) {
        //
        // is it large enough to even query next offset?
        //
        if (remaining_length <= 0 || remaining_length < traits_traits::minimum_size()) {
            break;
        }
        //
        // If type has next element offset then validate it before
        // validating the rest of data
        //
        size_t const next_element_offset = traits_traits::get_next_offset(first);
        //
        // Is offset of the next element in bound of the remaining buffer
        //
        if (remaining_length < next_element_offset) {
            break;
        }
        //
        // minimum and next are valid, check rest of the fields
        //
        if (!validate_element_fn(remaining_length, *traits_traits::ptr_to_t(first))) {
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

    return std::make_pair(result, flat_forward_list_ref<T, TT>{ cast_to_char_ptr(begin),
                                                                cast_to_char_ptr(last_valid),
                                                                cast_to_char_ptr(end) });
}

//!
//! @brief flat_forward_list_validate_no_next_offset
//! @tparam T - element type
//! @tparam TT - element type traits. Detaulted to 
//! specialization flat_forward_list_traits<T>
//! @tparam F - functor used to validate element
//! by default uses default_validate_element_fn<T, TT>
//! @details
//! Validates if buffer contains a valid intrusive flat forward list
//! See commment for flat_forward_list_validate.
//! Users are not expected to use this function directly,
//! instead prefer to use flat_forward_list_validate, which will call
//! flat_forward_list_validate_no_next_offset if TT::get_next_offset
//! is NOT defined.
//! You can call this function directly IFF TT::get_next_offset is 
//! defined, but you want to run validation as if it is not defined.
//!
template<typename T,
         typename TT,
         typename F>
constexpr inline std::pair<bool, flat_forward_list_ref<T, TT>> flat_forward_list_validate_no_next_offset(char const *first,
                                                                                                         char const *end,
                                                                                                         F const &validate_element_fn) noexcept {
    char const *begin{ first };
    //
    // For this function to work correctly we do not care if 
    // traits has get_next_offset
    using traits_traits = flat_forward_list_traits_traits<T, TT>;
    //
    // by default we did not found any valid elements
    //
    char const *last_valid = nullptr;
    //
    // null buffer is defined as valid
    //
    if (first == nullptr) {
        FFL_CODDING_ERROR_IF_NOT(nullptr == end);
        return std::make_pair(true, flat_forward_list_ref<T, TT>{ cast_to_char_ptr(begin),
                                                                  cast_to_char_ptr(last_valid),
                                                                  cast_to_char_ptr(end) });
    }
    //
    // empty buffer is defined as valid
    //
    if (first == end) {
        return std::make_pair(true, flat_forward_list_ref<T, TT>{ cast_to_char_ptr(begin),
                                                                  cast_to_char_ptr(last_valid),
                                                                  cast_to_char_ptr(end) });
    }
    //
    // Can we safely subtract pointers?
    //
    FFL_CODDING_ERROR_IF(end < first);

    std::ptrdiff_t remaining_length = end - first;
    bool result = false;
    for (;;) {
        //
        // is it large enough to even query next offset?
        //
        if (remaining_length <= 0 || remaining_length < traits_traits::minimum_size()) {
            //
            // If buffer is too small to fit next element
            // then we are done.
            // If element type does not have next element offset
            // then validation succeeded.
            //
            result = true;
            break;
        }
        //
        // Check that element is valid before asking to calculate
        // element size.
        //
        if (!validate_element_fn(remaining_length, *traits_traits::ptr_to_t(first))) {
            break;
        }
        //
        // Next element starts right after this element ends
        //
        size_t next_element_offset = traits_traits::get_next_offset(first);
        //
        // Element size must be at least min size
        //
        if (next_element_offset < traits_traits::minimum_size()) {
            break;
        }
        //
        // keep going to see if buffer is large enough to hold
        // another element
        //

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

    return std::make_pair(result, flat_forward_list_ref<T, TT>{ cast_to_char_ptr(begin),
                                                                cast_to_char_ptr(last_valid),
                                                                cast_to_char_ptr(end) });
}
//!
//! @brief Validates that buffer contains valid flat forward list
//! and returns a pointer to the last element.
//!
//! @tparam T - element type
//! @tparam TT - element type traits. Detaulted to 
//!              algorithms expects following methods
//!                - get_next_offset
//!                - minimum_size
//!                - validate
//! specialization flat_forward_list_traits<T>
//! @tparam F - functor used to validate element
//! by default uses default_validate_element_fn<T, TT>
//!
//! @param first - start of buffer we are validating
//! @param end - first byte pass the buffer we are validation
//!      buffer size == end - first
//! @param validate_element_fn - a functor that is used to validate
//!      element. For instance if element contains offset to variable
//!      lengths data, it can check that these data are in bound of 
//!      the buffer. Default dunctor calls TT::validate.
//!      You can use noop_validate_element_fn if you do not want 
//!      validate element
//!
//! @return std::pair<id_valid, last_element_offset>
//!          first - result of validation
//!                - true if buffer is null
//!                - true if buffer is empty
//!                - true if we found a valid element with offset to the next element equals 0
//!                - false is all other cases
//!          second - pointer to the last valid element
//!                 - null if no valid elements were found
//!         <true, nullptr>  - buffer is NULL or empty
//!                            It is safe to use iterators.
//!         <false, nullptr> - buffer is too small to query next element offset,
//!                            or offset to next element is pointing beyond end,
//!                            or element fields validation did not pass.
//!                            We did not find any entries that pass validation
//!                            so far.
//!                            Buffer contains no flat list.
//!         <false, ptr>     - same as above, but we did found at least one valid element.
//!                            Buffer contains valid head, but tail is corrupt and have to 
//!                            be either truncated by setting next offset on the last valid 
//!                            element to 0 or if possible by fixing first elements pass the
//!                            last valid element.
//!         <true, ptr>      - buffer contains a valid flat forward list, it is safe
//!                            to use iterators
//!
//! @details
//!     When TT has get_next_offset we will use
//!     flat_forward_list_validate_has_next_offset, otherwise
//!     we call flat_forward_list_validate_no_next_offset
//!     - flat_forward_list_validate_has_next_offset stops
//!       when next element offset is 0
//!     - flat_forward_list_validate_no_next_offset stops
//!       when buffer cannot fit next element
//!
template<typename T,
         typename TT,
         typename F>
constexpr inline std::pair<bool, flat_forward_list_ref<T, TT>> flat_forward_list_validate(char const *first,
                                                                                          char const *end, 
                                                                                          F const &validate_element_fn) noexcept {
    using traits_traits = flat_forward_list_traits_traits<T, TT>;
    constexpr auto const type_has_next_offset{ traits_traits::has_next_offset_v };
    //
    // If TT::get_next_offset is defined then use 
    // flat_forward_list_validate_has_next_offset algorithm
    // otherwise use flat_forward_list_validate_no_next_offset
    // algorithm
    //
    if constexpr (type_has_next_offset) {
        return flat_forward_list_validate_has_next_offset<T, TT, F>(first, end, validate_element_fn);
    } else {
        return flat_forward_list_validate_no_next_offset<T, TT, F>(first, end, validate_element_fn);
    }
}

template<typename T,
         typename TT,
         typename F>
constexpr inline std::pair<bool, flat_forward_list_ref<T, TT>> flat_forward_list_validate(char *first,
                                                                                          char *end, 
                                                                                          F const &validate_element_fn) noexcept {
    using traits_traits = flat_forward_list_traits_traits<T, TT>;
    constexpr auto const type_has_next_offset{ traits_traits::has_next_offset_v };
    //
    // If TT::get_next_offset is defined then use 
    // flat_forward_list_validate_has_next_offset algorithm
    // otherwise use flat_forward_list_validate_no_next_offset
    // algorithm
    //
    if constexpr (type_has_next_offset) {
        return flat_forward_list_validate_has_next_offset<T, TT, F>(first, end, validate_element_fn);
    } else {
        return flat_forward_list_validate_no_next_offset<T, TT, F>(first, end, validate_element_fn);
    }
}
//!
//! @brief Overload that takes pointers to element types
//!
template<typename T,
         typename TT,
         typename F>
constexpr inline std::pair<bool, flat_forward_list_ref<T, TT>> flat_forward_list_validate(T *first,
                                                                                          T *end,
                                                                                          F const &validate_element_fn) noexcept {
    return flat_forward_list_validate<T, TT>(reinterpret_cast<char *>(first),
                                             reinterpret_cast<char *>(end),
                                             validate_element_fn);
}
//!
//! @brief Overload that takes pointers to element types
//!
template<typename T,
         typename TT,
         typename F>
constexpr inline std::pair<bool, flat_forward_list_ref<T, TT>> flat_forward_list_validate(T const *first,
                                                                                          T const *end,
                                                                                          F const &validate_element_fn) noexcept {
    return flat_forward_list_validate<T, TT>(reinterpret_cast<char const *>(first),
                                             reinterpret_cast<char const *>(end),
                                             validate_element_fn);
}
//!
//! @brief Overload that takes unsigned char const pointers
//!
template<typename T,
         typename TT,
         typename F>
constexpr inline std::pair<bool, flat_forward_list_ref<T, TT>> flat_forward_list_validate(unsigned char const *first,
                                                                                          unsigned char const *end,
                                                                                          F const &validate_element_fn) noexcept {
    return flat_forward_list_validate<T, TT>(reinterpret_cast<char const *>(first),
                                             reinterpret_cast<char const *>(end),
                                             validate_element_fn);
}
//!
//! @brief Overload that takes unsigned char pointers
//!
template<typename T,
         typename TT,
         typename F>
constexpr inline std::pair<bool, flat_forward_list_ref<T, TT>> flat_forward_list_validate(unsigned char *first,
                                                                                          unsigned char *end,
                                                                                          F const &validate_element_fn) noexcept {
    return flat_forward_list_validate<T, TT>(reinterpret_cast<char *>(first),
                                             reinterpret_cast<char *>(end),
                                             validate_element_fn);
}
//!
//! @brief Overload that takes void const pointers
//!
template<typename T,
         typename TT,
         typename F>
constexpr inline std::pair<bool, flat_forward_list_ref<T, TT>> flat_forward_list_validate(void const *first,
                                                                                          void const *end,
                                                                                          F const &validate_element_fn) noexcept {
    return flat_forward_list_validate<T, TT>(reinterpret_cast<char const *>(first),
                                             reinterpret_cast<char const *>(end),
                                             validate_element_fn);
}
//!
//! @brief Overload that takes void pointers
//!
template<typename T,
         typename TT,
         typename F>
constexpr inline std::pair<bool, flat_forward_list_ref<T, TT>> flat_forward_list_validate(void *first,
                                                                                          void *end,
                                                                                          F const &validate_element_fn) noexcept {
    return flat_forward_list_validate<T, TT>(reinterpret_cast<char *>(first),
                                             reinterpret_cast<char *>(end),
                                             validate_element_fn);
}

} // namespace iffl
