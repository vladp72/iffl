#include "iffl.h"
#include "iffl_c_api_usecase2.h"
#include "iffl_list_array.h"

// 
//  This sample demonstrates how to use container on the client
//  to process batches of data from the server, and how to use 
//  container with input_buffer_memory_resource to fill buffer
//  with output.
//
//  server_api_call creates container that with the help of
//  input_buffer_memory_resource temporarily attaches to the 
//  input buffer, and fills it with data using try_* functions,
//  that avoid reallocation, and instead would return failure.
//
//  call_server checks if server_api_call succeeded then
//  it revalidates data returned in the buffer, and processes 
//  them. Client keeps calling server for the next back as long
//  as server can returns more data.
//

//
// Keep  track where server stopped last time
//
unsigned short idx{ 0 };

bool server_api_call2(char *buffer, size_t *buffer_size) noexcept {
    bool result{ false };
    //
    // If user did not pass a valid buffer then bail out
    //
    if (!buffer || !buffer_size) {
        return result;
    }
    //
    // When container allocates memory this memory resource 
    // returns pointer to the input buffer
    //
    iffl::input_buffer_memory_resource input_buffer{reinterpret_cast<void *>(buffer), 
                                                    *buffer_size};
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

    std::printf("Preparing output, input buffer size %zu\n", *buffer_size);
    //
    // idx is a global variable that remembers where server stopped last time
    //
    for (unsigned short added_count{0};; ++idx, ++added_count) {
        //
        // Estimate size of the element we are inserting
        //
        size_t element_size{ char_array_list::traits::minimum_size() + idx * sizeof(char_array_list::value_type::type) };
        std::printf("Emplacing element [%03hu] element size %03zu (padded %02zu), capacity before {used %03zu, remaining %03zu}", 
                    idx,
                    element_size,
                    iffl::flat_forward_list_traits_traits<char_array_list_entry>::roundup_to_alignment(element_size),
                    data.used_capacity(),
                    data.remaining_capacity());
        //
        // try_emplace_back will succeed if we can add element
        // without triggering reallocation, otherwise it will return 
        // false.
        //
        if (!data.try_emplace_back(element_size,
                                    [] (char_array_list_entry &e,
                                        size_t element_size) noexcept {
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
            std::printf("\nServer was able to add %03hu arrays, used capacity %03zu\n", 
                        added_count,
                        *buffer_size);
            break;
        }
        //
        // If we inserted at least one element then return true
        //
        result = true;
        std::printf(", capacity after {used %03zu, remaining %03zu}\n",
                    data.used_capacity(),
                    data.remaining_capacity());

    }
    return result;
}

void process_data2(char_array_list const &data) {
    for (auto const &array_list_entry : data) {
        print(array_list_entry);
    }
}

void call_server2() {
    //
    // Client uses debug memory resource to 
    // help detecting buffer overruns/under-runs
    //
    iffl::debug_memory_resource client_memory_resource;
    //
    // Create flat forward list container, and resize
    // its buffer to the size we want to use to fetch 
    // data from server
    //
    char_array_list buffer{ &client_memory_resource };
    buffer.resize_buffer(100);
    //
    // We will use this variable to communicate with server
    // how large is buffer, and after server returns it will
    // tell us how much data buffer contains
    //
    size_t buffer_size{ buffer.total_capacity() };
    //
    // Keep asking server for the next batch of data
    // as long as last call succeeded and returned some data
    //
    while (server_api_call2(buffer.data(), &buffer_size) && 0 != buffer_size) {
        //
        // Server changed buffer content, which effectively invalidated
        // container state. for instance last element how might be at a 
        // different position. Call revalidate_data tells container to 
        // verify that we have a valid ffl. Passing buffer_size tells 
        // container upper watermark for the valid data.
        //
        if (buffer.revalidate_data(buffer_size)) {
            //
            // If buffer contains a valid ffl then now we can safely iterate 
            // over the buffer
            //
            process_data2(buffer);
        } else {
            //
            // Server returned invalid list. Abort.
            //
            std::printf("Buffer revalidation failed. New buffer size %zu\n", buffer_size);
            break;
        }
        //
        // We are about to call server for the next batch.
        // THis call will change buffer content, and will invalidate
        // position of the last element of the list.
        // Tell container to forget where last element is so we would not
        // antecedently attempt to use broken list.
        // This call does not change size of the buffer.
        //
        buffer.erase_all();
        //
        // After last call to server buffer_size contains number of
        // bytes returned by the server. Update it with buffer size
        // before starting next call.
        //
        buffer_size = buffer.total_capacity();
    }
}


void run_ffl_c_api_usecase2() {
    iffl::flat_forward_list_traits_traits<char_array_list_entry>::print_traits_info();
    call_server2();
}
