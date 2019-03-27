#include "iffl.h"
#include "iffl_unaligned.h"
#include "iffl_list_array.h"

// 
//  This sample demonstrates how to handle a flat forward list when
//  caller does not properly aligns elements, and you cannot fix the
//  caller. All access to the element's data must be done using
//  pointers annotated as unaligned.
//  To achieve that you would need to define a custom trait that
//  will access data using pointer annotated as unaligned.
//  You also have to use pointer annotated as unaligned when accessing
//  elements in the container.
//
//  populate_container_of_unaligned_elements creates a container 
//  with some elements not aligned.
//
//  process_unaligned_view iterates over list and prints info about every 
//  element that is not properly aligned.
//

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

void populate_container_of_unaligned_elements(unaligned_char_array_list &data, size_t element_count) noexcept {
    size_t unaligned_count{ 0 };
    for (unsigned short idx = 0; idx < element_count; ++idx) {
        size_t element_size{ unaligned_char_array_list::traits::minimum_size() + idx * sizeof(unaligned_char_array_list::value_type::type) };
        //
        // since type traits explicitly specified aligned 1
        // container will not add padding to keep next element aligned 
        //
        data.emplace_back(element_size,
                          [&idx, &unaligned_count] (char_array_list_entry &e,
                                                    size_t element_size) noexcept {
                             //
                             // Element might be pointing to unaligned data
                             // make sure we access it using pointer marked as 
                             // unaligned
                             //
                              unaligned_char_array_list_entry_ptr unaligned_e_ptr{ &e };

                              if (iffl::roundup_ptr_to_alignment<char_array_list_entry>(unaligned_e_ptr) != unaligned_e_ptr) {
                                  ++unaligned_count;
                                  std::printf("Added char_array_list_entry length %03hu at unaligned address %p\n", 
                                              idx,
                                              reinterpret_cast<void*>(unaligned_e_ptr));
                              }

                              unaligned_e_ptr->length = idx;
                              std::fill(unaligned_e_ptr->arr, unaligned_e_ptr->arr + unaligned_e_ptr->length, static_cast<char>(idx)+1);
                          });
    }
    std::printf("Created collection with %zu total elements and %zu unaligned elements\n", 
                element_count, 
                unaligned_count);
}


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

void run_ffl_unaligned() {
    iffl::debug_memory_resource dbg_resource;
    unaligned_char_array_list data{ &dbg_resource };
    populate_container_of_unaligned_elements(data, 30);
    process_unaligned_view(unaligned_char_array_list_view{ data });
}
