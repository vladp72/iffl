#include "iffl.h"
#include "iffl_c_api_usecase2.h"
#include "iffl_list_array.h"

// 
//  This sample demostrates how to pass buffer ownership
//  across C interface boundary.
//  Server and client have to agree on allocator that will be used to 
//  allocate buffer.
//
//  server_api_call creates container, and fills it with data.
//  if no failures then it detaches buffer from container, and
//   assignes pointers to the buffer to the output parameters.
//
//  call_server checks if server_api_call succeeded then
//  it attaches buffers it got from server to a new container
//  that will be responsible for deallocating buffer.
//  Attch will validate buffer, but cunstructor will silently 
//  swallow if validation was able to find any valid entries.
//  If buffer did not contain a valid list then container will
//  take ownership of buffer, but it would not contain any elements.
//  If you are interested in finding result of validation then
//  use explicit attach method.
//

unsigned short idx{ 0 };

bool server_api_call2(char *buffer, size_t *buffer_size) noexcept {
    bool result{ false };

    if (!buffer || !buffer_size) {
        return result;
    }

    iffl::input_buffer_memory_resource input_buffer{reinterpret_cast<void *>(buffer), *buffer_size};

    char_array_list data{ &input_buffer };
    data.resize_buffer(*buffer_size);

    std::printf("Preparing output, input buffer size %zu\n", *buffer_size);

    for (unsigned short added_count{0};; ++idx, ++added_count) {
        size_t element_size{ char_array_list::traits::minimum_size() + idx * sizeof(char_array_list::value_type::type) };
        std::printf("Emplacing element [%03hu] element size %03zu (padded %02zu), capacity before {used %03zu, remainig %03zu}", 
                    idx,
                    element_size,
                    iffl::flat_forward_list_traits_traits<char_array_list_entry>::roundup_to_alignment(element_size),
                    data.used_capacity(),
                    data.remaining_capacity());
        if (!data.try_emplace_back(element_size,
                                    [] (char_array_list_entry &e,
                                           size_t element_size) noexcept {
                                        e.length = idx;
                                        std::fill(e.arr, e.arr + e.length, static_cast<char>(idx)+1);
                                    })) {
            data.fill_padding();
            *buffer_size = data.used_capacity();
            std::printf("\nServer was able to add %03hu arrays, used capacity %03zu\n", 
                        added_count,
                        *buffer_size);
            break;
        }
        result = true;
        std::printf(", capacity after {used %03zu, remainig %03zu}\n",
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
    // help detecting buffer overruns/underruns
    //
    iffl::debug_memory_resource client_memory_resource;
    //
    // Create flat forward list container, and resize
    // its buffer to the size we want to use to fetch 
    //
    char_array_list buffer{ &client_memory_resource };
    buffer.resize_buffer(100);

    size_t buffer_size{ buffer.total_capacity() };

    while (server_api_call2(buffer.data(), &buffer_size) && 0 != buffer_size) {
        if (buffer.revalidate_data(buffer_size)) {
            process_data2(buffer);
        } else {
            std::printf("Buffer revalidate failed. New buffer size %zu\n", buffer_size);
            break;
        }
        buffer_size = buffer.total_capacity();
    }
}


void run_ffl_c_api_usecase2() {
    iffl::flat_forward_list_traits_traits<char_array_list_entry>::print_traits_info();
    call_server2();
}
