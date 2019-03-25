#include "iffl.h"
#include "iffl_views.h"
#include "iffl_list_array.h"

// 
//  This sample demonstrates how to split container into sized views.
//  Views can be passed to another thread for processing
//
//  populate_container populates container with elements.
//
//  process_batch processes batch of elements passed as a view.
//
//  process_container_in_batches splits container into equally sized 
//  views and passes them for processing to process_batch.
//

void populate_container(char_array_list &data, size_t element_count) noexcept {
    for (unsigned short idx = 0; idx < element_count; ++idx) {
        size_t element_size{ char_array_list::traits::minimum_size() + idx * sizeof(char_array_list::value_type::type) };
        data.emplace_back(element_size,
                          [&idx] (char_array_list_entry &e,
                                  size_t element_size) noexcept {
                              e.length = idx;
                              std::fill(e.arr, e.arr + e.length, static_cast<char>(idx)+1);
                          });
    }
    std::printf("Created collection with %zu elements\n", element_count);
}

void process_batch(size_t batch_number, char_array_list_view view) {
    std::printf("--Start processing batch # %zu\n", batch_number);
    for (auto const &array_list_entry : view) {
        print(array_list_entry);
    }
    std::printf("--Complete processing batch # %zu\n", batch_number);
}

void process_container_in_batches(char_array_list const &data, size_t batch_size) {

    FFL_CODDING_ERROR_IF(0 == batch_size);

    auto batch_begin{ data.cbegin() };
    auto batch_end{ data.cbegin() };
    auto cur{ data.cbegin() };
    auto end{ data.cend() };
    size_t element_in_batch{ 0 };

    size_t batch_number{ 0 };

    for (; cur != end; ++cur) {
        ++element_in_batch;
        batch_end = cur;
        //
        // If we've accumulated batch size of elements
        // then post it for processing.
        //
        if (batch_size == element_in_batch) {
            process_batch(++batch_number, { batch_begin, batch_end });
            element_in_batch = 0;
            ++cur;
            if (cur == end) {
                break;
            }
            batch_begin = cur;
        }
    }
    //
    // if we've any tail left smaller then batch 
    // size then post process it for processing
    //
    if (0 < element_in_batch) {
        process_batch(++batch_number, { batch_begin, batch_end });
    }
}

void run_ffl_views() {
    iffl::debug_memory_resource dbg_resource;
    char_array_list data{ &dbg_resource };
    populate_container(data, 30);
    process_container_in_batches(data, 11);
}
