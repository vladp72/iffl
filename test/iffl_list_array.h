#include "iffl.h"
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

void print([[maybe_unused]] long_long_array_list_entry const &e) {
    std::printf("arr[%lu] = {", e.length);
    std::for_each(e.arr,
                  e.arr + e.length,
                  [](long long v) noexcept {
        std::printf("%lld ", v);
    });
    std::printf("}\n");
}

using char_array_list_entry = pod_array_list_entry<char>;

namespace iffl {
    template <>
    struct flat_forward_list_traits<char_array_list_entry>
        : public pod_array_list_entry_traits<char_array_list_entry::type> {
    };
}

using char_array_list = iffl::pmr_flat_forward_list<char_array_list_entry>;


void print([[maybe_unused]] char_array_list_entry const &e) {
    std::printf("arr[%lu] = {", e.length);
    std::for_each(e.arr,
                  e.arr + e.length,
                  [](char v) noexcept {
        std::printf("%i ", v);
    });
    std::printf("}\n");
}