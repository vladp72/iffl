# Intrusive Flat Forward List for Trivialy Movable types

This header only library that implements intrusive flat forward list (iffl).

* __Intrusive__ - Pointer to the next element must be a part of the container's elements data type;
* __Flat__ - All elements of container are laid out in a continues block of memory, rather than each element being allocated separately.
* __Forward__ __List__ - Elements have information on how to locate start of the next element. There is no way to get to the previous element other than linear scan from the beginning of the list. Because this is a __Flat__ container instead of pointers to the next element, it uses offset to the next element from the beginning of the current element.
*
[Trivialy Movable](https://quuxplusone.github.io/blog/2018/09/28/trivially-relocatable-vs-destructive-movable/)

[Implementation](https://github.com/vladp72/iffl/tree/master/include)

[Doxygen documentation](https://vladp72.github.io/iffl/html/annotated.html)

[Tests and samples](https://github.com/vladp72/iffl/tree/master/test)

[This project is compiled and published using CMake](https://github.com/vladp72/iffl/blob/master/CMakeLists.txt)

Tests and samples are build and verified on 
* Windows 10 using Visual Studio C++ compiler 
* Ubuntu Linux using both clang and gcc.

## Motivation

Often times when dealing with OS or just C interface we need to pass in or parse a a buffer that contains a linked list of variable size structures. 

These POD typess have following general structure:
```
                      ------------------------------------------------------------
                      |                                                          |
                      |                                                          V
 | <fields> | offset to next element | <offsets of data> | [data] | [padding] || [next element] ...
 |                        header                         | [data] | [padding] || [next element] ...
 ```
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
```
[FILE_NOTIFY_EXTENDED_INFORMATION documentation](https://docs.microsoft.com/en-us/windows/desktop/api/winnt/ns-winnt-_file_notify_extended_information)

Output for the following information classes from [FILE_INFO_BY_HANDLE_CLASS](https://msdn.microsoft.com/en-us/8f02e824-ca41-48c1-a5e8-5b12d81886b5)
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
For example on success following call
```
GetFileInformationByHandleEx(file_handle, FileIdBothDirectoryInfo, buffer, buffer_size);
```
will fill output buffer with structures
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
You would get the same output when you call NtQueryDirectoryFile with [FileIdBothDirectoryInfo](https://docs.microsoft.com/en-us/windows-hardware/drivers/ddi/content/ntifs/ns-ntifs-_file_both_dir_information).

Other class of types that fall into similar category are types that do not have next element offset, but it can be calculated from the element size. Offset of the next element is size of this element data, plus optional padding to keep ext element properly aligned
```
                     -----------------------------------
                     |                                 |
    (next element offset = sizeof(this element)        |
                     |   + padding)                    |
                     |                                 V
| <fields> | <offsets of data> | [data] | [padding] || [next element] ...
|       header                 | [data] | [padding] || [next element] ...
```

Exanmples:

 CLUSPROP_SYNTAX
* [property list](https://docs.microsoft.com/en-us/previous-versions/windows/desktop/mscs/property-lists)
* [data structures](https://docs.microsoft.com/en-us/previous-versions/windows/desktop/mscs/data-structures)
* [cluster property syntax](https://docs.microsoft.com/en-us/windows/desktop/api/clusapi/ns-clusapi-clusprop_syntax)

```
 typedef union CLUSPROP_SYNTAX {
   DWORD  dw;
   struct {
       WORD wFormat;
       WORD wType;
   } DUMMYSTRUCTNAME;
 } CLUSPROP_SYNTAX;
```
 [CLUSPROP_VALUE](https://docs.microsoft.com/en-us/windows/desktop/api/clusapi/ns-clusapi-clusprop_value)
```
 typedef struct CLUSPROP_VALUE {
     CLUSPROP_SYNTAX Syntax;
     DWORD           cbLength;
 } CLUSPROP_VALUE;
```

## Overview

This header only library provides an algorithms and containers for safe creation and parsing such a collection.
It includes:
* [**flat_forward_list_ref**](https://vladp72.github.io/iffl/html/classiffl_1_1flat__forward__list__ref.html) and [**flat_forward_list_view**](https://vladp72.github.io/iffl/html/namespaceiffl.html) a non-owning containers that allows iterating over a flat forward list in a buffer.
* [**flat_forward_list_validate**](https://vladp72.github.io/iffl/html/namespaceiffl.html) a family of functions that help to validate untrusted buffer, and returns a ref/view to a subrange to the buffer that contains valid list.
* [**flat_forward_list**](https://vladp72.github.io/iffl/html/classiffl_1_1flat__forward__list.html) a container that owns and resizes buffer as you are adding/removing elements.
* [**debug_memory_resource**](https://vladp72.github.io/iffl/html/classiffl_1_1debug__memory__resource.html) a memory resource that help with debugging
* [**input_buffer_memory_resource**](https://vladp72.github.io/iffl/html/classiffl_1_1input__buffer__memory__resource.html) a memory resource that helps in scenarios where server have to fill a passed in buffer. 

## Boilerplate

Types that are usually used with this container are POD types defined elsewhere. We cannot extent these types. We do need several methods to be able to traverse and modify elements in the container:

* Mandatory methods have to be implemented in all types and all scenarios
  * Returns minimum required buffer size. For example minimum required buffer size for FILE_FULL_EA_INFORMATION is offset of field EaValueLength, plus size of EaValueLength. If buffer is smaller then we cannot even tell size used by the data of the element.
   * ```static size_t minimum_size() noexcept;```
  * Returns element size calculated from size of data contained by the element. For example for FILE_FULL_EA_INFORMATION it would be size of minimum header plus value in EaNameLength and EaValueLength.
   * ```static size_t get_size(<type> const &e) noexcept;```
* Optional method that validates element contains correct data. For instance it can check that size of the fields fit in the element buffer. For example for FILE_FULL_EA_INFORMATION it can check that element size would not be larger than offset to the next element.
  * ```static bool validate(size_t buffer_size, <type> const &e) noexcept;```
* Types that have offset to the next element must implement method that return that value. Container uses this method to traverse list, but if type does not have that field then you should not implement this method. Without this method container will use get_size as an offset to the next element.
 * ```static size_t get_next_offset(<type> const &e) noexcept;```
* For scenario where we need to modify list, and the type supports next element offset field, you need to implement a method that allows setting updating this field. If type does not support next element offset then do not implement this method. If this method is not present then container assumes next offset field is not part of the type and will no-op updating it.
  * ```static void set_next_element_offset(<type> &buffer, size_t size) noexcept;```
* Optionally you can add a static variable that tells alignment required for an element header. Container will add padding to the element to keep next element properly aligned. If this member is absent then container assumes that elements can be 1 byte aligned and would not add any padding. Reading unaligned data might cause an exception or sig-fault on some platforms, unless you explicitly annotate your pointers as unaligned.
  * ```constexpr static size_t const alignment{ <alignment> }```

User can pass a type that implements these method as an explicit template parameter or she can specialize iffl::flat_forward_list_traits for the type. By default all containers and algorithms will look for this specialization.

List of methods that can be implemented:
```
    namespace iffl {
        template <>
        struct flat_forward_list_traits<FLAT_FORWARD_LIST_TEST> {
            constexpr static size_t const alignment{ alignof(FLAT_FORWARD_LIST_TEST) };
            constexpr static size_t minimum_size() noexcept { <implementation> }
            constexpr static size_t get_next_offset(FLAT_FORWARD_LIST_TEST const &e) noexcept { <implementation> }
            constexpr static void set_next_offset(FLAT_FORWARD_LIST_TEST &e, size_t size) noexcept { <implementation> }
            constexpr static size_t get_size(FLAT_FORWARD_LIST_TEST const &e) noexcept { <implementation> }
            constexpr static bool validate(size_t buffer_size, FLAT_FORWARD_LIST_TEST const &e) noexcept {<implementation>}
        };
    }
```

By default you will not explicitly spell that for FILE_FULL_EA_INFORMATION we should use iffl::flat_forward_list_traits<FILE_FULL_EA_INFORMATION>. Compiler will do the right thing using partial template specialization magic.

```
template <typename T,
          typename TT = flat_forward_list_traits<T>,
          typename A = std::allocator<T>>
class flat_forward_list final;
```
You can simply declare container as ```iffl::flat_forward_list final<FILE_FULL_EA_INFORMATION>```. If you want to point container to a different set of traits then you can pass them in explicitly ```iffl::flat_forward_list final<FILE_FULL_EA_INFORMATION, iffl::flat_forward_list_traits<FLAT_FORWARD_LIST_TEST>>```

Let's take a look at a complete [implementation](https://github.com/vladp72/iffl/blob/master/test/iffl_ea.h) for FILE_FULL_EA_INFORMATION:

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
    
        constexpr static size_t const alignment{ alignof(FILE_FULL_EA_INFORMATION) };
        
        static size_t get_next_offset(FILE_FULL_EA_INFORMATION const &e) noexcept {
            return e.NextEntryOffset;
        }
        
        static size_t minimum_size() noexcept {
            return FFL_SIZE_THROUGH_FIELD(FILE_FULL_EA_INFORMATION, EaValueLength);
        }
        
        static size_t get_size(FILE_FULL_EA_INFORMATION const &e) {
            return  FFL_SIZE_THROUGH_FIELD(FILE_FULL_EA_INFORMATION, EaValueLength) +
                    e.EaNameLength +
                    e.EaValueLength;
        }
        
        static bool validate_size(FILE_FULL_EA_INFORMATION const &e, size_t buffer_size) noexcept {
        if (e.NextEntryOffset == 0) {
                return  get_size(e) <= buffer_size;
            } else if (e.NextEntryOffset <= buffer_size) {
                return  get_size(e) <= e.NextEntryOffset;
            }
            return false;
        }

        static bool validate_data(FILE_FULL_EA_INFORMATION const &e) noexcept {
            return true;
        }
        
        static bool validate(size_t buffer_size, FILE_FULL_EA_INFORMATION const &e) noexcept {
            return validate_size(e, buffer_size) && validate_data(e);
        }
        
        static void set_next_offset(FILE_FULL_EA_INFORMATION &e, size_t size) noexcept {
            FFL_CODDING_ERROR_IF_NOT(size == 0 || size >= get_size(e));
            e.NextEntryOffset = static_cast<ULONG>(size);
        }
    };
}

```

Now FILE_FULL_EA_INFORMATION is ready to be used with iffl containers. 

```
using ea_iffl_ref = iffl::flat_forward_list_ref<FILE_FULL_EA_INFORMATION>;
using ea_iffl_view = iffl::flat_forward_list_view<FILE_FULL_EA_INFORMATION>;
using ea_iffl = iffl::flat_forward_list<FILE_FULL_EA_INFORMATION>;
using pmr_ea_iffl = iffl::pmr_flat_forward_list<FILE_FULL_EA_INFORMATION>;
```

Class pod_array_list_entry is an example of type that does not have offset to the next element. For that type we do not implement get_next_offset and set_next offset. A complete implementation is in [test/iffl_list_array.h](https://github.com/vladp72/iffl/blob/master/test/iffl_list_array.h).

```
template <typename T>
struct pod_array_list_entry {
    unsigned short length;
    T arr[1];
    
    using type = T;
};

template <typename T>
struct pod_array_list_entry_traits {

    using header_type = pod_array_list_entry<T>;

    constexpr static size_t const alignment{ alignof(header_type) };

    constexpr static size_t minimum_size() noexcept {
        return FFL_SIZE_THROUGH_FIELD(header_type, length);
    }

    constexpr static size_t get_size(header_type const &e) {
        return FFL_FIELD_OFFSET(header_type, arr) + e.length * sizeof(T);
    }

    constexpr static bool validate(size_t buffer_size, header_type const &e) noexcept {
        return  get_size(e) <= buffer_size;
    }
};
```
You can use tis type to declare variable array of long long

```
using long_long_array_list_entry = pod_array_list_entry<long long>;

namespace iffl {
    template <>
    struct flat_forward_list_traits<long_long_array_list_entry>
        : public pod_array_list_entry_traits<long_long_array_list_entry::type> {
    };
}

using long_long_array_list = iffl::pmr_flat_forward_list<long_long_array_list_entry>;
```
Type long_long_array_list is a list of variable length arrays of long long. 

Or a variable array of char

```
using char_array_list_entry = pod_array_list_entry<char>;

namespace iffl {
    template <>
    struct flat_forward_list_traits<char_array_list_entry>
        : public pod_array_list_entry_traits<char_array_list_entry::type> {
    };
}

using char_array_list = iffl::pmr_flat_forward_list<char_array_list_entry>;
```
Type char_array_list is a list of variable length arrays of char. 

## Scenarios

### Adding elements to flat forward list

Since FILE_FULL_EA_INFORMATION is a Plain Old Definition (POD) it does not have constructor, container methods that deal with creation of new elements allow passing a functor that will be called once container allocates requested space for the element to initialize element data. In this sample you can see prepare_ea_and_call_handler is calling emplace_front and emplace_back and is passing in a lambda that initializes element.
If you want to zero initialize element then call container.push_back(element_size). 
If you want to initialize an element using a buffer that contains element blueprint then call .push_back(element_size, bluprint_buffer). It will initialize element by copy blueprint buffer.
Note that for last element container always resets next element offset after element construction is done so you do not need to worry about that.
A complete sample is located in [test/iffl_ea_usecase.cpp](https://github.com/vladp72/iffl/blob/master/test/iffl_ea_usecase.cpp).

```
ea_iffl eas;

char const ea_name0[] = "TEST_EA_0";

// Add EA to the front of the list

eas.emplace_front(FFL_SIZE_THROUGH_FIELD(FILE_FULL_EA_INFORMATION, EaValueLength) + sizeof(ea_name0)-sizeof(char), 
                 [](FILE_FULL_EA_INFORMATION &e, size_t new_element_size) noexcept {
                    e.Flags = 0;
                    e.EaNameLength = sizeof(ea_name0)-sizeof(char);
                    e.EaValueLength = 0;
                    iffl::copy_data(e.EaName, ea_name0, sizeof(ea_name0)-sizeof(char));
                  });

char const ea_name1[] = "TEST_EA_1";
char const ea_data1[] = {1,2,3};

// Add EA to the end of the list

eas.emplace_back(FFL_SIZE_THROUGH_FIELD(FILE_FULL_EA_INFORMATION, EaValueLength) + sizeof(ea_name1)-sizeof(char) + sizeof(ea_data1),
                 [](FILE_FULL_EA_INFORMATION &e, size_t new_element_size) noexcept {
                    e.Flags = 1;
                    e.EaNameLength = sizeof(ea_name1)-sizeof(char);
                    e.EaValueLength = sizeof(ea_data1);
                    iffl::copy_data(e.EaName, ea_name1, sizeof(ea_name1)-sizeof(char));
                    iffl::copy_data(e.EaName + sizeof(ea_name1)-sizeof(char), ea_data1, sizeof(ea_data1));
                  });

char const ea_name2[] = "TEST_EA_2";
char const ea_data2[] = { 1,2,3,4,5,6,7,8,9,0xa,0xb,0xc,0xd,0xf };

// Add EA to the front of the list

eas.emplace_front(FFL_SIZE_THROUGH_FIELD(FILE_FULL_EA_INFORMATION, EaValueLength) + sizeof(ea_name2)-sizeof(char) + sizeof(ea_data2),
                 [](FILE_FULL_EA_INFORMATION &e, size_t new_element_size) noexcept {
                    e.Flags = 2;
                    e.EaNameLength = sizeof(ea_name2)-sizeof(char);
                    e.EaValueLength = sizeof(ea_data2);
                    iffl::copy_data(e.EaName, ea_name2, sizeof(ea_name2)-sizeof(char));
                    iffl::copy_data(e.EaName + sizeof(ea_name2)-sizeof(char), ea_data2, sizeof(ea_data2));
                  });

```
will produce following list of extended attributes
```
FILE_FULL_EA_INFORMATION[0] @ = 0x0x607000000100, buffer offset 0
FILE_FULL_EA_INFORMATION[0].NextEntryOffset = 32
FILE_FULL_EA_INFORMATION[0].Flags = 2
FILE_FULL_EA_INFORMATION[0].EaNameLength = 9 "TEST_EA_2"
FILE_FULL_EA_INFORMATION[0].EaValueLength = 14 123456789abcdf

FILE_FULL_EA_INFORMATION[1] @ = 0x0x607000000120, buffer offset 32
FILE_FULL_EA_INFORMATION[1].NextEntryOffset = 20
FILE_FULL_EA_INFORMATION[1].Flags = 0
FILE_FULL_EA_INFORMATION[1].EaNameLength = 9 "TEST_EA_0"
FILE_FULL_EA_INFORMATION[1].EaValueLength = 0

FILE_FULL_EA_INFORMATION[2] @ = 0x0x607000000134, buffer offset 52
FILE_FULL_EA_INFORMATION[2].NextEntryOffset = 0
FILE_FULL_EA_INFORMATION[2].Flags = 1
FILE_FULL_EA_INFORMATION[2].EaNameLength = 9 "TEST_EA_1"
FILE_FULL_EA_INFORMATION[2].EaValueLength = 3 123
```

### Adding elements to flat forward list over input buffer not owned by the calee.

A complete sample is located in [test/iffl_c_api_usecase2.cpp](https://github.com/vladp72/iffl/blob/master/test/iffl_c_api_usecase2.cpp).

```
unsigned short idx{ 0 };

bool server_api_call(char *buffer, size_t *buffer_size) noexcept {
    bool result{ false };
    //
    // If user did not pass a valid buffer then bail out
    //
    if (!buffer || !buffer_size) {
        return result;
    }
    //
    // when container allocates memory this memory resource 
    // returns pointer to the input buffer
    //
    iffl::input_buffer_memory_resource input_buffer{reinterpret_cast<void *>(buffer), *buffer_size};
    //
    // Create container that will use above memory resource
    //
    char_array_list data{ &input_buffer };
    //
    // Resizing container to the input buffer size.
    // Container will ask memory resource for allocation
    // and resource will return pointer to the input buffer.
    // Rest of algorithm will avoid making any calls that can
    // trigger memory reallocation. If we trigger that then 
    // resource will throw std::bad_alloc because it does not have
    // any more memory it can allocate.
    //
    data.resize_buffer(*buffer_size);
    //
    // idx is a global variable that remembers where server stopped last time.
    // Keep adding elements for as long as buffer has capacity
    //
    for (;; ++idx) {
        //
        // Estimate size of the element we are inserting
        //
        size_t element_size{ char_array_list::traits::minimum_size() + idx * sizeof(char_array_list::value_type::type) };
        //
        // try_emplace_back will succeed if we can add element
        // without triggering reallocation, otherwise it will return 
        // false.
        //
        if (!data.try_emplace_back(element_size,
                                    [] (char_array_list_entry &e, size_t element_size) noexcept {
                                        e.length = idx;
                                        std::fill(e.arr, e.arr + e.length, static_cast<char>(idx)+1);
                                    })) {
            //
            // Before we return back to the caller make sure we are not leaking any
            // values that would help caller to attack server.
            //
            data.fill_padding();
            //
            // Tell client up to what point buffer contains valid data
            //
            *buffer_size = data.used_capacity();
            break;
        }
        //
        // If we inserted at least one element then return true
        //
        result = true;
    }
    return result;
}

```

### Validate input buffer

If you are receiving a untrusted buffer that is expected to contain a flat forward list you can validate it using **flat_forward_list_validate** family of function.

```
template<typename T,
         typename TT = flat_forward_list_traits<T>,
         typename F = default_validate_element_fn<T, TT>>
constexpr inline std::pair<bool, flat_forward_list_ref<T, TT>> 
  flat_forward_list_validate(char const *first,
                             char const *end, 
                             F const &validate_element_fn = default_validate_element_fn<T, TT>{}) noexcept;

template<typename T,
         typename TT = flat_forward_list_traits<T>,
         typename F = default_validate_element_fn<T, TT>>
constexpr inline std::pair<bool, flat_forward_list_ref<T, TT>> 
  flat_forward_list_validate(char *first,
                             char *end,
                             F const &validate_element_fn = default_validate_element_fn<T, TT>{}) noexcept;
```

Function returns a pair of boolean and a reference or view. If passed parameter is a non-const buffer then result is a reference. If input buffer is const then result is a view (const reference). Boolean indicates if buffer contains a valid list. Even if list is broken, and function returns false, view will point to the subset of the buffer that contains a valid list. For instance if a pointer to the next element refers outside of the buffer range, then validate would abort, and return false, but returned reference/view will describe element from the beginning to the last valid element. 

You can choose to be strict and reject invalid buffer. A complete sample is in [test/iffl_test_cases.cpp](https://github.com/vladp72/iffl/blob/master/test/iffl_test_cases.cpp). 

```
auto [is_valid, view] = iffl::flat_forward_list_validate<FILE_FULL_EA_INFORMATION>(std::begin(buffer), std::end(buffer));
if (is_valid) {
  std::for_each(view.begin(), view.begin(), 
                [](FLAT_FORWARD_LIST_TEST &e) noexcept {
                    print_element(e);
                });
    return true;
} else {
    return false;
}
```
Or to handle as much of the buffer as you can.  
```
auto [is_valid, view] = iffl::flat_forward_list_validate<FILE_FULL_EA_INFORMATION>(std::begin(buffer), std::end(buffer));
std::for_each(view.begin(), view.begin(), 
              [](FLAT_FORWARD_LIST_TEST &e) noexcept {
                  print_element(e);
              });
return is_valid;
```
In the samples above we traverse buffer two times. It is possible to traverse and handle elements in a single loop by passing your own functor in place of the default functor that calls trait's validate method. A complete sample is in [test/iffl_ea_usecase.cpp](https://github.com/vladp72/iffl/blob/master/test/iffl_ea_usecase.cpp).

```
FILE_FULL_EA_INFORMATION const *failed_validation{nullptr};
size_t invalid_element_length{0};

auto[is_valid, buffer_view] =
    iffl::flat_forward_list_validate<FILE_FULL_EA_INFORMATION>(
        buffer,
        buffer + buffer_lenght,
        [&failed_validation, &invalid_element_length] (size_t buffer_size,
                                                       FILE_FULL_EA_INFORMATION const &e) -> bool {
            //
            // validate element
            //
            bool const is_valid{ iffl::flat_forward_list_traits<FILE_FULL_EA_INFORMATION>::validate(buffer_size, e) };
            //
            // if element is valid then process element
            //
            if (is_valid) {
                handle_validated_element(e);
            } else {
                //
                // Save additional information about element that failed validation
                //
                invalid_element_length = buffer_size;
                failed_validation = &e;
            }
            //
            // validation loop is aborted if element is not valid
            //
            return is_valid;
        });
```
You can use flat_forward_list_validate as if it is a find-first algorithm and abort validation as soon as you've found an element you are looking for.

### Passing buffer ownership across C interface avoiding extra copy.

If server and client can agree on how to allocate an deallocate buffer then you can create buffer using flat_forward_list, detach ownership of buffer from container, pass pointers to the buffer over a C interface, callee can take ownership of the buffer, and deallocate once processing is done.

A complete sample is located in [test/iffl_c_api_usecase1.cpp](https://github.com/vladp72/iffl/blob/master/test/iffl_c_api_usecase1.cpp).

In this example we will use a global variable providing a common memory resource for client and server.
```
iffl::debug_memory_resource global_memory_resource;
```
Server creates list, and returns pointers to the buffer containing list.
```
bool server_api_call(char **buffer, size_t *buffer_size) noexcept {
    if (!buffer || !buffer_size) {
        return false;
    }

    try {
    
        char_array_list data{ &global_memory_resource };
    
        unsigned short array_size{ 10 };
        size_t element_buffer_size{ char_array_list_entry::byte_size_to_array_size(array_size) };
        char pattern{ 1 };

        data.emplace_back(element_buffer_size,
                          [array_size, pattern] (char_array_list_entry &e,
                                                 size_t element_size) noexcept {
                               e.length = array_size;
                               std::fill(e.arr, e.arr + e.length, pattern);
                          });

        array_size = 5;
        element_buffer_size = char_array_list_entry::byte_size_to_array_size(array_size);
        pattern = 2;

        data.emplace_back(element_buffer_size,
                          [array_size, pattern](char_array_list_entry &e,
                                                size_t element_size) noexcept {
                              e.length = array_size;
                              std::fill(e.arr, e.arr + e.length, pattern);
                          });

        array_size = 20;
        element_buffer_size = char_array_list_entry::byte_size_to_array_size(array_size);
        pattern = 3;

        data.emplace_back(element_buffer_size,
                          [array_size, pattern](char_array_list_entry &e,
                                                size_t element_size) noexcept {
                              e.length = array_size;
                              std::fill(e.arr, e.arr + e.length, pattern);
                          });

        array_size = 0;
        element_buffer_size = char_array_list_entry::byte_size_to_array_size(array_size);
        pattern = 4;

        data.emplace_back(element_buffer_size,
                          [array_size, pattern](char_array_list_entry &e,
                                                size_t element_size) noexcept {
                              e.length = array_size;
                              std::fill(e.arr, e.arr + e.length, pattern);
                          });

        array_size = 11;
        element_buffer_size = char_array_list_entry::byte_size_to_array_size(array_size);
        pattern = 5;

        data.emplace_back(element_buffer_size,
                          [array_size, pattern](char_array_list_entry &e,
                                                size_t element_size) noexcept {
                              e.length = array_size;
                              std::fill(e.arr, e.arr + e.length, pattern);
                          });
        //
        // Make sure we are not leaking in the padding any information that can
        // be helpful to attack server.
        //
        // Without that call padding between entries is field with garbage. In this 
        // case 0xfe.
        //
        // cdb/windbg>db 0x00000256`ebd78fa0 L47
        //
        // 00000256`ebd78fa0  0a 00 00 00 01 01 01 01 - 01 01 01 01 01 01 fe fe  ................
        // 00000256`ebd78fb0  05 00 00 00 02 02 02 02 - 02 fe fe fe 14 00 00 00  ................
        // 00000256`ebd78fc0  03 03 03 03 03 03 03 03 - 03 03 03 03 03 03 03 03  ................
        // 00000256`ebd78fd0  03 03 03 03 00 00 00 00 - 0b 00 00 00 05 05 05 05  ................
        // 00000256`ebd78fe0  05 05 05 05 05 05 05    -                          .......
        //
        // After the call padding is filled with zeros
        //
        // cdb/windbg>db 0x00000256`ebd78fa0 L47
        //
        // 00000254`fd668fa0  0a 00 00 00 01 01 01 01 - 01 01 01 01 01 01 00 00  ................
        // 00000254`fd668fb0  05 00 00 00 02 02 02 02 - 02 00 00 00 14 00 00 00  ................
        // 00000254`fd668fc0  03 03 03 03 03 03 03 03 - 03 03 03 03 03 03 03 03  ................
        // 00000254`fd668fd0  03 03 03 03 00 00 00 00 - 0b 00 00 00 05 05 05 05  ................
        // 00000254`fd668fe0  05 05 05 05 05 05 05    -                          .......
        //
        data.fill_padding();

        iffl::buffer_ref detached_buffer{ data.detach() };
        *buffer = detached_buffer.begin;
        *buffer_size = detached_buffer.size();

    } catch (...) {
        return false;
    }
    return true;
}
```
Client would take ownership of the buffer, validate received buffer, and process elements
```
void call_server() {
    char *buffer{ nullptr };
    size_t buffer_size{ 0 };

    if (server_api_call(&buffer, &buffer_size)) {
        //
        // If server call succeeded the 
        // take ownership of the buffer
        //
        char_array_list data{ iffl::attach_buffer{},
                              buffer, 
                              buffer_size,
                              &global_memory_resource };
        process_data(data);
    }
}
```

### Handle buffer that containes not propertly alligned elements.

In some cases you need to handle flat forward list that contains elements that are not properly aligned. Ideally you always should prefer fixing code that produces broken list, but if it is not an option and you have to workaround the issue then you have to access your elements using pointers annotated as unaligned. On Visual studio you can use [*unaligned*](https://docs.microsoft.com/en-us/cpp/cpp/unaligned?view=vs-2017). On GCC and CLANG you can use  [*attribute ((packed))*](https://gcc.gnu.org/onlinedocs/gcc-4.0.2/gcc/Type-Attributes.html). If you ignore problem then on some platforms CPU might raise a fault and you program will crash when trying to load a value using unaligned pointer. When pointer is marked unaligned, compiler will generate code to load data in smaller pieces. This will cause performance hit, but it would avoid faults. Some platforms allow annotating executable such that OS will handle CPU faults for unaligned access and will patch it up. That would have even higher performance cost. While this might be the only option if you do not have access to the source code, you always should prefer fixing code. 

Complete Sample is [intest/iffl_unaligned.cpp](https://github.com/vladp72/iffl/blob/master/test/iffl_unaligned.cpp).

This sample also demonstates how to explicitely pass type traits to the container, instead of relying on a default, which picks traits using template class specialization. 

```
template <typename T>
struct flat_forward_list_traits_unaligned {
    using  header_type = pod_array_list_entry<T>;
    //
    // All memory access will be done using pointer annotated
    // that data might be unaligned. On platforms where CPU raises 
    // signal when accessing unaligned data, compiler is expected to 
    // generate code that reads data in multiple round trips using
    // smaller types that will be properly aligned. For instance
    // it can load it as a series of bytes.
    //
    using header_type_ptr = FFL_UNALIGNED header_type *;
    using header_type_const_ptr = FFL_UNALIGNED header_type const *;

    constexpr static size_t const alignment{ 1 };
    //
    // This method is required for validate algorithm
    //
    constexpr static size_t minimum_size() noexcept {
        return FFL_SIZE_THROUGH_FIELD(header_type, length);
    }
    //
    // Required by container validate algorithm when type
    // does not support get_next_offset
    //
    constexpr static size_t get_size(header_type const &e) {
        //
        // Element might be pointing to unaligned data
        // make sure we access it using pointer marked as 
        // unaligned
        //
        header_type_const_ptr unaligned_e{ &e };
        size_t const size = FFL_FIELD_OFFSET(header_type, arr) + unaligned_e->length * sizeof(T);
        return size;
    }
    //
    // This method is required for validate algorithm
    //
    constexpr static bool validate(size_t buffer_size, header_type const &e) noexcept {
        return  get_size(e) <= buffer_size;
    }
};

 using unaligned_char_array_list_entry_ptr = FFL_UNALIGNED char_array_list_entry *;
 using unaligned_char_array_list_entry_const_ptr = FFL_UNALIGNED char_array_list_entry const *;

using unaligned_char_array_list_ref = iffl::flat_forward_list_ref<char_array_list_entry, 
                                                                  flat_forward_list_traits_unaligned<char_array_list_entry::type>>;
using unaligned_char_array_list_view = iffl::flat_forward_list_view<char_array_list_entry, 
                                                                    flat_forward_list_traits_unaligned<char_array_list_entry::type>>;
using unaligned_char_array_list = iffl::pmr_flat_forward_list<char_array_list_entry, 
                                                              flat_forward_list_traits_unaligned<char_array_list_entry::type>>;


void process_unaligned_view(unaligned_char_array_list_view const &data) {
    unaligned_char_array_list_view::const_iterator end{ data.cend() };
    for (auto cur{ data.cbegin() }; cur != end; ++cur) {
        //
        // Element might be pointing to unaligned data
        // make sure we access it using pointer marked as 
        // unaligned
        //
        unaligned_char_array_list_entry_const_ptr unaligned_e_ptr{ &*cur };
        if (iffl::roundup_ptr_to_alignment<char_array_list_entry>(unaligned_e_ptr) != unaligned_e_ptr) {
            std::printf("Found char_array_list_entry length %03hu at unaligned address %p\n",
                        unaligned_e_ptr->length,
                        reinterpret_cast<void const*>(unaligned_e_ptr));
        }
    }
}
```

### Other Use Cases.

You can use a list of flat forward list as a queue where producer creates batches of buffers organized in flat forward lists, and once buffer is full it would move it to the list for processing by a consumer.
```
std::list<char_array_list>
```
You can subdivide a large flat forward list to sublists tracked using views, and pass processing of each view to a separate thread. Sample application for this use case is in [test/iffl_views.cpp](https://github.com/vladp72/iffl/blob/master/test/iffl_views.cpp).

You use an entry of char_array_list as a frame that contains a serialized message. Container char_array_list can accumulate certain number of frames, and once you are ready, you can send entire batch over a socket.

On receiving side you can read data into a buffer, use _flat_forward_list_validate_ to process as many fully received frames as we can find in the buffer. Remove processed data, by shifting tail to the head, and receive more data to the tails of the buffer.

