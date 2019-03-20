# Intrusive Flat Forward List for plain old definition (POD) types

This header only library that implements intrusive flat forward list (iffl).

https://github.com/vladp72/iffl/tree/master/include

## Motivation

Often times when dealing with OS or just C interface we need to pass in or parse a a buffer that contains a linked list of variable size structs. 

These pods have following general structure:
```
                      ------------------------------------------------------------
                      |                                                          |
                      |                                                          V
 | <fields> | offset to next element | <offsets of data> | [data] | [padding] || [next element] ...
 |                        header                         | [data] | [padding] || [next element] ...
 ```

Examples are from Windows, but I am sure there is plenty of samples in Unix:
```
typedef struct _FILE_FULL_EA_INFORMATION {
  ULONG  NextEntryOffset; // intrusive hook with offset of the next element
  UCHAR  Flags;
  UCHAR  EaNameLength;
  USHORT EaValueLength;
  CHAR   EaName[1];
} FILE_FULL_EA_INFORMATION, *PFILE_FULL_EA_INFORMATION;
```
[FILE_FULL_EA_INFORMATION documentation](https://docs.microsoft.com/en-us/windows-hardware/drivers/ddi/content/wdm/ns-wdm-_file_full_ea_information)
```
typedef struct _FILE_NOTIFY_EXTENDED_INFORMATION {
  DWORD         NextEntryOffset; // intrusive hook with offset of the next element
  DWORD         Action;
  LARGE_INTEGER CreationTime;
  LARGE_INTEGER LastModificationTime;
  LARGE_INTEGER LastChangeTime;
  LARGE_INTEGER LastAccessTime;
  LARGE_INTEGER AllocatedLength;
  LARGE_INTEGER FileSize;
  DWORD         FileAttributes;
  DWORD         ReparsePointTag;
  LARGE_INTEGER FileId;
  LARGE_INTEGER ParentFileId;
  DWORD         FileNameLength;
  WCHAR         FileName[1];
} FILE_NOTIFY_EXTENDED_INFORMATION, *PFILE_NOTIFY_EXTENDED_INFORMATION;
[FILE_NOTIFY_EXTENDED_INFORMATION documentation](https://docs.microsoft.com/en-us/windows/desktop/api/winnt/ns-winnt-_file_notify_extended_information)
```
output for the following information classes from [FILE_INFO_BY_HANDLE_CLASS](https://msdn.microsoft.com/en-us/8f02e824-ca41-48c1-a5e8-5b12d81886b5)
```
FileIdBothDirectoryInfo
FileIdBothDirectoryRestartInfo
FileIoPriorityHintInfo
FileRemoteProtocolInfo
FileFullDirectoryInfo
FileFullDirectoryRestartInfo
FileStorageInfo
FileAlignmentInfo
FileIdInfo
FileIdExtdDirectoryInfo
FileIdExtdDirectoryRestartInfo
For example output buffer for
```
For eample
```
GetFileInformationByHandleEx(file_handle, FileIdBothDirectoryInfo, buffer, buffer_size);
```
will be filled with structures
```
typedef struct _FILE_ID_BOTH_DIR_INFO {
  DWORD         NextEntryOffset; // intrusive hook with offset of the next element
  DWORD         FileIndex;
  LARGE_INTEGER CreationTime;
  LARGE_INTEGER LastAccessTime;
  LARGE_INTEGER LastWriteTime;
  LARGE_INTEGER ChangeTime;
  LARGE_INTEGER EndOfFile;
  LARGE_INTEGER AllocationSize;
  DWORD         FileAttributes;
  DWORD         FileNameLength;
  DWORD         EaSize;
  CCHAR         ShortNameLength;
  WCHAR         ShortName[12];
  LARGE_INTEGER FileId;
  WCHAR         FileName[1];
} FILE_ID_BOTH_DIR_INFO, *PFILE_ID_BOTH_DIR_INFO;
```
Output of NtQueryDirectoryFile
[FILE_ID_BOTH_DIR_INFO documentation](https://docs.microsoft.com/en-us/windows-hardware/drivers/ddi/content/ntifs/ns-ntifs-_file_both_dir_information)

## Overview

This header only library provides an algorithms and containers for safe creation and parsing such a collection.
It includes:
* _flat_forward_list_ref_ and _flat_forward_list_view_ a non-owning containers that allows iterating over a flat forward list in a buffer.
* _flat_forward_list_validate_ a family of functions that help to validate untrusted buffer, and prodice a ref/view to a subrange to the buffer that contains valid list.
* _flat_forward_list_ a container that owns and resizes buffer as you are adding/removing elements.
* _debug_memory_resource_ a memory resource that help with debugging
* _input_buffer_memory_resource_ a memory resource that helps in scenarios where server have to fill a passed in buffer. 

## Scenarios

function **flat_forward_list_validate** that can be use to deal with untrusted buffers. You can use it to validate if untrusted buffer contains a valid list, and to find boundary at which list gets invalid. with polymorphic allocator for debugging contained elements.

```
template<typename T,
         typename TT = flat_forward_list_traits<T>>
struct default_validate_element_fn {
    bool operator() (size_t buffer_size, char const *buffer) const noexcept {
        return TT::validate(buffer_size, buffer);
    }
};

template<typename T,
         typename TT = flat_forward_list_traits<T>,
         typename F = default_validate_element_fn<T, TT>>
constexpr inline std::pair<bool, char const *> flat_forward_list_validate(
        char const *first,
        char const *end, 
        F const &validate_element_fn = default_validate_element_fn<T, TT>{}
    ) noexcept;
```   



iterators for going over trusted collection, and a container for manipulating this list (push/pop/erase/insert/sort/merge/etc). To facilitate usage across C interface container also supports attach (a.k.a adopt ) buffer and detach buffer (you would need a custom allocator both sides would agree on).



**flat_forward_list_iterator** and flat_forward_list_const_iterator are forward iterators that can be used to enmirate over previously validated buffer.

```   
template<typename T,
         typename TT = flat_forward_list_traits<T>>
using flat_forward_list_iterator = flat_forward_list_iterator_t<T, TT>;

template<typename T,
         typename TT = flat_forward_list_traits<T>>
using flat_forward_list_const_iterator = flat_forward_list_iterator_t< std::add_const_t<T>, TT>;
```   

**flat_forward_list** a container that provides a set of helper algorithms and manages y while list changes.
Container tracks data in buffer using 3 pointers
 - **buffer_begin** is a pointer to the buffer that containes flat forward list
 - **last_element** is a pointer to the start of the last element in the buffer
 - **buffer_end** is a pointer pass the end of the buffer
 Just like with vector, user can resize buffer to a size larger than required by the current elements. This helps avoid buffer reallocations as you insert new elements or resize existing elements.
 Erasing elements does not shrink buffer. To shrink buffer user have to explicitely call shrink_to_fit.

 ```   
 template <typename T,
          typename TT = flat_forward_list_traits<T>,
          typename A = std::allocator<char>>
class flat_forward_list;
```   

**pmr_flat_forward_list** is a an aliase of flat_forward_list where allocatoe is polimorfic_allocator.

 ```   
template <typename T,
          typename TT = flat_forward_list_traits<T>>
 using pmr_flat_forward_list = flat_forward_list<T, 
                                                 TT, 
                                                 std::pmr::polymorphic_allocator<char>>;
 ```   

**debug_memory_resource** a memory resource that can be used along with polimorfic allocator for debugging contained.
 ```   
class debug_memory_resource;
 ```   

User is responsible for implementing helper class that has following methods
- tell us minimum required size element must have to be able to query element size
constexpr static size_t minimum_size() noexcept;
and addition documentation in this mode right above where primary
constexpr static size_t get_next_element_offset(char const *buffer) noexcept;
- update offset to the next element
constexpr static void set_next_element_offset(char *buffer, size_t size) noexcept;
- calculate element size from data
constexpr static size_t calculate_next_element_offset(char const *buffer) noexcept;
- validate that data fit into the buffer
constexpr static bool validate(size_t buffer_size, char const *buffer) noexcept;

By default we are looking for a partial specialization for the element type.

User have to implement following interface:
```
    namespace iffl {
        template <>
        struct flat_forward_list_traits<FLAT_FORWARD_LIST_TEST> {
            constexpr static size_t minimum_size() noexcept { <implementation> }
            constexpr static size_t get_next_element_offset(char const *buffer) noexcept { <implementation> }
            constexpr static void set_next_element_offset(char *buffer, size_t size) noexcept { <implementation> }
            constexpr static size_t calculate_next_element_offset(char const *buffer) noexcept { <implementation> }
            constexpr static bool validate(size_t buffer_size, char const *buffer) noexcept {<implementation>}
        };
    }
```

Sample implementation for FILE_FULL_EA_INFORMATION:

```
typedef struct _FILE_FULL_EA_INFORMATION {
    ULONG  NextEntryOffset;
    UCHAR  Flags;
    UCHAR  EaNameLength;
    USHORT EaValueLength;
    CHAR   EaName[1];
} FILE_FULL_EA_INFORMATION, *PFILE_FULL_EA_INFORMATION;

namespace iffl {
    template <>
    struct flat_forward_list_traits<FILE_FULL_EA_INFORMATION> {
        //
        // This is the only method required by flat_forward_list_iterator.
        //
        constexpr static size_t get_next_element_offset(char const *buffer) noexcept {
            FILE_FULL_EA_INFORMATION const &e = *reinterpret_cast<FILE_FULL_EA_INFORMATION const *>(buffer);
            return e.NextEntryOffset;
        }
        //
        // This method is requiered for validate algorithm
        //
        constexpr static size_t minimum_size() noexcept {
            return FFL_SIZE_THROUGH_FIELD(FILE_FULL_EA_INFORMATION, EaValueLength);
        }
        //
        // Helper method that calculates buffer size. Not required.
        //
        constexpr static size_t get_size(FILE_FULL_EA_INFORMATION const &e) {
            return  FFL_SIZE_THROUGH_FIELD(FILE_FULL_EA_INFORMATION, EaValueLength) +
                    e.EaNameLength +
                    e.EaValueLength;
        }
        constexpr static size_t get_size(char const *buffer) {
            return get_size(*reinterpret_cast<FILE_FULL_EA_INFORMATION const *>(buffer));
        }
        //
        // Helper method that check that element sizes are correct. Not required.
        //
        constexpr static bool validate_size(FILE_FULL_EA_INFORMATION const &e, size_t buffer_size) noexcept {
            if (e.NextEntryOffset == 0) {
                return  get_size(e) <= buffer_size;
            } else if (e.NextEntryOffset <= buffer_size) {
                return  get_size(e) <= e.NextEntryOffset;
            }
            return false;
        }
        //
        // Helper method that checks that data are valid
        //
        static bool validate_data(FILE_FULL_EA_INFORMATION const &e) noexcept {
            //
            // This is and example of a validation you might want to do.
            // Extended attribute name does not have to be 0 terminated 
            // so it is not strictly speaking nessesary here.
            //
            // char const *end = e.EaName + e.EaNameLength;
            // return end != std::find_if(e.EaName,
            //                            end, 
            //                            [](char c) -> bool {
            //                                 return c == '\0'; 
            //                            });
            return true;
        }
        //
        // This method is required for validate algorithm and container
        //
        constexpr static bool validate(size_t buffer_size, char const *buffer) noexcept {
            FILE_FULL_EA_INFORMATION const &e = *reinterpret_cast<FILE_FULL_EA_INFORMATION const *>(buffer);
            return validate_size(e, buffer_size) && validate_data(e);
        }
        //
        // This method is required by container only
        //
        constexpr static void set_next_element_offset(char *buffer, size_t size) noexcept {
            FILE_FULL_EA_INFORMATION &e = *reinterpret_cast<FILE_FULL_EA_INFORMATION *>(buffer);
            FFL_CODDING_ERROR_IF_NOT(size == 0 || size >= get_size(e));
            e.NextEntryOffset = static_cast<ULONG>(size);
        }
        //
        // This method is required by container only
        //
        constexpr static size_t calculate_next_element_offset(char const *buffer) noexcept {
            return get_size(buffer);
        }
    };
}
```

Now FILE_FULL_EA_INFORMATION is ready to be used with iffl. By default you will not explicitely spell that for FILE_FULL_EA_INFORMATION we should use iffl::flat_forward_list_traits<FILE_FULL_EA_INFORMATION>. Compiler will do the right thing using partial template specialization magic.
Here is an example where prepare_ea_and_call_handler uses container to prepare buffer with FILE_FULL_EA_INFORMATION, and calls  handle_ea.
Function handle_ea uses flat_forward_list_validate to safely process elements of untrusted buffer. 

Since FILE_FULL_EA_INFORMATION is a Plain Old Definition (POD) it does not have constructor, container methods that deal with creation of new elements allow passing
a functor that will be called once container allocates requested space for the element to initialize element data. In this sample you can see prepare_ea_and_call_handler is calling emplace_front and emplace_back and is passing in a lambda that initializes element.
If you want to zero intialize element then call container.push_back(element_size). 
If you want to initialzie element using a buffer that contains element blueprint then call .push_back(element_size, bluprint_buffer). It will initialize element by copy bluprint buffer.
Note that for last element container always resets next element offset after element contruction is done so you do not need to worry about that.

```
using ea_iffl = iffl::flat_forward_list<FILE_FULL_EA_INFORMATION>;

void handle_ea(char const *buffer, size_t buffer_lenght) {
    size_t idx{ 0 };
    char const *failed_validation{ nullptr };
    size_t invalid_element_length{ 0 };
    //
    // Use flat_forward_list_validate algorithm to safely process elements of untrusted input buffer
    //
    auto[is_valid, last_valid] =
        iffl::flat_forward_list_validate<FILE_FULL_EA_INFORMATION>(
            buffer,
            buffer + buffer_lenght,
            [buffer, &idx, &failed_validation, &invalid_element_length] (size_t buffer_size,
                                                                         char const *element_buffer) -> bool {
                bool is_valid = iffl::flat_forward_list_traits<FILE_FULL_EA_INFORMATION>::validate(buffer_size, element_buffer);
                if (is_valid) {
                    //
                    // Add here code that handles element
                    //
                    FILE_FULL_EA_INFORMATION const &e = 
                        *reinterpret_cast<FILE_FULL_EA_INFORMATION const *>(element_buffer);
                    printf("FILE_FULL_EA_INFORMATION[%zi] @ = 0x%p, buffer offset %zi\n",
                           idx,
                           &e,
                           element_buffer - buffer);
                } else {
                    invalid_element_length = buffer_size;
                    failed_validation = buffer;
                }
                return is_valid;
            });
}

void prepare_ea_and_call_handler() {
    //
    // Use container to fill buffer
    //
    ea_iffl eas;

    char const ea_name0[] = "TEST_EA_0";

    eas.emplace_front(FFL_SIZE_THROUGH_FIELD(FILE_FULL_EA_INFORMATION, EaValueLength) 
                     + sizeof(ea_name0)-1, 
                     [](char *buffer,
                        size_t new_element_size) {
                        FILE_FULL_EA_INFORMATION &e = *reinterpret_cast<FILE_FULL_EA_INFORMATION *>(buffer);
                        e.Flags = 0;
                        e.EaNameLength = sizeof(ea_name0)-1;
                        e.EaValueLength = 0;
                        iffl::copy_data(e.EaName,
                                        ea_name0,
                                        sizeof(ea_name0)-1);
                      });

    char const ea_name1[] = "TEST_EA_1";
    char const ea_data1[] = {1,2,3};

    eas.emplace_back(FFL_SIZE_THROUGH_FIELD(FILE_FULL_EA_INFORMATION, EaValueLength) 
                     + sizeof(ea_name1)-1
                     + sizeof(ea_data1),
                     [](char *buffer,
                        size_t new_element_size) {
                        FILE_FULL_EA_INFORMATION &e = *reinterpret_cast<FILE_FULL_EA_INFORMATION *>(buffer);
                        e.Flags = 1;
                        e.EaNameLength = sizeof(ea_name1)-1;
                        e.EaValueLength = sizeof(ea_data1);
                        iffl::copy_data(e.EaName,
                                        ea_name1,
                                        sizeof(ea_name1)-1);
                        iffl::copy_data(e.EaName + sizeof(ea_name1)-1,
                                        ea_data1,
                                        sizeof(ea_data1));
                      });

    handle_ea(eas.data(), eas.used_capacity());
 }

```
If user prefers separate buffer validation and element processing then she can use default iffl::flat_forward_list_validate validation functor, and ince validation passes use iterators to walk the elements.
Here is a samole implementation:

```
using ea_iterator = iffl::flat_forward_list_iterator<FILE_FULL_EA_INFORMATION>;
using ea_const_iterator = iffl::flat_forward_list_const_iterator<FILE_FULL_EA_INFORMATION>;

void handle_ea(char const *buffer, size_t buffer_lenght) {
    size_t idx{ 0 };
    char const *failed_validation{ nullptr };
    size_t invalid_element_length{ 0 };
    //
    // Use flat_forward_list_validate algorithm to safely process elements of untrusted input buffer
    //
    auto[is_valid, last_valid] = iffl::flat_forward_list_validate<FILE_FULL_EA_INFORMATION>( buffer, buffer + buffer_lenght);
    //
    // If buffer contains a valid non-empty list then use iterators to walk elements
    //
    if (is_valid && last_valid) {
        std::for_each(
            ea_const_iterator{buffer}, 
            ea_const_iterator{}, // this sentinel iterator plays a role of end
             [](FILE_FULL_EA_INFORMATION const &e) {
                    printf("FILE_FULL_EA_INFORMATION @ = 0x%p\n", &e);
             }
    }
}
```

You can find sample implementation for another type FLAT_FORWARD_LIST_TEST at [test\iffl_test_cases.cpp](https://github.com/vladp72/iffl/blob/master/test/iffl_test_cases.cpp)
Addition documetation is in [iffl.h](https://github.com/vladp72/iffl/blob/master/include/iffl.h) right above declaration of primary template for flat_forward_list_traits


If picking traits using partial specialization is not feasible then traits can be passed as

an explicit template parameter. For example:
```
   using ffl_iterator = iffl::flat_forward_list_iterator<FILE_FULL_EA_INFORMATION, my_alternative_ea_traits>;
```
