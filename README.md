# iffl

C++ container for Intrusive Flat Forward List

Intrusive Flat Forward List for POD elements
Implements intrusive flat forward list (iffl).
https://github.com/vladp72/iffl/tree/master/include

Often times when dealing with OS or just C interface we need to pass in or parse a linked 
list of variable size structs organized into a linked list in a buffer. This header only 
library provides an algorithm for safe parsing such a collection, iterators for going over 
trusted collection, and a container for manipulating this list (push/pop/erase/insert/sort/merge/etc). 
To facilitate usage across C interface container also supports attach (a.k.a adopt ) buffer 
and detach buffer (you would need a custom allocator both sides would agree on).

This container is designed to contain element of following general structure:
that can be used to enumerate over previously validated buffer
                      ------------------------------------------------------------
                      |                                                          |
                      |                                                          V
 | <fields> | offset to next element | <offsets of data> | [data] | [padding] || [next element] ...
 |                        header                         | [data] | [padding] || [next element] ...

Examples are from Windows, but I am sure there is plenty of samples in Unix:

FILE_FULL_EA_INFORMATION
https:docs.microsoft.com/en-us/windows-hardware/drivers/ddi/content/wdm/ns-wdm-_file_full_ea_information

FILE_NOTIFY_EXTENDED_INFORMATION
https:docs.microsoft.com/en-us/windows/desktop/api/winnt/ns-winnt-_file_notify_extended_information

WIN32_FIND_DATA
https:docs.microsoft.com/en-us/windows/desktop/api/minwinbase/ns-minwinbase-_win32_find_dataa

FILE_INFO_BY_HANDLE_CLASS
https:msdn.microsoft.com/en-us/8f02e824-ca41-48c1-a5e8-5b12d81886b5
output for the following information classes
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

Output of NtQueryDirectoryFile
https:docs.microsoft.com/en-us/windows-hardware/drivers/ddi/content/ntifs/ns-ntifs-_file_both_dir_information

Cluster property list CLUSPROP_VALUE
https:docs.microsoft.com/en-us/windows/desktop/api/clusapi/ns-clusapi-clusprop_value
This module implements

function flat_forward_list_validate that
can be use to deal with untrusted buffers.
You can use it to validate if untrusted buffer
contains a valid list, and to find boundary at
which list gets invalid.
with polymorphic allocator for debugging contained.
elements

flat_forward_list_iterator and flat_forward_list_const_iterator
that can be used to enmirate over previously validated buffer

flat_forward_list a container that provides a set of helper
algorithms and manages y while list changes.

pmr_flat_forward_list is a an aliase of flat_forward_list
where allocatoe is polimorfic_allocator.

debug_memory_resource a memory resource that can be used along
with polimorfic allocator for debugging contained.

User is responsible for implementing helper class that has following methods
- tell us minimum required size element must have to be able to query element size
constexpr static size_t minimum_size() noexcept
and addition documentation in this mode right above where primary
constexpr static size_t get_next_element_offset(char const *buffer) noexcept
- update offset to the next element
constexpr static void set_next_element_offset(char *buffer, size_t size) noexcept
- calculate element size from data
constexpr static size_t calculate_next_element_offset(char const *buffer) noexcept
- validate that data fit into the buffer
constexpr static bool validate(size_t buffer_size, char const *buffer) noexcept

By default we are looking for a partial specialization for the element type.
For example:

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


for sample implementation see flat_forward_list_traits<FLAT_FORWARD_LIST_TEST> @ test\iffl_test_cases.cpp
and addition documetation in this mode right above where primary
template for flat_forward_list_traits is defined

If picking traits using partial specialization is not feasible then traits can be passed as
an explicit template parameter. For example:
   using ffl_iterator = iffl::flat_forward_list_iterator<FLAT_FORWARD_LIST_TEST, my_traits>;
