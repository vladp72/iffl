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

bool server_api_call2(char *buffer, size_t *buffer_size) noexcept {
    if (!buffer || !buffer_size) {
        return false;
    }

    iffl::input_buffer_memory_resource input_buffer{reinterpret_cast<void *>(buffer), *buffer_size};

    char_array_list data{ &input_buffer };
    data.resize_buffer(*buffer_size);

    std::printf("Preparing output, input buffer size %llu\n", *buffer_size);

    for (unsigned long len{ 0 }; ; ++len) {
        if (!data.try_emplace_back(char_array_list::traits::minimum_size() + len * sizeof(char_array_list::value_type::type),
                            [len] (char_array_list_entry &e,
                                   size_t element_size) noexcept {
                                e.length = len;
                                std::fill(e.arr, e.arr + e.length, static_cast<char>(len)+1);
                            })) {
            data.fill_padding();
            *buffer_size = data.used_capacity();
            std::printf("Server was able to add %lu arrays, used capacity %llu\n", len, *buffer_size);
            break;
        }
    }
    return true;
}

void process_data2(char_array_list const &data) {
    for (auto const &array_list_entry : data) {
        print(array_list_entry);
    }
}

void call_server2() {
    iffl::debug_memory_resource client_memory_resource;

    char_array_list buffer{ &client_memory_resource };

    buffer.resize_buffer(100);
    size_t buffer_size{ buffer.total_capacity() };

    if (server_api_call2(buffer.data(), &buffer_size)) {
        if (buffer.revalidate_data(buffer_size)) {
            process_data2(buffer);
        } else {
            std::printf("Buffer revalidate failed. New buffer size %llu\n", buffer_size);
        }
    }
}


void run_ffl_c_api_usecase2() {
    iffl::flat_forward_list_traits_traits<char_array_list_entry>::print_traits_info();
    call_server2();
}
