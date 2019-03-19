#include "iffl.h"
#include "iffl_c_api_usecase.h"

//
// This sample demonstrates
//
// How to define a generic header that can be used for types that
// do not have intrusive hook.
// Templace pod_array_list_entry that can be used for any POD type
// Template pod_array_list_entry_traits defines traits implementation
// To use them with your type
//
//  1. Instantiate pod_array_list_entry
//      using long_long_array_list_entry = pod_array_list_entry<long long>
//
//  2. Define traits and implement them using instantiation of pod_array_list_entry_traits
//      namespace iffl {
//          template <>
//          struct flat_forward_list_traits<long_long_array_list_entry>
//              : public pod_array_list_entry_traits<long_long_array_list_entry::type> {
//          };
//      }
//
//  3. Instantiate list of arrays
//      using long_long_array_list = iffl::pmr_flat_forward_list<long_long_array_list_entry>;
// 
//  This sample also demostrates how to pass buffer ownership
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

//
// List of arrays of Ts 
//
template <typename T>
struct pod_array_list_entry {
    unsigned long length;
    T arr[1];

    using type = T;

    constexpr static size_t byte_size_to_array_size(size_t array_size) {
        return FFL_FIELD_OFFSET(pod_array_list_entry, arr) + array_size * sizeof(T);
    }

    constexpr static size_t array_size_to_byte_size(size_t array_size) {
        if (array_size < sizeof(pod_array_list_entry)) {
            return 0;
        } else {
            return (array_size - FFL_FIELD_OFFSET(pod_array_list_entry, arr)) / sizeof(T);
        }
    }
};

template <typename T>
struct pod_array_list_entry_traits {

    using header_type = pod_array_list_entry<T>;

    constexpr static size_t const alignment{ alignof(header_type) };

    //
    // This method is requiered for validate algorithm
    //
    constexpr static size_t minimum_size() noexcept {
        return FFL_SIZE_THROUGH_FIELD(header_type, length);
    }
    //
    // Required by container ir validate algorithm when type
    // does not support get_next_offset
    //
    constexpr static size_t get_size(header_type const &e) {
        size_t const size = FFL_FIELD_OFFSET(header_type, arr) + e.length * sizeof(T);
        FFL_CODDING_ERROR_IF_NOT(iffl::roundup_size_to_alignment<T>(size) == size);
        return size;
    }
    //
    // This method is required for validate algorithm
    //
    constexpr static bool validate(size_t buffer_size, header_type const &e) noexcept {
        return  get_size(e) <= buffer_size;
    }
};

using long_long_array_list_entry = pod_array_list_entry<long long>;

namespace iffl {
    template <>
    struct flat_forward_list_traits<long_long_array_list_entry>
        : public pod_array_list_entry_traits<long_long_array_list_entry::type> {
    };
}

using long_long_array_list = iffl::pmr_flat_forward_list<long_long_array_list_entry>;

using char_array_list_entry = pod_array_list_entry<char>;

namespace iffl {
    template <>
    struct flat_forward_list_traits<char_array_list_entry>
        : public pod_array_list_entry_traits<char_array_list_entry::type> {
    };
}

using char_array_list = iffl::pmr_flat_forward_list<char_array_list_entry>;

iffl::debug_memory_resource global_memory_resource;

bool server_api_call(char **buffer, size_t *buffer_size) noexcept {
    if (!buffer || !buffer_size) {
        return false;
    }

    try {
    
        char_array_list data{ &global_memory_resource };
    
        unsigned long array_size{ 10 };
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

        iffl::buffer_ref detached_buffer{ data.detach() };
        *buffer = detached_buffer.begin;
        *buffer_size = detached_buffer.size();

    } catch (...) {
        return false;
    }
    return true;
}

void print([[maybe_unused]] char_array_list_entry const &e) {
    std::printf("arr[%lu] = {", e.length);
    std::for_each(e.arr, 
                  e.arr + e.length,
                  [](char v) noexcept {
                     std::printf("%i ", v);
                  });
    std::printf("}\n");
}

void process_data(char_array_list const &data) {
    for (auto const &array_list_entry : data) {
        print(array_list_entry);
    }
}

void call_server() {
    char *buffer{ nullptr };
    size_t buffer_size{ 0 };

    if (server_api_call(&buffer, &buffer_size)) {
        //
        // If server call succeeded the 
        // take ownershipt of the buffer
        //
        char_array_list data{ iffl::attach_buffer{},
                              buffer, 
                              buffer_size,
                              &global_memory_resource };
        process_data(data);
    } else {
        FFL_CODDING_ERROR_IF(buffer || buffer_size);
    }
}


void run_ffl_c_api_usecase() {
    iffl::flat_forward_list_traits_traits<char_array_list_entry>::print_traits_info();
    call_server();
}
