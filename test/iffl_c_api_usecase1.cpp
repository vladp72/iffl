#include "iffl.h"
#include "iffl_c_api_usecase1.h"
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

iffl::debug_memory_resource global_memory_resource;

bool server_api_call1(char **buffer, size_t *buffer_size) noexcept {
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
        // After the call padding is filled with zeroes
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

void process_data1(char_array_list const &data) {
    for (auto const &array_list_entry : data) {
        print(array_list_entry);
    }
}

void call_server1() {
    char *buffer{ nullptr };
    size_t buffer_size{ 0 };

    if (server_api_call1(&buffer, &buffer_size)) {
        //
        // If server call succeeded the 
        // take ownershipt of the buffer
        //
        char_array_list data{ iffl::attach_buffer{},
                              buffer, 
                              buffer_size,
                              &global_memory_resource };
        process_data1(data);
    } else {
        FFL_CODDING_ERROR_IF(buffer || buffer_size);
    }
}


void run_ffl_c_api_usecase1() {
    iffl::flat_forward_list_traits_traits<char_array_list_entry>::print_traits_info();
    call_server1();
}
