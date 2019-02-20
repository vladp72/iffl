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
    size_t next_offset;
    size_t length;
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
    // This is the only method required by flat_forward_list_iterator.
    //
    constexpr static size_t get_next_offset(char const *buffer) noexcept {
        header_type const &e = *reinterpret_cast<header_type const *>(buffer);
        return e.next_offset;
    }
    //
    // This method is requiered for validate algorithm
    //
    constexpr static size_t minimum_size() noexcept {
        return FFL_SIZE_THROUGH_FIELD(header_type, length);
    }
    //
    // Helper method that calculates buffer size
    // Not required.
    // Is used by validate below
    //
    constexpr static size_t get_size(header_type const &e) {
        size_t size = FFL_FIELD_OFFSET(header_type, arr) + e.length * sizeof(T);
        FFL_CODDING_ERROR_IF_NOT(iffl::roundup_size_to_alignment<T>(size) == size);
        return size;
    }
    //
    // Required by container ir validate algorithm when type
    // does not support get_next_offset
    //
    constexpr static size_t get_size(char const *buffer) {
        return get_size(*reinterpret_cast<header_type const *>(buffer));
    }

    //
    // This method is required for validate algorithm
    //
    constexpr static bool validate(size_t buffer_size, char const *buffer) noexcept {
        header_type const &e = *reinterpret_cast<header_type const *>(buffer);
        if (e.next_offset == 0) {
            return  get_size(e) <= buffer_size;
        } else if (e.next_offset <= buffer_size) {
            return  get_size(e) <= e.next_offset;
        }
        return false;
    }
    //
    // This method is required by container
    //
    constexpr static void set_next_offset(char *buffer, size_t size) noexcept {
        header_type &e = *reinterpret_cast<header_type *>(buffer);
        FFL_CODDING_ERROR_IF_NOT(size == 0 ||
                                    size >= get_size(e));
        FFL_CODDING_ERROR_IF_NOT(iffl::roundup_size_to_alignment<T>(size) == size);
        e.next_offset = static_cast<unsigned long long>(size);
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

iffl::debug_memory_resource global_memory_resource;

bool server_api_call(char **buffer, size_t *buffer_size) noexcept {
    if (!buffer || !buffer_size) {
        return false;
    }

    try {
    
        long_long_array_list data{ &global_memory_resource };
    
        size_t array_size{ 10 };
        size_t element_buffer_size{ long_long_array_list_entry::byte_size_to_array_size(array_size) };
        long long pattern{ 1 };

        data.emplace_back(element_buffer_size,
                          [array_size, pattern] (char *buffer,
                                                 size_t element_size) {
                               long_long_array_list_entry &e {*reinterpret_cast<long_long_array_list_entry *>(buffer)};
                               e.length = array_size;
                               std::fill(e.arr, e.arr + e.length, pattern);
                          });

        array_size = 5;
        element_buffer_size = long_long_array_list_entry::byte_size_to_array_size(array_size);
        pattern = 2;

        data.emplace_back(element_buffer_size,
                          [array_size, pattern](char *buffer,
                                                size_t element_size) {
                              long_long_array_list_entry &e{ *reinterpret_cast<long_long_array_list_entry *>(buffer) };
                              e.length = array_size;
                              std::fill(e.arr, e.arr + e.length, pattern);
                          });

        array_size = 20;
        element_buffer_size = long_long_array_list_entry::byte_size_to_array_size(array_size);
        pattern = 3;

        data.emplace_back(element_buffer_size,
                          [array_size, pattern](char *buffer,
                                                size_t element_size) {
                              long_long_array_list_entry &e{ *reinterpret_cast<long_long_array_list_entry *>(buffer) };
                              e.length = array_size;
                              std::fill(e.arr, e.arr + e.length, pattern);
                          });

        array_size = 0;
        element_buffer_size = long_long_array_list_entry::byte_size_to_array_size(array_size);
        pattern = 4;

        data.emplace_back(element_buffer_size,
                          [array_size, pattern](char *buffer,
                                                size_t element_size) {
                              long_long_array_list_entry &e{ *reinterpret_cast<long_long_array_list_entry *>(buffer) };
                              e.length = array_size;
                              std::fill(e.arr, e.arr + e.length, pattern);
                          });

        array_size = 11;
        element_buffer_size = long_long_array_list_entry::byte_size_to_array_size(array_size);
        pattern = 5;

        data.emplace_back(element_buffer_size,
                          [array_size, pattern](char *buffer,
                                                size_t element_size) {
                              long_long_array_list_entry &e{ *reinterpret_cast<long_long_array_list_entry *>(buffer) };
                              e.length = array_size;
                              std::fill(e.arr, e.arr + e.length, pattern);
                          });
        //
        // Pass ownershipt of the buffer to the caller
        //
        size_t last_element_offset{ 0 };
        std::tie(*buffer, last_element_offset, *buffer_size) = data.detach();

    } catch (...) {
        return false;
    }
    return true;
}

void print([[maybe_unused]] long_long_array_list_entry const &e) {
    printf("arr[%zi] = {", e.length);
    std::for_each(e.arr, 
                  e.arr + e.length,
                  [](long long v) {
                     printf("%I64i ", v);
                  });
    printf("}\n");
}

void process_data(long_long_array_list const &data) {
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
        long_long_array_list data{ iffl::attach_buffer{}, 
                                   buffer, 
                                   buffer_size,
                                   &global_memory_resource };
        process_data(data);
    } else {
        FFL_CODDING_ERROR_IF(buffer || buffer_size);
    }
}


void run_ffl_c_api_usecase() {
    iffl::flat_forward_list_traits_traits<iffl::flat_forward_list_traits<long_long_array_list_entry>>::print_traits_info();
    call_server();
}
