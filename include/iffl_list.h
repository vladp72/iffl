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
//! @tparam TT type traits
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
//! using my_traits_traits = flat_forward_list_traits_traits<my_traits>;
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
template <typename TT>
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
    using has_get_size_mfn = decltype(std::declval<P &>().get_size(nullptr));
    //!
    //! @typedef has_next_offset_mfn
    //! @tparam P - type we will evaluate this metafunction for. 
    //!             Traits type should be used here.
    //! !brief Metafunction that detects if traits include get_next_offset
    //!
    template <typename P>
    using has_next_offset_mfn = decltype(std::declval<P &>().get_next_offset(nullptr));
    //!
    //! @typedef can_set_next_offset_mfn
    //! @tparam P - type we will evaluate this metafunction for. 
    //!             Traits type should be used here.
    //! !brief Metafunction that detects if traits include set_next_offset
    //!
    template <typename P>
    using can_set_next_offset_mfn = decltype(std::declval<P &>().set_next_offset(nullptr, 0));
    //!
    //! @typedef can_validate_mfn
    //! @tparam P - type we will evaluate this metafunction for. 
    //!             Traits type should be used here.
    //! !brief Metafunction that detects if traits include validate
    //!
    template <typename P>
    using can_validate_mfn = decltype(std::declval<P &>().validate(0, nullptr));
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
        return size_with_padding_t{ type_traits::get_size(buffer) };
    }
    //!
    //! @brief Asks type traits to validate element. 
    //! @details If traits do not have validate method
    //! @param buffer_size - size of the buffer used by this element
    //! @param buffer - pointer to the begining of the element
    //! @return element size wrapped into size_with_padding_t
    //!
    constexpr static bool validate(size_t buffer_size, char const *buffer) noexcept {
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
            return type_traits::get_next_offset(buffer);
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
        return type_traits::set_next_offset(buffer, size);
    }
    //!
    //! @brief Prints information about what traits_traits discovered
    //! about traits class. 
    //!
    static void print_traits_info() noexcept {
        type_info const & ti = typeid(type_traits);

        printf("type \"%s\" {\n", ti.name());

        if constexpr (has_minimum_size_v) {
            printf("  minimum_size    : yes -> %zu\n", minimum_size());
        } else {
            printf("  minimum_size    : no \n");
        }

        if constexpr (has_get_size_v) {
            printf("  get_size        : yes\n");
        } else {
            printf("  get_size        : no \n");
        }

        if constexpr (has_next_offset_v) {
            printf("  get_next_offset : yes\n");
        } else {
            printf("  get_next_offset : no \n");
        }

        if constexpr (can_set_next_offset_v) {
            printf("  set_next_offset : yes\n");
        } else {
            printf("  set_next_offset : no \n");
        }

        if constexpr (can_validate_v) {
            printf("  validate        : yes\n");
        } else {
            printf("  validate        : no \n");
        }

        if constexpr (has_alignment_v) {
            printf("  alignment       : yes -> %zu\n", alignment);
        } else {
            printf("  alignment       : no \n");
        }
        printf("}\n");
    }
};

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
    //! @param buffer - pointer to the buffer used by the element
    //!
    bool operator() (size_t buffer_size, char const *buffer) const noexcept {
        return TT::validate(buffer_size, buffer);
    }
};
//!
//! @class noop_validate_element_fn
//! @details Does nothing
//!
struct noop_validate_element_fn {
    //!
    //! @brief Function call iterator that validates element
    //! @details This functor noops validation 
    //!
    bool operator() (size_t, char const *) const noexcept {
        return true;
    }
};

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
         typename TT = flat_forward_list_traits<T>,
         typename F = default_validate_element_fn<T, TT>>
constexpr inline std::pair<bool, char const *> flat_forward_list_validate_has_next_offset(char const *first,
                                                                                          char const *end,
                                                                                          F const &validate_element_fn = default_validate_element_fn<T, TT>{}) noexcept {
    using traits_traits = flat_forward_list_traits_traits<TT>;
    constexpr auto const type_has_next_offset{ traits_traits::has_next_offset_v };
    static_assert(type_has_next_offset, "traits type must define get_next_offset");
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
        size_t next_element_offset = traits_traits::get_next_offset(first);
        //
        // Is offset of the next element in bound of the remaining buffer
        //
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
         typename TT = flat_forward_list_traits<T>,
         typename F = default_validate_element_fn<T, TT>>
constexpr inline std::pair<bool, char const *> flat_forward_list_validate_no_next_offset(char const *first,
                                                                                         char const *end,
                                                                                         F const &validate_element_fn = default_validate_element_fn<T, TT>{}) noexcept {
    //
    // For this function to work correctly we do not care if 
    // traits has get_next_offset
    using traits_traits = flat_forward_list_traits_traits<TT>;
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
        if (!validate_element_fn(remaining_length, first)) {
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

    return std::make_pair(result, last_valid);
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
         typename TT = flat_forward_list_traits<T>,
         typename F = default_validate_element_fn<T, TT>>
constexpr inline std::pair<bool, char const *> flat_forward_list_validate(char const *first,
                                                                          char const *end, 
                                                                          F const &validate_element_fn = default_validate_element_fn<T, TT>{}) noexcept {
    using traits_traits = flat_forward_list_traits_traits<TT>;
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
//! @brief Overload that takes non-const pointers to buffers
//!
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
//!
//! @brief Overload that takes pointers to element types
//!
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
//!
//! @brief Overload that takes unsigned char const pointers
//!
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
//!
//! @brief Overload that takes unsigned char pointers
//!
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
//!
//! @brief Overload that takes void const pointers
//!
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
//!
//! @brief Overload that takes void pointers
//!
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
class flat_forward_list_iterator_t {
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
    //! @brief Element pointers type
    //!
    using pointer = T*;
    //!
    //! @typedef reference
    //! @brief Element reference type
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
    using traits_traits = flat_forward_list_traits_traits<TT>;
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
    //! @brief asignment operator from char * 
    //! @param p - pointer to the buffer that contains element
    //! @return returns a reference to this object
    //!
    constexpr flat_forward_list_iterator_t &operator= (buffer_char_pointer p) noexcept {
        p_ = p;
        return *this;
    }
    //!
    //! @brief asignment operator from T * 
    //! @param p - pointer to the element
    //! @return returns a reference to this object
    //!
    constexpr flat_forward_list_iterator_t &operator= (T *p) noexcept {
        p_ = (buffer_char_pointer)p;
        return *this;
    }
    //!
    //! @brief asignment operator from unsigned char * 
    //! @param p - pointer to the buffer that contains element
    //! @return returns a reference to this object
    //!
    constexpr flat_forward_list_iterator_t &operator= (buffer_unsigned_char_pointer p) noexcept {
        p_ = (buffer_char_pointer )p;
        return *this;
    }
    //!
    //! @brief asignment operator from void * 
    //! @param p - pointer to the buffer that contains element
    //! @return returns a reference to this object
    //!
    constexpr flat_forward_list_iterator_t &operator= (buffer_void_pointer p) noexcept {
        p_ = (buffer_char_pointer )p;
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
        size_t next_offset = traits_traits::get_next_offset(p_);
        if (0 == next_offset) {
            p_ = nullptr;
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
            p_ = nullptr; 
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
        return *(T *)(p_);
    }
    //!
    //! @brief pointer operator. 
    //! @return Returns a pointer to the element pointed by iterator
    //!
    constexpr T *operator->() const noexcept {
        return (T *)(p_);
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
template <typename T,
          typename TT = flat_forward_list_traits<T>,
          typename A = std::allocator<char>>
class flat_forward_list : private A {
public:

    //
    // Technically we need T to be 
    // - trivialy destructable
    // - trivialy constructable
    // - trivialy movable
    // - trivialy copyable
    //
    static_assert(std::is_pod_v<T>, "T must be a Plain Old Definition");

    using value_type = T;
    using pointer = T * ;
    using const_pointer = T const *;
    using reference = T & ;
    using const_reference = T const &;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using buffer_value_type = char;
    using const_buffer_value_type = char const;
    using traits = TT;
    using traits_traits = flat_forward_list_traits_traits<TT>;
    using range_t = typename traits_traits::range_t;
    using size_with_padding_t = typename traits_traits::size_with_padding_t;
    using offset_with_aligment_t = typename traits_traits::offset_with_aligment_t;
    using allocator_type = A ;
    using allocator_type_traits = std::allocator_traits<A>;
    using buffer_pointer = char *;
    using const_buffer_pointer = char const *;
    using buffer_reference = char & ;
    using const_buffer_reference = char const &;

    //!
    //! pointer to the start of buffer,
    //! used buffer size,
    //! total buffer size
    //!
    using detach_type_as_size = std::tuple<char *, size_t, size_t>;
    //!
    //! pointer to the start of buffer,
    //! pointer to the start of last element,
    //! pointer to the buffer end
    //!
    using detach_type_as_pointers = std::tuple<char *, char *, char *>;

    using iterator = flat_forward_list_iterator<T, TT>;
    using const_iterator = flat_forward_list_const_iterator<T, TT>;

    inline static size_type const npos = iffl_npos;

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
        : A(allocator_type_traits::select_on_container_copy_construction(other.get_allocator())) {
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
        attach(buffer, buffer_size);
    }

    template <typename AA = A>
    flat_forward_list(char const *buffer,
                      size_t buffer_size,
                      AA &&a = AA{}) noexcept
        : A(std::forward<AA>(a)) {
        copy_from_buffer(buffer, buffer_size);
    }

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

    flat_forward_list &operator= (flat_forward_list const &other) {
        if (this != &other) {
            //
            // If we are propagating allocator then delete old data
            // before changing allocator
            //
            if constexpr (allocator_type_traits::propagate_on_container_copy_assignment::value) {
                clear();
                *static_cast<A *>(this) = allocator_type_traits::select_on_container_copy_construction(other.get_allocator());
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
        if (last_element) {
            FFL_CODDING_ERROR_IF_NOT(buffer_begin < last_element && last_element < buffer_end);
        } else {
            FFL_CODDING_ERROR_IF_NOT(buffer_begin < buffer_end);
        }
        clear();
        buffer_begin_ = buffer_begin;
        buffer_end_ = buffer_end;
        last_element_ = last_element;
    }

    bool attach(char *buffer,
                size_t buffer_size) noexcept {
        FFL_CODDING_ERROR_IF(buffer_begin_ == buffer);
        auto[is_valid, last_valid] = flat_forward_list_validate<T, TT>(buffer, 
                                                                       buffer + buffer_size);
        attach(buffer, 
               is_valid ? last_valid : nullptr, 
               buffer + buffer_size);
        return is_valid;
    }

    void copy_from_buffer(char const *buffer_begin, 
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
        copy_from_buffer(buffer_begin,
                         is_valid ? last_valid : nullptr,
                         buffer_end);

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
        return allocator_type_traits::max_size(get_allocator());
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

    void tail_shrink_to_fit() {
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
            if (nullptr != last_element_) {
                copy_data(new_buffer, buffer_begin_, prev_sizes.used_capacity_unaligned);
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

            bool is_valid{ true };
            char *last_valid{ last_element_ };
            //
            // If we are shrinking below used capacity then last element will
            // be removed, and we need to do linear search for the new last element 
            // that would fit new buffer size.
            //
            if (prev_sizes.used_capacity_unaligned > size) {
                std::tie(is_valid, last_valid) = flat_forward_list_validate<T, TT>(buffer_begin_, buffer_begin_ + size);
                if (is_valid) {
                    FFL_CODDING_ERROR_IF_NOT(last_valid == last_element_);
                } else {
                    FFL_CODDING_ERROR_IF_NOT(last_valid != last_element_);
                }
            }

            if (last_valid) {
                size_type new_last_element_offset = last_valid - buffer_begin_;

                size_with_padding_t last_valid_element_size{ traits_traits::get_size(last_valid) };
                size_type new_used_capacity = new_last_element_offset + last_valid_element_size.size_not_padded();;
                
                set_no_next_element(last_valid);
                
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

    template <typename F 
             // ,typename ... P
             >
    void emplace_back(size_type element_size,
                      F const &fn
                      //,P&& ... p
                       ) {
        validate_pointer_invariants();

        FFL_CODDING_ERROR_IF(element_size < traits_traits::minimum_size());
      
        char *new_buffer{ nullptr };
        size_t new_buffer_size{ 0 };
        auto deallocate_buffer{ make_scoped_deallocator(&new_buffer, &new_buffer_size) };

        all_sizes prev_sizes{get_all_sizes()};
      
        char *cur{ nullptr };

        //
        // We are appending. New last element does not have to be padded 
        // since there will be no elements after. We do need to padd the 
        // previous last element so the new last element will be padded
        //
        if (prev_sizes.remaining_capacity_for_append < element_size) {
            new_buffer_size = traits_traits::roundup_to_alignment(prev_sizes.total_capacity) + 
                              (element_size - prev_sizes.remaining_capacity_for_append);
            new_buffer = allocate_buffer(new_buffer_size);
            cur = new_buffer + prev_sizes.used_capacity_aligned;
        } else {
            cur = buffer_begin_ + prev_sizes.used_capacity_aligned;
        }

        // fn(cur, element_size, std::forward<P>(p)...);
        fn(cur, element_size);

        set_no_next_element(cur);
        //
        // After element was constructed it cannot be larger than 
        // size requested for this element.
        //
        size_with_padding_t cur_element_size{ traits_traits::get_size(cur) };
        FFL_CODDING_ERROR_IF(element_size < cur_element_size.size_not_padded());
        //
        // element that used to be last becomes
        // an element in the middle so we need to change 
        // its next element pointer
        //
        if (last_element_) {
            set_next_offset(last_element_, prev_sizes.last_element_size_padded);
        }
        //
        // swap new buffer and new buffer
        //
        if (new_buffer != nullptr) {
            //
            // If we are reallocating buffer then move existing 
            // elements to the new buffer. 
            //
            if (buffer_begin_) {
                copy_data(new_buffer, buffer_begin_, prev_sizes.used_capacity_unaligned);
            }
            commit_new_buffer(new_buffer, new_buffer_size);
        }
        //
        // Element that we've just addede is the new last element
        //
        last_element_ = cur;

        validate_pointer_invariants();
        validate_data_invariants();
    }

    void pop_back() noexcept {
        validate_pointer_invariants();

        FFL_CODDING_ERROR_IF(empty());
        
        if (has_exactly_one_entry()) {
            //
            // The last element is also the first element
            //
            last_element_ = nullptr;
        } else {
            //
            // Find element before last
            //
            size_t last_element_start_offset{ static_cast<size_t>(last_element_ - buffer_begin_) };
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
            last_element_ = element_before_it.get_ptr();
        }
        
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

    template <typename F
              //, typename ... P
             >
    iterator emplace(iterator const &it,
                     size_type new_element_size, 
                     F const &fn
                     //, P&& ... p
                     ) {

        validate_pointer_invariants();
        validate_iterator(it);

        FFL_CODDING_ERROR_IF(new_element_size < traits_traits::minimum_size());

        if (end() == it) {
            //emplace_back(new_element_size, fn, std::forward<P>(p)...);
            emplace_back(new_element_size, fn);
            return last();
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

        all_sizes prev_sizes{ get_all_sizes() };
        range_t element_range{ this->range_unsafe(it) };
        //
        // Note that in this case old element that was in that position
        // is a part of the tail
        //
        size_type tail_size{ prev_sizes.used_capacity_unaligned - element_range.begin() };

        char *begin{ nullptr };
        char *cur{ nullptr };
        //
        // We are inserting new element in the middle. 
        // We do not need make last eleement aligned. 
        // We need to add padding for the element that we are inserting to
        // keep element that we are shifting right propertly aligned
        //
        if (prev_sizes.remaining_capacity_for_insert < new_element_size_aligned) {
            new_buffer_size = traits_traits::roundup_to_alignment(prev_sizes.total_capacity) + 
                              (new_element_size_aligned - prev_sizes.remaining_capacity_for_insert);
            new_buffer = allocate_buffer(new_buffer_size);
            cur = new_buffer + element_range.begin();
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
            new_tail_start = begin + element_range.begin() + new_element_size_aligned;
            move_data(new_tail_start, it.get_ptr(), tail_size);
        }
        //
        // construct
        //
        try {
            //fn(cur, new_element_size, std::forward<P>(p)...);
            //
            // Pass to the constructor requested size, not padded size
            //
            fn(cur, new_element_size);
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
        size_with_padding_t cur_element_size{ traits_traits::get_size(cur) };
        if constexpr (traits_traits::has_next_offset_v) {
            FFL_CODDING_ERROR_IF(new_element_size < cur_element_size.size_not_padded() ||
                                 new_element_size_aligned < cur_element_size.size_padded());
        } else {
            FFL_CODDING_ERROR_IF_NOT(new_element_size == cur_element_size.size_not_padded() &&
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
            if (buffer_begin_) {
                copy_data(new_buffer, buffer_begin_, element_range.begin());
                copy_data(cur + new_element_size_aligned, it.get_ptr(), tail_size);
            }
            commit_new_buffer(new_buffer, new_buffer_size);
        }
        //
        // Last element moved ahead by the size of the new inserted element
        //
        last_element_ = buffer_begin_ + prev_sizes.last_element_offset + new_element_size_aligned;

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
              //,typename ... P
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

        FFL_CODDING_ERROR_IF(empty());
        //
        // If we have only one element then simply forget it
        //
        if (has_one_or_no_entry()) {
            last_element_ = nullptr;
            return;
        }
        //
        // Otherwise calculate sizes and offsets
        //
        all_sizes prev_sizes{ get_all_sizes() };
        iterator begin_it{ iterator{ buffer_begin_ } };
        iterator secont_element_it{ begin_it + 1 };
        range_t second_element_range{ this->range_unsafe(secont_element_it) };
        size_type bytes_to_copy{ prev_sizes.used_capacity_unaligned - second_element_range.begin() };
        //
        // Shift all elements after the first element
        // to the start of buffer
        //
        move_data(buffer_begin_, buffer_begin_ + second_element_range.begin(), bytes_to_copy);

        last_element_ -= second_element_range.begin();

        validate_pointer_invariants();
        validate_data_invariants();
    }

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
        FFL_CODDING_ERROR_IF(empty());
        //
        // Find pointer to the element that we are erasing
        //
        iterator element_to_erase_it = it;
        ++element_to_erase_it;
        bool erasing_last_element = (last() == element_to_erase_it);

        if (erasing_last_element) {
            //
            // trivial case when we are deleting last element and this element 
            // is becoming last
            //
            set_no_next_element(it.get_ptr());
            last_element_ = it.get_ptr();
        } else {
            //
            // calculate sizes and offsets
            //
            all_sizes prev_sizes{ get_all_sizes() };
            range_t element_to_erase_range{ this->range_unsafe(element_to_erase_it) };
            size_type tail_size{ prev_sizes.used_capacity_unaligned - element_to_erase_range.buffer_end_unaligned() };
            //
            // Shift all elements after the element that we are erasing
            // to the position where erased element used to be
            //
            move_data(buffer_begin_ + element_to_erase_range.begin(), 
                      buffer_begin_ + element_to_erase_range.buffer_end_unaligned(), tail_size
            );
            last_element_ -= element_to_erase_range.buffer_size_not_padded();
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
        all_sizes prev_sizes{ get_all_sizes() };

        range_t first_element_to_erase_range{ this->range_unsafe(first_element_to_erase_it) };
        range_t last_element_to_erase_range{ this->range_unsafe(last) };

        //
        // Note that element_range returns element size adjusted with padding
        // that is required for the next element to be propertly aligned.
        //
        size_type bytes_to_copy{ prev_sizes.used_capacity_unaligned - last_element_to_erase_range.buffer_end_unaligned() };
        size_type bytes_erased{ last_element_to_erase_range.buffer_end_unaligned() - first_element_to_erase_range.begin() };
        //
        // Shift all elements after the last element that we are erasing
        // to the position where first erased element used to be
        //
        move_data(buffer_begin_ + first_element_to_erase_range.begin(),
                  buffer_begin_ + last_element_to_erase_range.buffer_end_unaligned(),
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
        if (end() != it) {
            last_element_ = it.get_ptr();
            set_no_next_element(last_element_);

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
        if (end() != it) {

            if (it == begin()) {
                last_element_ = nullptr;
                return end();
            }

            range_t element_rtange{ range_unsafe(it) };
            iterator element_before = find_element_before(element_rtange.begin());
            FFL_CODDING_ERROR_IF(end() == element_before);

            erase_all_after(element_before);

            set_no_next_element(element_before.get_ptr());

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

        if (begin() == it) {
            pop_front();
            return begin();
        }

        if (last() == it) {
            pop_back();
            return end();
        } 

        all_sizes prev_sizes{ get_all_sizes() };
        range_t element_range{ range_unsafe(it) };
        //
        // Size of all elements after element that we are erasing,
        // not counting padding after last element.
        //
        size_type tail_size{ prev_sizes.used_capacity_unaligned - element_range.buffer_end_unaligned() };
        //
        // Shifting remaining elements will erase this element.
        //
        move_data(buffer_begin_ + element_range.begin(),
                  buffer_begin_ + element_range.buffer_end_unaligned(),
                  tail_size);

        last_element_ -= element_range.buffer_size_not_padded();

        validate_pointer_invariants();
        validate_data_invariants();

        return it;
    }
    //
    // This is an erase for an half opened range [start, end)
    //
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
        all_sizes prev_sizes{ get_all_sizes() };
        
        range_t start_range = range_unsafe(start);
        range_t end_range = range_unsafe(end);
        size_type bytes_to_copy{ prev_sizes.used_capacity_unaligned - end_range.buffer_end_unaligned() };
        size_type bytes_erased{ end_range.begin() - start_range.begin() };

        move_data(buffer_begin_ + start_range.begin(),
                  buffer_begin_ + end_range.begin(),
                  bytes_to_copy);

        last_element_ -= bytes_erased;

        validate_pointer_invariants();
        validate_data_invariants();

        return start;
    }

    void swap(flat_forward_list &other) noexcept (allocator_type_traits::propagate_on_container_swap::value ||
                                                  allocator_type_traits::propagate_on_container_move_assignment::value) {
        if constexpr (allocator_type_traits::propagate_on_container_swap::value) {
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
            sorted_list.push_back(used_size(i), i.get_ptr());
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
                             : end();
    }

    const_iterator begin() const noexcept {
        validate_pointer_invariants();
        return last_element_ ? const_iterator{ buffer_begin_ }
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

    iterator last() noexcept {
        validate_pointer_invariants();
        return last_element_ ? iterator{ last_element_ }
                             : end();
    }

    const_iterator last() const noexcept {
        validate_pointer_invariants();
        return last_element_ ? const_iterator{ last_element_ }
                             : cend();
    }

    iterator end() noexcept {
        validate_pointer_invariants();
        if (last_element_) {
            if (traits_traits::has_next_offset_v) {
                return iterator{ };
            } else {
                size_with_padding_t last_element_size{ traits_traits::get_size(last_element_) };
                return iterator{ last_element_ + last_element_size.size_padded() };
            }
        } else {
            return iterator{ };
        }
    }

    const_iterator end() const noexcept {
        validate_pointer_invariants();
        if (last_element_) {
            if constexpr (traits_traits::has_next_offset_v) {
                return const_iterator{ };
            } else {
                size_with_padding_t last_element_size{ traits_traits::get_size(last_element_) };
                return const_iterator{ last_element_ + last_element_size.size_padded() };
            }
        } else {
            return const_iterator{ };
        }
    }

    const_iterator cbegin() const noexcept {
        return begin();
    }

    const_iterator clast() const noexcept {
        return last();
    }

    const_iterator cend() const noexcept {
        return end();
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

    void shrink_to_fit() {
        shrink_to_fit(begin(), end());
        tail_shrink_to_fit();
    }

    void shrink_to_fit(iterator const &first, iterator const &end) {
        for (iterator i = first; i != end; ++i) {
            shrink_to_fit(i);
        }
    }

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

        iterator ret_it = element_resize(it,
                                         new_element_size,
                                         [new_element_size] (char *buffer,
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

    iterator element_add_size(iterator const &it,
                              size_type size_to_add) {
        validate_pointer_invariants();
        validate_iterator_not_end(it);

        return element_resize(it, 
                              traits_traits::roundup_to_alignment(used_size(it) + size_to_add),
                              [] (char *buffer,  
                                  size_type old_size, 
                                  size_type new_size) {
                                  //
                                  // must be extending, and
                                  // data must fit new buffer
                                  //
                                  FFL_CODDING_ERROR_IF_NOT(old_size <= new_size);
                                  zero_buffer(buffer + old_size, new_size - old_size);
                                  FFL_CODDING_ERROR_IF_NOT(traits_traits::validate(new_size, buffer));
                              });
    }

    //!
    //! Should take constructor
    //!
    template <typename F
             //,typename ... P
             >
    iterator element_resize(iterator const &it, 
                            size_type new_size, 
                            F const &fn
                            //,P&& ... p
                           ) noexcept {
        //
        // Resize to 0 is same as erase
        //
        if (0 == new_size) {
            return erase(it);
        }

        FFL_CODDING_ERROR_IF(new_size < traits_traits::minimum_size());
        //
        // Separately handle case of resizing the last element 
        // we do not need to deal with tail and padding
        // for the next element
        //
        if (last() == it) {
            return resize_last_element(new_size, fn);
        }
        //
        // Now deal with resizing element in the middle
        // or at the head
        //
        validate_pointer_invariants();
        validate_iterator_not_end(it);

        iterator result_it;

        all_sizes prev_sizes{ get_all_sizes() };
        range_t element_range_before{ range_unsafe(it) };
        //
        // We will change element size by padded size to make sure
        // that next element stays padded
        //
        size_type new_size_padded{ traits_traits::roundup_to_alignment(new_size) };
        //
        // Calculate tail size
        //
        size_type tail_size{ prev_sizes.used_capacity_unaligned - element_range_before.buffer_end_unaligned() };
        //
        // By how much element size is changing
        //
        difference_type element_size_diff{ static_cast<difference_type>(new_size_padded - element_range_before.buffer_size_not_padded()) };
        //
        // This branch handles case when we do not need to reallocate buffer
        // and we have sufficiently large buffer.
        //
        if (element_size_diff < 0 ||
            prev_sizes.remaining_capacity_for_insert >= static_cast<size_type>(element_size_diff)) {

            size_type tail_start_offset = element_range_before.buffer_end_unaligned();
            //
            // If element is getting extended in size then shift tail to the 
            // right to make space for element data, and remember new tail 
            // position
            //
            if (new_size_padded > element_range_before.buffer_size_not_padded()) {
                move_data(buffer_begin_ + element_range_before.begin() + new_size_padded,
                          buffer_begin_ + tail_start_offset,
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
                                            &prev_sizes, 
                                            tail_start_offset, 
                                            tail_size,
                                            &element_range_before] {
                //
                // See how much space elements consumes now
                //
                size_with_padding_t element_size_after = this->size_unsafe(it);
                //
                // New element size must not be larget than size that it is 
                // allowed to grow by
                //
                FFL_CODDING_ERROR_IF(element_size_after.size_not_padded() > new_size);
                //
                // If we have next pointer then next element should start after new_size_padded
                // otherwise if should start after padded size of the element
                //
                range_t element_range_after{ element_range_before.begin(),
                                             element_range_after.begin() + element_size_after.size_not_padded(),
                                             element_range_after.begin() + new_size_padded };
                element_range_after.verify();
                //
                // New evement end must not pass current tail position
                //
                FFL_CODDING_ERROR_IF(element_range_after.buffer_end_unaligned() > tail_start_offset);
                //
                // if size changed, then shift tal to the left
                //
                if (element_range_after.buffer_end_unaligned() != element_range_before.buffer_end_unaligned()) {

                    move_data(buffer_begin_ + element_range_after.buffer_end_unaligned(),
                              buffer_begin_ + tail_start_offset,
                              tail_size);
                    //
                    // calculate by how much tail moved
                    //
                    difference_type tail_shift{ static_cast<difference_type>(element_range_after.buffer_size_not_padded() -
                                                                             element_range_before.buffer_size_not_padded()) };
                    //
                    // Update pointer to last element
                    //
                    last_element_ += tail_shift;
                }

                this->set_next_offset(it.get_ptr(), element_range_after.buffer_size_not_padded());
            }) };

            fn(it.get_ptr(), 
               element_range_before.buffer_size_not_padded(),
               new_size);

            result_it = it;

        } else {
            //
            // Buffer is increasing in size so we will need to reallocate buffer
            //
            char *new_buffer{ nullptr };
            size_type new_buffer_size{ 0 };
            auto deallocate_buffer{ make_scoped_deallocator(&new_buffer, &new_buffer_size) };

            new_buffer_size = prev_sizes.used_capacity_unaligned + new_size_padded - element_range_before.buffer_size_not_padded();
            new_buffer = allocate_buffer(new_buffer_size);
            //
            // copy element that we are changing to the new buffer
            //
            copy_data(new_buffer + element_range_before.begin(),
                      buffer_begin_ + element_range_before.begin(),
                      element_range_before.buffer_size_not_padded());
            //
            // change element
            //
            fn(new_buffer + element_range_before.begin(),
               element_range_before.buffer_size_not_padded(),
               new_size);
            //
            // fn did not throw an exception, and remainder of 
            // function is noexcept
            //
            result_it = iterator{ new_buffer + element_range_before.begin() };
            //
            // copy head
            //
            copy_data(new_buffer, buffer_begin_, element_range_before.begin());
            //
            // See how much space elements consumes now
            //
            size_with_padding_t element_size_after = this->size_unsafe(result_it);
            //
            // New element size must not be larget than size that it is 
            // allowed to grow by
            //
            FFL_CODDING_ERROR_IF(element_size_after.size_not_padded() > new_size);
            //
            // If we have next pointer then next element should start after new_size_padded
            // otherwise if should start after padded size of the element
            //
            range_t element_range_after{ element_range_before.begin(),
                                         element_range_after.begin() + element_size_after.size_not_padded(),
                                         element_range_after.begin() + new_size_padded };
            element_range_after.verify();
            //
            // Copy tail
            //
            move_data(new_buffer + element_range_after.buffer_end_unaligned(),
                      buffer_begin_ + element_range_before.buffer_end_unaligned(),
                      tail_size);
            //
            // commit mew buffer
            //
            commit_new_buffer(new_buffer, new_buffer_size);
            //
            // calculate by how much tail moved
            //
            difference_type tail_shift{ static_cast<difference_type>(element_range_after.buffer_size_not_padded() -
                                                                     element_range_before.buffer_size_not_padded()) };
            //
            // Update pointer to last element
            //
            last_element_ = buffer_begin_ + prev_sizes.last_element_offset + tail_shift;
            //
            // fix offset to the next element
            //
            this->set_next_offset(result_it.get_ptr(), element_range_after.buffer_size_not_padded());
        }

        validate_pointer_invariants();
        validate_data_invariants();
        //
        // Return iterator pointing to the new inserted element
        //
        return result_it;
    }

    size_type required_size(const_iterator const &it) const noexcept {
        validate_pointer_invariants();
        validate_iterator_not_end(it);
        return size_unsafe(it).size_not_padded();
    }

    size_type used_size(const_iterator const &it) const noexcept {
        validate_pointer_invariants();
        validate_iterator_not_end(it);

        return used_size_unsafe(it);
    }

    range_t range(const_iterator const &it) const noexcept {
        validate_iterator_not_end(it);

        return range_unsafe(it);
    }

    //!
    //! closed range [begin, last]
    //!
    range_t closed_range(const_iterator const &begin, const_iterator const &last) const noexcept {
        validate_iterator_not_end(begin);
        validate_iterator_not_end(last);

        return closed_range_unsafe(begin, last);
    }

    //!
    //! half opened range [begin, end)
    //!
    range_t half_open_range(const_iterator const &begin, const_iterator const &end) const noexcept {
        validate_iterator_not_end(begin);
        validate_iterator_not_end(end);

        return half_closed_range_unsafe(begin, end);
    }

    bool contains(const_iterator const &it, size_type position) const noexcept {
        validate_iterator(it);
        if (cend() == it || npos == position) {
            return false;
        }
        range_t const r{ range_unsafe(it) };
        return r.buffer_contains(position);
    }

    iterator find_element_before(size_type position) noexcept {
        validate_pointer_invariants();
        if (empty()) {
            return end();
        }
        auto[is_valid, last_valid] = flat_forward_list_validate<T, TT>(buffer_begin_, buffer_begin_ + position);
        if (last_valid) {
            return iterator{ const_cast<char *>(last_valid) };
        }
        return end();
    }

    const_iterator find_element_before(size_type position) const noexcept {
        validate_pointer_invariants();
        if (empty()) {
            return end();
        }
        auto[is_valid, last_valid] = flat_forward_list_validate<T, TT>(buffer_begin_, 
                                                                       buffer_begin_ + position);
        if (last_valid) {
            return const_iterator{ last_valid };
        }
        return end();
    }

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
        all_sizes s{ get_all_sizes() };
        return s.used_capacity_unaligned;
    }

    size_type total_capacity() const noexcept {
        validate_pointer_invariants();
        return buffer_end_ - buffer_begin_;
    }

    size_type remaining_capacity() const noexcept {
        validate_pointer_invariants();
        all_sizes s{ get_all_sizes() };
        return s.remaining_capacity;
    }

    void fill_padding(int fill_byte = 0, 
                      bool zero_unused_capacity = true) noexcept {
        validate_pointer_invariants();
        //
        // Fill gaps between any elements up to the last
        //
        auto last{ this->last() };
        //
        // zero padding of each element
        //
        for (auto it = begin(); last != it; ++it) {
            range_t element_range{ range_unsafe(it) };
            element_range.fill_unused_capacity_data_ptr(it.get_ptr(), fill_byte);
        }     
        //
        // If we told so then zero tail
        //
        if (zero_unused_capacity) {
            all_sizes prev_sizes{ get_all_sizes() };
            if (prev_sizes.used_capacity_unaligned > 0) {
                size_type last_element_end{ prev_sizes.last_element_offset + prev_sizes.last_element_size_not_padded };
                size_type unuset_tail_size{ prev_sizes.total_capacity - last_element_end };
                fill_buffer(buffer_begin_ + last_element_end,
                            fill_byte,
                            unuset_tail_size);
            }
        }
        
        validate_pointer_invariants();
        validate_data_invariants();
    }

private:

    //!
    //! Should take constructor
    //!
    template <typename F
             //,typename ... P
             >
    iterator resize_last_element( size_type new_size, 
                                  F const &fn
                                  //,P&& ... p
                                  ) noexcept {

        validate_pointer_invariants();

        all_sizes prev_sizes{ get_all_sizes() };
        difference_type element_size_diff{ static_cast<difference_type>(new_size - prev_sizes.last_element_size_not_padded) };
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
            prev_sizes.remaining_capacity_for_insert >= static_cast<size_type>(element_size_diff)) {

            fn(last_element_, 
               prev_sizes.last_element_size_not_padded,
               new_size);

        } else {

            char *new_buffer{ nullptr };
            size_type new_buffer_size{ 0 };
            auto deallocate_buffer{ make_scoped_deallocator(&new_buffer, &new_buffer_size) };

            new_buffer_size = prev_sizes.used_capacity_unaligned + new_size - prev_sizes.last_element_size_not_padded;
            new_buffer = allocate_buffer(new_buffer_size);

            char *new_last_ptr{ new_buffer + prev_sizes.last_element_offset };
            //
            // copy element
            //
            copy_data(new_last_ptr,
                      buffer_begin_ + prev_sizes.last_element_offset,
                      prev_sizes.last_element_size_not_padded);
            //
            // change element
            //
            fn(new_last_ptr,
               prev_sizes.last_element_size_not_padded,
               new_size);
            //
            // copy head
            //
            copy_data(new_buffer, buffer_begin_, prev_sizes.last_element_offset);
            //
            // commit mew buffer
            //
            commit_new_buffer(new_buffer, new_buffer_size);
            last_element_ = new_last_ptr;
        }

        validate_pointer_invariants();
        validate_data_invariants();

        return last();
    }

    constexpr bool has_one_or_no_entry() const noexcept {
        return last_element_ == buffer_begin_;
    }

    constexpr bool has_exactly_one_entry() const noexcept {
        return nullptr != last_element_ && 
               last_element_ == buffer_begin_;
    }


    constexpr static void set_no_next_element(char *buffer) noexcept {
        set_next_offset(buffer, 0);
    }

    constexpr static void set_next_offset(char *buffer, size_t size) noexcept {
        if constexpr (traits_traits::has_next_offset_v) {
            traits_traits::set_next_offset(buffer, size);
        } else {
            buffer;
            size;
        }
    }

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
            buffer_begin_ = allocate_buffer(other_sizes.used_capacity_unaligned);
            copy_data(buffer_begin_, other.buffer_begin_, other_sizes.used_capacity_unaligned);
            buffer_end_ = buffer_begin_ + other_sizes.used_capacity_unaligned;
            last_element_ = buffer_begin_ + other_sizes.last_element_offset;
        }
    }

    char *allocate_buffer(size_t buffer_size) {
        char *ptr{ allocator_type_traits::allocate(*this, buffer_size, 0) };
        FFL_CODDING_ERROR_IF(nullptr == ptr);
        //FFL_CODDING_ERROR_IF(*reinterpret_cast<size_t const*>(&ptr) & 0xFFF);
        return ptr;
    }

    void deallocate_buffer(char *buffer, size_t buffer_size) {
        FFL_CODDING_ERROR_IF(0 == buffer_size || nullptr == buffer);
        allocator_type_traits::deallocate(*this, buffer, buffer_size);
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
        if (last_element_) {
            //
            // For element types that have offset of next element use entire buffer for validation
            // for element types that do not we have to limit by the end of the last element.
            // If there is sufficient reservation after last element then validate would not know 
            // where to stop, and might fail.
            //
            size_type buffer_lenght{ static_cast<size_type>(buffer_end_ - buffer_begin_) };
            if constexpr (!traits_traits::has_next_offset_v) {
                auto[last_element_size_not_padded, last_element_size_padded] = traits_traits::get_size(last_element_);
                buffer_lenght = last_element_size_not_padded;
            }
            auto[valid, last] = flat_forward_list_validate<T, TT>(buffer_begin_, last_element_ + buffer_lenght);
            FFL_CODDING_ERROR_IF_NOT(valid);
            FFL_CODDING_ERROR_IF_NOT(last == last_element_);

            if (last_element_) {
                size_with_padding_t last_element_size = traits_traits::get_size(last_element_);
                size_type last_element_offset{ static_cast<size_type>(last_element_ - buffer_begin_) };
                FFL_CODDING_ERROR_IF(buffer_lenght < (last_element_offset + last_element_size.size_not_padded()));
            }
        }
#endif //FFL_DBG_CHECK_DATA_VALID
    }

    void validate_pointer_invariants() const noexcept {
        //
        // empty() calls this method so we canot call it here
        //
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
        if (empty()) {
            FFL_CODDING_ERROR_IF_NOT(cend() == it);
        } else {
            FFL_CODDING_ERROR_IF_NOT(cend() == it ||
                                     buffer_begin_ <= it.get_ptr() && it.get_ptr() <= last_element_);
            validate_compare_to_all_valid_elements(it);
        }
    }

    void validate_iterator_not_end(const_iterator const &it) const noexcept {
        //
        // Used in case when end iterator is meaningless.
        // Validates that container is not empty,
        // and iterator is pointing somewhere between
        // begin of the buffer and start of the first element
        //
        FFL_CODDING_ERROR_IF(cend() == it);
        FFL_CODDING_ERROR_IF(const_iterator{} == it);
        FFL_CODDING_ERROR_IF_NOT(buffer_begin_ <= it.get_ptr() && it.get_ptr() <= last_element_);
        validate_compare_to_all_valid_elements(it);
    }

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

    size_with_padding_t size_unsafe(const_iterator const &it) const noexcept {
        return  traits_traits::get_size(it.get_ptr());
    }


    size_type used_size_unsafe(const_iterator const &it) const noexcept {
        if constexpr (traits_traits::has_next_offset_v) {
            size_type next_offset = traits::get_next_offset(it.get_ptr());
            if (0 == next_offset) {
                size_with_padding_t s{ traits_traits::get_size(it.get_ptr()) };
                //
                // Last element
                //
                next_offset = s.size_not_padded();
            }
            return next_offset;
        } else {
            size_with_padding_t s{ traits_traits::get_size(it.get_ptr()) };
            //
            // buffer might end without including padding for the last element
            //
            if (end == it) {
                return s.size_not_padded();
            } else {
                return s.size_padded();
            }
        }
    }

    range_t range_unsafe(const_iterator const &it) const noexcept {
        size_with_padding_t s{ traits_traits::get_size(it.get_ptr()) };
        range_t r{};
        r.buffer_begin = it.get_ptr() - buffer_begin_;
        r.data_end = r.begin() + s.size_not_padded();
        if constexpr (traits_traits::has_next_offset_v) {
            size_type next_offset = traits::get_next_offset(it.get_ptr());
            if (0 == next_offset) {
                FFL_CODDING_ERROR_IF(last() != it);
                r.buffer_end = r.begin() + s.size_padded();
            } else {
                r.buffer_end = r.begin() + next_offset;
            }
        } else {
            if (end() == it) {
                r.buffer_end = r.begin() + s.size_not_padded();
            } else {
                r.buffer_end = r.begin() + s.size_padded();
            }
        }
        return r;
    }

    //!
    //! closed range [first, last]
    //!
    range_t closed_range_unsafe(const_iterator const &first, const_iterator const &last) const noexcept {
        if (first == last) {
            return element_range_unsafe(first);
        } else {
            range_t first_element_range{ range_unsafe(first) };
            range_t last_element_range{ range_unsafe(last) };

            return range_t{ first_element_range.begin,
                            last_element_range.data_end,
                            last_element_range.buffer_end };
        }
    }

    //!
    //! half closed range [first, end)
    //!
    range_t half_open_range_usafe(const_iterator const &first, 
                                  const_iterator const &end) const noexcept {
        validate_iterator_not_end(first);
        validate_iterator(end);

        if (this->end() == end) {
            return closed_range_usafe(first, last());
        }

        size_t end_begin{ static_cast<size_t>(end->get_ptr() - buffer_begin_) };
        iterator last{ find_element_before(end_begin) };

        return closed_range_usafe(first, last);
    }

    struct all_sizes {
        //!
        //! Starting offset of the last element
        //!
        size_type last_element_offset{ 0 };
        //!
        //! End offset of the last element without
        //! padding added to the element size.
        //!
        size_type last_element_size_not_padded{ 0 };
        //!
        //! End offset of the last element with
        //! padding added to the element size.
        //!
        size_type last_element_size_padded{ 0 };
        //!
        //! Offset of the unaligned position after last element
        //!
        size_type used_capacity_unaligned{ 0 };
        //!
        //! Offset of aligned position after last element
        //!
        size_type used_capacity_aligned{ 0 };
        //!
        //! Size of buffer
        //!
        size_type total_capacity{ 0 };
        //!
        //! How much free space we have in buffer if we do not need to 
        //! padd last element offset
        //!
        size_type remaining_capacity_for_append{ 0 };
        //!
        //! How much free space we have in buffer if we need to 
        //! padd last element offset
        //!
        size_type remaining_capacity_for_insert{ 0 };
    };

    all_sizes get_all_sizes() const noexcept {
        all_sizes s;

        s.total_capacity = buffer_end_ - buffer_begin_;

        if (nullptr != last_element_) {
           
            size_with_padding_t last_element_size = traits_traits::get_size(last_element_);

            s.last_element_offset = last_element_ - buffer_begin_;
            s.last_element_size_not_padded = last_element_size.size_not_padded();
            s.last_element_size_padded = last_element_size.size_padded();
            
            s.used_capacity_unaligned = s.last_element_offset + s.last_element_size_not_padded;
            s.used_capacity_aligned = s.last_element_offset + s.last_element_size_padded;
        }
        //
        // When we are inserting new element in the middle we need to make sure inserted element
        // is padded, but we do not need to padd tail element
        //
        FFL_CODDING_ERROR_IF(s.total_capacity < s.used_capacity_unaligned);
        s.remaining_capacity_for_insert = s.total_capacity - s.used_capacity_unaligned;
        // 
        // if (s.total_capacity > s.used_capacity_unaligned) {
        //     s.remaining_capacity_for_insert = s.total_capacity - s.used_capacity_unaligned;
        // } else {
        //     s.remaining_capacity_for_insert = 0;
        // }
        //
        // If we are appending then we need to padd current last element, but new inserted 
        // element does not have to be padded
        //
        if (s.total_capacity <= s.used_capacity_aligned) {
            s.remaining_capacity_for_append = 0;
        } else {
            s.remaining_capacity_for_append = s.total_capacity - s.used_capacity_aligned;
        }

        return s;
    }

    static void raise_invalid_buffer_exception(char const *message) {
        throw std::system_error{ std::make_error_code(std::errc::invalid_argument), message };
    }

    //!
    //! pointer to the beginninng of buffer
    //! and first element if we have any
    //!
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

//!
//! Use this typedef if you want to use container with polimorfic allocator
//!
template <typename T,
          typename TT = flat_forward_list_traits<T>>
 using pmr_flat_forward_list = flat_forward_list<T, 
                                                 TT, 
                                                 std::pmr::polymorphic_allocator<char>>;

} // namespace iffl
