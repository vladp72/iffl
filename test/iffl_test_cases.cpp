#include "iffl.h"
#include "iffl_test_cases.h"

typedef struct _FLAT_FORWARD_LIST_TEST {
    size_t NextEntryOffset;
    size_t Type;
    size_t DataLength;
} FLAT_FORWARD_LIST_TEST, *PFLAT_FORWARD_LIST_TEST;

void print_element(FLAT_FORWARD_LIST_TEST const &e) noexcept {
    std::printf("[NextEntryOffset %zu, Type %zu, DataLength %zu, Data {",
           e.NextEntryOffset,
           e.Type,
           e.DataLength);
    unsigned char const *first = reinterpret_cast<unsigned char const *>(&e) + sizeof(FLAT_FORWARD_LIST_TEST);
    unsigned char const *end = first + e.DataLength;
    for (; first != end; ++first) {
        std::printf("%x", *first);
    }
    std::printf("}]\n");
}

namespace iffl {
    template <>
    struct flat_forward_list_traits<FLAT_FORWARD_LIST_TEST> {

        constexpr static size_t const alignment{ alignof(FLAT_FORWARD_LIST_TEST) };
        //
        // This is the only method required by flat_forward_list_iterator.
        //
        constexpr static size_t get_next_offset(char const *buffer) noexcept {
            FLAT_FORWARD_LIST_TEST const &e = *reinterpret_cast<FLAT_FORWARD_LIST_TEST const *>(buffer);
            return e.NextEntryOffset;
        }
        //
        // This method is requiered for validate algorithm
        //
        constexpr static size_t minimum_size() noexcept {
            return FFL_SIZE_THROUGH_FIELD(FLAT_FORWARD_LIST_TEST, DataLength);
        }
        //
        // Helper method that calculates buffer size
        // Not required.
        // Is used by validate below
        //
        constexpr static size_t get_size(FLAT_FORWARD_LIST_TEST const &e) {
            return  FFL_SIZE_THROUGH_FIELD(FLAT_FORWARD_LIST_TEST, DataLength) +
                    e.DataLength;
        }
        constexpr static size_t get_size(char const *buffer) {
            return get_size(*reinterpret_cast<FLAT_FORWARD_LIST_TEST const *>(buffer));
        }
        //
        // This method is required for validate algorithm
        //
        constexpr static bool validate(size_t buffer_size, char const *buffer) noexcept {
            FLAT_FORWARD_LIST_TEST const &e = *reinterpret_cast<FLAT_FORWARD_LIST_TEST const *>(buffer);
            //
            // if this is last element then make sure it fits in the reminder of buffer
            // otherwise make sure that pointer to the next element fits reminder of buffer
            // and this element size fits before next element starts
            //
            // Note that when we evaluate if buffer is valid we are not requiering
            // that it is propertly aligned. An implementation can choose to enforce 
            // that
            //
            //
            if (e.NextEntryOffset == 0) {
                return  get_size(e) <= buffer_size;
            } else if (e.NextEntryOffset <= buffer_size) {
                return  get_size(e) <= e.NextEntryOffset;
            }
            return false;
        }
        //
        // This method is required by container
        //
        constexpr static void set_next_offset(char *buffer, size_t size) noexcept {
            FLAT_FORWARD_LIST_TEST &e = *reinterpret_cast<FLAT_FORWARD_LIST_TEST *>(buffer);
            FFL_CODDING_ERROR_IF_NOT(size == 0 || size >= get_size(e));
            //
            // Validate alignment
            //
            FFL_CODDING_ERROR_IF_NOT(roundup_ptr_to_alignment(buffer, alignment) == buffer);
            FFL_CODDING_ERROR_IF_NOT(0 == size || roundup_size_to_alignment(size, alignment) == size);
            e.NextEntryOffset = size;
        }
    };

    template <>
    struct flat_forward_list_traits<FLAT_FORWARD_LIST_TEST const>
        : public flat_forward_list_traits < FLAT_FORWARD_LIST_TEST > {
    };
}

using ffl_iterator = iffl::flat_forward_list_iterator<FLAT_FORWARD_LIST_TEST>;
using ffl_const_iterator = iffl::flat_forward_list_const_iterator<FLAT_FORWARD_LIST_TEST>;

void flat_forward_list_validate_test1(char const * title, bool expected_to_be_valid, char const *first, char const *end) {
    std::printf("-----\"%s\"-----\n", title);
    auto[is_valid, buffer_view] = iffl::flat_forward_list_validate<FLAT_FORWARD_LIST_TEST>(first, end);
    FFL_CODDING_ERROR_IF_NOT(is_valid == expected_to_be_valid);
    if (is_valid) {

        std::for_each(buffer_view.begin(), 
                      buffer_view.end(), 
                      [](FLAT_FORWARD_LIST_TEST const &e) noexcept {
            print_element(e);
        });
    }
}

template<size_t N>
void flat_forward_list_validate_array_test1(char const * title, bool expected_to_be_valid, FLAT_FORWARD_LIST_TEST const (&a)[N]) {
    std::printf("-----\"%s\"-----\n", title);
    auto[is_valid, buffer_view] = iffl::flat_forward_list_validate(a, a + N);
    FFL_CODDING_ERROR_IF_NOT(is_valid == expected_to_be_valid);

    if (is_valid) {
        std::for_each(buffer_view.begin(), 
                      buffer_view.end(), 
                      [](FLAT_FORWARD_LIST_TEST const &e) noexcept {
            print_element(e);
        });
    }
}

//
// Valid list
//
FLAT_FORWARD_LIST_TEST const cve1[] = {
    {sizeof(FLAT_FORWARD_LIST_TEST), 1, 0 },
    { sizeof(FLAT_FORWARD_LIST_TEST), 2, 0 },
    { sizeof(FLAT_FORWARD_LIST_TEST), 3, 0 },
    { sizeof(FLAT_FORWARD_LIST_TEST), 4, 0 },
    { 0, 5, 0 },
};

//
// Same as above
//
FLAT_FORWARD_LIST_TEST ve1[] = {
    {sizeof(FLAT_FORWARD_LIST_TEST), 1, 0 },
    { sizeof(FLAT_FORWARD_LIST_TEST), 2, 0 },
    { sizeof(FLAT_FORWARD_LIST_TEST), 3, 0 },
    { sizeof(FLAT_FORWARD_LIST_TEST), 4, 0 },
    { 0, 5, 0 },
};

//
// Valid list that ends at element 5
//
FLAT_FORWARD_LIST_TEST const cve2[] = {
    {sizeof(FLAT_FORWARD_LIST_TEST), 1, 0},
    {sizeof(FLAT_FORWARD_LIST_TEST), 2, 0},
    {sizeof(FLAT_FORWARD_LIST_TEST), 3, 0},
    {sizeof(FLAT_FORWARD_LIST_TEST), 4, 0},
    {0, 5, 0},
    {sizeof(FLAT_FORWARD_LIST_TEST), 6, 0},
};

//
// Valid list element 6 and 7 contain data for the element 5 
// so we will skip over it
//
FLAT_FORWARD_LIST_TEST const cve3[] = {
    {sizeof(FLAT_FORWARD_LIST_TEST), 1, 0},
    {sizeof(FLAT_FORWARD_LIST_TEST), 2, 0},
    {sizeof(FLAT_FORWARD_LIST_TEST), 3, 0},
    {sizeof(FLAT_FORWARD_LIST_TEST), 4, 0},
    {3 * sizeof(FLAT_FORWARD_LIST_TEST), 5, 2 * sizeof(FLAT_FORWARD_LIST_TEST)},
    {0xFFFFFFF1, 0xFFFFFFF2, 0xFFFFFFF3},
    {0xFFFFFFF4, 0xFFFFFFF5, 0xFFFFFFF6},
    {0, 6, 0},
};

//
// Valid list. Some elements are skipped as data 
//
FLAT_FORWARD_LIST_TEST const cve4[] = {
    {2 * sizeof(FLAT_FORWARD_LIST_TEST), 1, 1 * sizeof(FLAT_FORWARD_LIST_TEST)},
    {0xFFFFFFF1, 0xFFFFFFF2, 0xFFFFFFF3},
    {3 * sizeof(FLAT_FORWARD_LIST_TEST), 2, 2 * sizeof(FLAT_FORWARD_LIST_TEST)},
    {0xFFFFFFF1, 0xFFFFFFF2, 0xFFFFFFF3},
    {0xFFFFFFF4, 0xFFFFFFF5, 0xFFFFFFF6},
    {4 * sizeof(FLAT_FORWARD_LIST_TEST), 3, 3 * sizeof(FLAT_FORWARD_LIST_TEST)},
    {0xFFFFFFF1, 0xFFFFFFF2, 0xFFFFFFF3},
    {0xFFFFFFF4, 0xFFFFFFF5, 0xFFFFFFF6},
    {0xFFFFFFF7, 0xFFFFFFF8, 0xFFFFFFF9},
    {0, 4, 4 * sizeof(FLAT_FORWARD_LIST_TEST)},
    {0xFFFFFFF1, 0xFFFFFFF2, 0xFFFFFFF3},
    {0xFFFFFFF4, 0xFFFFFFF5, 0xFFFFFFF6},
    {0xFFFFFFF7, 0xFFFFFFF8, 0xFFFFFFF9},
    {0xFFFFFFFA, 0xFFFFFFFB, 0xFFFFFFFC},
};

//
// Not valid. Element 5 is pointing to the next element beyond
// buffer.
//
FLAT_FORWARD_LIST_TEST const cie1[] = {
    {sizeof(FLAT_FORWARD_LIST_TEST), 1, 0},
    {sizeof(FLAT_FORWARD_LIST_TEST), 2, 0},
    {sizeof(FLAT_FORWARD_LIST_TEST), 3, 0},
    {sizeof(FLAT_FORWARD_LIST_TEST), 4, 0},
    {sizeof(FLAT_FORWARD_LIST_TEST), 5, 0},
};

//
// Not valid. Element 3 next offset is not pointing to element 4
//
FLAT_FORWARD_LIST_TEST const cie2[] = {
    {sizeof(FLAT_FORWARD_LIST_TEST), 1, 0},
    {sizeof(FLAT_FORWARD_LIST_TEST), 2, 0},
    {1, 3, 0},
    {sizeof(FLAT_FORWARD_LIST_TEST), 4, 0},
    {sizeof(FLAT_FORWARD_LIST_TEST), 5, 0},
};

//
// Not valid. Element 3 data length is spanning to the next element
//
FLAT_FORWARD_LIST_TEST const cie3[] = {
    {sizeof(FLAT_FORWARD_LIST_TEST), 1, 0},
    {sizeof(FLAT_FORWARD_LIST_TEST), 2, 0},
    {1, 3, 1},
    {sizeof(FLAT_FORWARD_LIST_TEST), 4, 0},
    {sizeof(FLAT_FORWARD_LIST_TEST), 5, 0},
};

//
// Not valid. last element data is going beyond buffer size 
//
FLAT_FORWARD_LIST_TEST const cie4[] = {
    {2 * sizeof(FLAT_FORWARD_LIST_TEST), 1, 1 * sizeof(FLAT_FORWARD_LIST_TEST)},
    {0xFFFFFFF1, 0xFFFFFFF2, 0xFFFFFFF3},
    {3 * sizeof(FLAT_FORWARD_LIST_TEST), 2, 2 * sizeof(FLAT_FORWARD_LIST_TEST)},
    {0xFFFFFFF1, 0xFFFFFFF2, 0xFFFFFFF3},
    {0xFFFFFFF4, 0xFFFFFFF5, 0xFFFFFFF6},
    {4 * sizeof(FLAT_FORWARD_LIST_TEST), 3, 3 * sizeof(FLAT_FORWARD_LIST_TEST)},
    {0xFFFFFFF1, 0xFFFFFFF2, 0xFFFFFFF3},
    {0xFFFFFFF4, 0xFFFFFFF5, 0xFFFFFFF6},
    {0xFFFFFFF7, 0xFFFFFFF8, 0xFFFFFFF9},
    {0, 4, 4 * sizeof(FLAT_FORWARD_LIST_TEST)},
    {0xFFFFFFF1, 0xFFFFFFF2, 0xFFFFFFF3},
    {0xFFFFFFF4, 0xFFFFFFF5, 0xFFFFFFF6},
    {0xFFFFFFF7, 0xFFFFFFF8, 0xFFFFFFF9},
};

//
// Not valid. 3rd element data is going beyond element 
// size size and spans to element 4
//
FLAT_FORWARD_LIST_TEST const cie5[] = {
    {2 * sizeof(FLAT_FORWARD_LIST_TEST), 1, 1 * sizeof(FLAT_FORWARD_LIST_TEST)},
    {0xFFFFFFF1, 0xFFFFFFF2, 0xFFFFFFF3},
    {3 * sizeof(FLAT_FORWARD_LIST_TEST), 2, 2 * sizeof(FLAT_FORWARD_LIST_TEST)},
    {0xFFFFFFF1, 0xFFFFFFF2, 0xFFFFFFF3},
    {0xFFFFFFF4, 0xFFFFFFF5, 0xFFFFFFF6},
    {3 * sizeof(FLAT_FORWARD_LIST_TEST), 3, 3 * sizeof(FLAT_FORWARD_LIST_TEST)},
    {0xFFFFFFF1, 0xFFFFFFF2, 0xFFFFFFF3},
    {0xFFFFFFF4, 0xFFFFFFF5, 0xFFFFFFF6},
    {0, 4, 4 * sizeof(FLAT_FORWARD_LIST_TEST)},
    {0xFFFFFFF1, 0xFFFFFFF2, 0xFFFFFFF3},
    {0xFFFFFFF4, 0xFFFFFFF5, 0xFFFFFFF6},
    {0xFFFFFFF7, 0xFFFFFFF8, 0xFFFFFFF9},
    {0xFFFFFFFA, 0xFFFFFFFB, 0xFFFFFFFC},
};

inline void flat_forward_list_validate_test1() {
    //
    // Valid list
    //
    flat_forward_list_validate_test1("nullptr - nullptr", true, nullptr, nullptr);
    //
    // Valid empty list
    //
    flat_forward_list_validate_test1("1 - 1", true, (char const *)1, (char const *)1);
    //
    // first element is too short
    //
    flat_forward_list_validate_test1("1 = 1 + sizeof(FLAT_FORWARD_LIST_TEST) - 1", false, (char const *)1, (char const *)1 + sizeof(FLAT_FORWARD_LIST_TEST) - 1);

    flat_forward_list_validate_array_test1("cve1", true, cve1);
    flat_forward_list_validate_array_test1("cve2", true, cve2);
    flat_forward_list_validate_array_test1("cve3", true, cve3);
    flat_forward_list_validate_array_test1("cve4", true, cve4);

    flat_forward_list_validate_array_test1("cie1", false, cie1);
    flat_forward_list_validate_array_test1("cie2", false, cie2);
    flat_forward_list_validate_array_test1("cie3", false, cie3);
    flat_forward_list_validate_array_test1("cie4", false, cie4);
    flat_forward_list_validate_array_test1("cie5", false, cie5);
}

inline void flat_forward_list_iterator_test1() {
    std::printf("----- flat_forward_list_iterator over \"ver1\"-----\n");
    auto [is_valid, view] = iffl::flat_forward_list_validate(std::begin(ve1), std::end(ve1));
    iffl::unused_variable(is_valid);
    std::for_each(view.begin(), view.begin(), 
                  [](FLAT_FORWARD_LIST_TEST &e) noexcept {
                      print_element(e);
                  });
}

//
// This function is not designed to be called
// it is for checking that stuff compiles.
// Runnig this function will cause access violation.
//
inline void flat_forward_list_iterator_syntax_check() {

    static_assert(!std::is_const_v<ffl_iterator::value_type>,
                  "This is a non-const iterator");
    static_assert(std::is_const_v<ffl_const_iterator::value_type>,
                  "This is a const iterator");
    static_assert(std::is_same_v<std::remove_cv_t<ffl_iterator::value_type>,
                  std::remove_cv_t<ffl_const_iterator::value_type>>,
                  "Elements should differ only by constness");

    ffl_iterator bli1;
    ffl_iterator bli2{ bli1 };
    ffl_iterator bli3{ std::move(bli1) };

    bli2 = bli1;
    bli3 = std::move(bli1);

    ffl_iterator const bli4;
    [[maybe_unused]] ffl_iterator const bli5{ bli1 };
    ffl_iterator const bli6{ std::move(bli1) };

    bli2 = bli4;

    ffl_const_iterator blci1;
    ffl_const_iterator blci2{ blci1 };
    ffl_const_iterator blci3{ std::move(blci1) };

    blci2 = blci1;
    blci3 = std::move(blci1);

    ffl_const_iterator const blci4;
    ffl_const_iterator const blci5{ blci1 };
    [[maybe_unused]] ffl_const_iterator const blci6{ blci5 };
    ffl_const_iterator const blci7{ std::move(blci1) };

    blci2 = blci4;

    ffl_const_iterator blci8{ bli1 };
    [[maybe_unused]] ffl_const_iterator const blci9{ bli4 };
    ffl_const_iterator const blci10{ std::move(bli1) };

    blci8 = bli1;
    blci8 = bli4;
    blci8 = std::move(bli1);

    //
    // Constructing non-const iterator from 
    // const iterator should be failing
    //
    //ffl_iterator bli_nc1{ blci1 };
    //ffl_iterator bli_nc2{ std::move(blci1) };
    //

    //
    // Assignment non-const iterator from 
    // const iterator should be failing
    //
    [[maybe_unused]] ffl_iterator const bli_nc3;
    [[maybe_unused]] ffl_iterator const bli_nc4;
    //
    // bli_nc3 = blci1;
    // bli_nc4 = std::move(blci1);
    //

    //
    // We can compare two non-const iterators
    //
    iffl::unused_expression_result(bli1 == bli2);
    iffl::unused_expression_result(bli1 != bli2);
    //
    // We can compare two const iterators
    //
    iffl::unused_expression_result(blci1 == blci2);
    iffl::unused_expression_result(blci1 != blci2);
    //
    // We can compare const and non-const iterator
    //
    iffl::unused_expression_result(bli1 == blci1);
    iffl::unused_expression_result(blci1 == bli1);
    iffl::unused_expression_result(bli4 == blci4);
    iffl::unused_expression_result(blci4 == bli4);
    
    std::for_each(bli1, bli2, [](FLAT_FORWARD_LIST_TEST &) {});

    std::for_each(blci1, blci2, [](FLAT_FORWARD_LIST_TEST const &) {});
}

void flat_forward_list_push_back_test1() {
    
    iffl::debug_memory_resource dbg_memory_resource;
    iffl::pmr_flat_forward_list<FLAT_FORWARD_LIST_TEST> ffl{ &dbg_memory_resource };
    //
    // every push is extending buffer
    //
    size_t const iterations_count{ 100 };
    for (size_t i = 1; i <= iterations_count; ++i) {
        ffl.push_back(i * sizeof(FLAT_FORWARD_LIST_TEST));
        FLAT_FORWARD_LIST_TEST const &back = ffl.back();
        FFL_CODDING_ERROR_IF_NOT(back.NextEntryOffset == 0);
    }
    //
    // Erase elements without shrinking buffer
    //
    ffl.erase_all();
    //
    // Push do not resize buffer
    //
    for (size_t i = iterations_count; i > 0; --i) {
        ffl.push_back(i * sizeof(FLAT_FORWARD_LIST_TEST));
    }
}

void flat_forward_list_push_front_test1() {

    iffl::debug_memory_resource dbg_memory_resource;
    iffl::pmr_flat_forward_list<FLAT_FORWARD_LIST_TEST> ffl{ &dbg_memory_resource };
    //
    // every push is extending buffer
    //
    size_t const iterations_count{ 100 };
    for (size_t i = 1; i < iterations_count; ++i) {
        ffl.push_front(i * sizeof(FLAT_FORWARD_LIST_TEST));
        FLAT_FORWARD_LIST_TEST const &front = ffl.front();
        if (i == 1) {
            FFL_CODDING_ERROR_IF_NOT(front.NextEntryOffset == 0);
        } else {
            FFL_CODDING_ERROR_IF(front.NextEntryOffset == 0);
        }
    }
    //
    // Pop elements without shrinking buffer
    //
    while (!ffl.empty()) {
        FLAT_FORWARD_LIST_TEST const &front = ffl.front();
        if (1 == ffl.size()) {
            FFL_CODDING_ERROR_IF_NOT(front.NextEntryOffset == 0);
        } else {
            FFL_CODDING_ERROR_IF(front.NextEntryOffset == 0);
        }
        ffl.pop_front();
    }
    //
    // Push do not resize buffer
    //
    for (size_t i = iterations_count; i > 0; --i) {
        ffl.push_front(i * sizeof(FLAT_FORWARD_LIST_TEST));
    }
}

template<typename T1, typename T2>
void test_swap(T1 &lhs, T2 &rhs) {
    //
    // we are doing even swaps so at the end state
    // of containers will be unchanged
    //
    lhs.swap(rhs);
    rhs.swap(lhs);
    //
    // pick using ADL
    //
    swap(lhs, rhs);
    swap(rhs, lhs);
    //
    // pick standard
    //
    std::swap(lhs, rhs);
    std::swap(rhs, lhs);
}

template<typename T>
void fill_container_with_data(T &ffl, size_t extra_reserve = 0 ) {

    size_t const iterations_count{ 100 };
    for (size_t i = 1; i <= iterations_count; ++i) {
        size_t const element_size{ i * sizeof(FLAT_FORWARD_LIST_TEST) };
        ffl.emplace_back(element_size,
                         [i, element_size](char *buffer,
                                           size_t new_element_size) {
                            FFL_CODDING_ERROR_IF_NOT(element_size == new_element_size);
                            FFL_CODDING_ERROR_IF(element_size < sizeof(FLAT_FORWARD_LIST_TEST));
                            FLAT_FORWARD_LIST_TEST &e = *reinterpret_cast<FLAT_FORWARD_LIST_TEST *>(buffer);
                            e.Type = i;
                            e.DataLength = element_size - sizeof(FLAT_FORWARD_LIST_TEST);
                        });
        FLAT_FORWARD_LIST_TEST const &back = ffl.back();
        FFL_CODDING_ERROR_IF_NOT(back.NextEntryOffset == 0);
        FFL_CODDING_ERROR_IF_NOT(ffl.used_capacity() == ffl.total_capacity());
        FFL_CODDING_ERROR_IF_NOT(i == ffl.size());
    }
    ffl.resize_buffer(ffl.total_capacity() + extra_reserve);
    FFL_CODDING_ERROR_IF_NOT(iterations_count == ffl.size());
}

void flat_forward_list_swap_test1() {

    //
    // r2_ffl1 and r2_ffl2 use dbg_memory_resource2, 
    // compare of PMR for these containers returns true
    // r1_ffl1 uses dbg_memory_resource1
    // compare of PMR for with 2 containers above 
    // returns true
    //

    iffl::debug_memory_resource dbg_memory_resource1;
    iffl::pmr_flat_forward_list<FLAT_FORWARD_LIST_TEST> r1_ffl1{ &dbg_memory_resource1 };
    fill_container_with_data(r1_ffl1);

    iffl::pmr_flat_forward_list<FLAT_FORWARD_LIST_TEST> r1_ffl_empty{ &dbg_memory_resource1 };

    iffl::debug_memory_resource dbg_memory_resource2;
    iffl::pmr_flat_forward_list<FLAT_FORWARD_LIST_TEST> r2_ffl1{ &dbg_memory_resource2 };
    fill_container_with_data(r2_ffl1);

    iffl::pmr_flat_forward_list<FLAT_FORWARD_LIST_TEST> r2_ffl2{ &dbg_memory_resource2 };
    fill_container_with_data(r2_ffl2);

    iffl::pmr_flat_forward_list<FLAT_FORWARD_LIST_TEST> r2_ffl_empty{ &dbg_memory_resource2 };

    test_swap(r1_ffl1, r2_ffl1);
    test_swap(r2_ffl1, r2_ffl2);
    
    test_swap(r2_ffl_empty, r1_ffl_empty);
    test_swap(r1_ffl1, r1_ffl_empty);
    test_swap(r2_ffl1, r1_ffl_empty);
}

void flat_forward_list_swap_test2() {

    iffl::flat_forward_list<FLAT_FORWARD_LIST_TEST> ffl1;
    fill_container_with_data(ffl1);

    iffl::flat_forward_list<FLAT_FORWARD_LIST_TEST> ffl2;
    fill_container_with_data(ffl2);

    test_swap(ffl1, ffl2);
}

void flat_forward_list_detach_attach_test1() {

    iffl::debug_memory_resource dbg_memory_resource1;
    iffl::pmr_flat_forward_list<FLAT_FORWARD_LIST_TEST> ffl1{ &dbg_memory_resource1 };
    fill_container_with_data(ffl1);

    auto used_allocator = ffl1.get_allocator();
    iffl::buffer_ref buff{ ffl1.detach() };
    used_allocator.deallocate(buff.begin, buff.size());
}

void flat_forward_list_detach_attach_test2() {

    iffl::debug_memory_resource dbg_memory_resource1;
    iffl::pmr_flat_forward_list<FLAT_FORWARD_LIST_TEST> ffl1{ &dbg_memory_resource1 };
    fill_container_with_data(ffl1);

    iffl::pmr_flat_forward_list<FLAT_FORWARD_LIST_TEST> ffl2{ &dbg_memory_resource1 };

    iffl::buffer_ref buff{ ffl1.detach() };

    ffl2.attach(buff.begin, buff.last, buff.end);
}

void flat_forward_list_detach_attach_test3() {

    iffl::debug_memory_resource dbg_memory_resource1;
    iffl::pmr_flat_forward_list<FLAT_FORWARD_LIST_TEST> ffl1{ &dbg_memory_resource1 };
    fill_container_with_data(ffl1);

    size_t element_count = ffl1.size();

    iffl::pmr_flat_forward_list<FLAT_FORWARD_LIST_TEST> ffl2{ &dbg_memory_resource1 };

    iffl::buffer_ref buff{ ffl1.detach() };

    FFL_CODDING_ERROR_IF_NOT(ffl2.attach(buff.begin, buff.size()));

    FFL_CODDING_ERROR_IF_NOT(element_count == ffl2.size());
}

void flat_forward_list_resize_elements_test1() {

    iffl::debug_memory_resource dbg_memory_resource;
    iffl::pmr_flat_forward_list<FLAT_FORWARD_LIST_TEST> ffl{ &dbg_memory_resource };
    fill_container_with_data(ffl, 10);

    size_t const elements_count{ ffl.size() };

    size_t idx{ 1 };
    //
    // Initially all elements have variable sizes.
    // After that loop every element size is struct + 10 bytes
    //
    for (auto i = ffl.begin(); i != ffl.end(); ++i, ++idx) {
        i = ffl.element_resize(i,
                               sizeof(FLAT_FORWARD_LIST_TEST) + 10,
                               [idx](char *buffer,
                                     size_t old_element_size,
                                     size_t new_element_size) {
                                    FLAT_FORWARD_LIST_TEST &e = *reinterpret_cast<FLAT_FORWARD_LIST_TEST *>(buffer);
                                    FFL_CODDING_ERROR_IF(old_element_size < sizeof(FLAT_FORWARD_LIST_TEST));
                                    FFL_CODDING_ERROR_IF(new_element_size < sizeof(FLAT_FORWARD_LIST_TEST));
                                    FFL_CODDING_ERROR_IF_NOT(e.DataLength + sizeof(FLAT_FORWARD_LIST_TEST) <= old_element_size);
                                    FFL_CODDING_ERROR_IF_NOT(idx == e.Type);
                                    e.Type += 1000;
                                    e.DataLength = new_element_size - sizeof(FLAT_FORWARD_LIST_TEST);
                                });

        FFL_CODDING_ERROR_IF_NOT(elements_count == ffl.size());
    }

    ffl.fill_padding(0xe1);

    idx = 1;
    //
    // Increase each element size by 10
    //
    for (auto i = ffl.begin(); i != ffl.end(); ++i, ++idx) {
        i = ffl.element_resize(i,
                               sizeof(FLAT_FORWARD_LIST_TEST) + 20,
                               [](char *buffer,
                                  size_t old_element_size,
                                  size_t new_element_size) {
                                    FLAT_FORWARD_LIST_TEST &e = *reinterpret_cast<FLAT_FORWARD_LIST_TEST *>(buffer);
                                    FFL_CODDING_ERROR_IF(old_element_size < sizeof(FLAT_FORWARD_LIST_TEST));
                                    FFL_CODDING_ERROR_IF(new_element_size < sizeof(FLAT_FORWARD_LIST_TEST));
                                    FFL_CODDING_ERROR_IF_NOT(e.DataLength + sizeof(FLAT_FORWARD_LIST_TEST) <= old_element_size);
                                    e.Type += 1000;
                                    e.DataLength = new_element_size - sizeof(FLAT_FORWARD_LIST_TEST);
                                });

        FFL_CODDING_ERROR_IF_NOT(elements_count == ffl.size());
    }

    ffl.fill_padding(0xe2);
    //
    // Shrink buffer to fit to make sure on next iteration 
    // every resize would reallocate buffer
    //
    ffl.shrink_to_fit();

    ffl.fill_padding(0xe3);

    idx = 1;
    //
    // Increase each element size by 10
    // But set Data size to 0 so on next 
    // iteration we can shrink to fit
    //
    for (auto i = ffl.begin(); i != ffl.end(); ++i, ++idx) {
        i = ffl.element_add_size(i,
                                 sizeof(FLAT_FORWARD_LIST_TEST) + 30);

        FFL_CODDING_ERROR_IF_NOT(elements_count == ffl.size());
    }

    ffl.fill_padding(0xe5);

    idx = 1;
    //
    // Shrink to fit
    //
    for (auto i = ffl.begin(); i != ffl.end(); ++i, ++idx) {
        ffl.shrink_to_fit(i);
        FFL_CODDING_ERROR_IF_NOT(elements_count == ffl.size());
    }

    ffl.fill_padding(0xe6);

    //
    // Shrink buffer to fit to make sure on next iteration 
    // every resize would reallocate buffer
    //
    ffl.shrink_to_fit();

    idx = 1;
    //
    // Make all elements variable size again
    // with data size from 0 to 100
    //
    for (auto i = ffl.begin(); i != ffl.end(); ++i, ++idx) {
        i = ffl.element_add_size(i, idx);
        FFL_CODDING_ERROR_IF_NOT(elements_count == ffl.size());
    }

    ffl.fill_padding(0xe7);

    idx = 1;
    //
    // Setting size of every element to 0 will erase element
    //
    for (auto i = ffl.begin(); i != ffl.end(); ++idx) {
        i = ffl.element_resize(i,
                               0,
                               [](char *buffer,
                                  [[maybe_unused]] size_t old_element_size,
                                  [[maybe_unused]] size_t new_element_size) noexcept {
                                  [[maybe_unused]] FLAT_FORWARD_LIST_TEST const &e = *reinterpret_cast<FLAT_FORWARD_LIST_TEST *>(buffer);
                                    FFL_CRASH_APPLICATION();
                                });
    }

    ffl.fill_padding(0xe8);

    FFL_CODDING_ERROR_IF_NOT(0 == ffl.size());
    ffl.shrink_to_fit();

    ffl.fill_padding(0xe9);
}

void flat_forward_list_find_by_offset_test1() {
    iffl::debug_memory_resource dbg_memory_resource;
    iffl::pmr_flat_forward_list<FLAT_FORWARD_LIST_TEST> ffl{ &dbg_memory_resource };
    fill_container_with_data(ffl, 10);

    FFL_CODDING_ERROR_IF_NOT(10 <= ffl.size());

    auto const element0_it = ffl.cbegin();
    auto const element0_range = ffl.range(element0_it);
    FFL_CODDING_ERROR_IF(element0_range.begin() < 0 || element0_range.begin() >= element0_range.buffer_end);
    FFL_CODDING_ERROR_IF(true == ffl.contains(element0_it, element0_range.begin() - 1));
    FFL_CODDING_ERROR_IF(false == ffl.contains(element0_it, element0_range.begin()));
    FFL_CODDING_ERROR_IF(false == ffl.contains(element0_it, element0_range.begin() + 1));
    FFL_CODDING_ERROR_IF(false == ffl.contains(element0_it, element0_range.buffer_end - 1));
    FFL_CODDING_ERROR_IF(true  == ffl.contains(element0_it, element0_range.buffer_end));
    FFL_CODDING_ERROR_IF(true == ffl.contains(element0_it, element0_range.buffer_end_aligned() + 1));

    auto const element1_it = element0_it + 1;
    auto const element1_range = ffl.range(element1_it);
    FFL_CODDING_ERROR_IF(element1_range.begin() < 0 || element1_range.begin() >= element1_range.buffer_end);
    FFL_CODDING_ERROR_IF_NOT(element1_range.begin() == element0_range.buffer_end_aligned());
    FFL_CODDING_ERROR_IF(true == ffl.contains(element1_it, element1_range.begin() - 1));
    FFL_CODDING_ERROR_IF(false == ffl.contains(element1_it, element1_range.begin()));
    FFL_CODDING_ERROR_IF(false == ffl.contains(element1_it, element1_range.begin() + 1));
    FFL_CODDING_ERROR_IF(false == ffl.contains(element1_it, element1_range.buffer_end - 1));
    FFL_CODDING_ERROR_IF(true == ffl.contains(element1_it, element1_range.buffer_end));
    FFL_CODDING_ERROR_IF(true == ffl.contains(element1_it, element1_range.buffer_end + 1));

    auto const element2_it = element1_it + 1;
    auto const element2_range = ffl.range(element2_it);
    FFL_CODDING_ERROR_IF(element2_range.begin() < 0 || element2_range.begin() >= element2_range.buffer_end);
    FFL_CODDING_ERROR_IF_NOT(element2_range.begin() == element1_range.buffer_end_aligned());
    FFL_CODDING_ERROR_IF(true == ffl.contains(element2_it, element2_range.begin() - 1));
    FFL_CODDING_ERROR_IF(false == ffl.contains(element2_it, element2_range.begin()));
    FFL_CODDING_ERROR_IF(false == ffl.contains(element2_it, element2_range.begin() + 1));
    FFL_CODDING_ERROR_IF(false == ffl.contains(element2_it, element2_range.buffer_end - 1));
    FFL_CODDING_ERROR_IF(true == ffl.contains(element2_it, element2_range.buffer_end));
    FFL_CODDING_ERROR_IF(true == ffl.contains(element2_it, element2_range.buffer_end + 1));

    auto const element3_it = element2_it + 1;
    auto const element3_range = ffl.range(element3_it);
    FFL_CODDING_ERROR_IF(element3_range.begin() < 0 || element3_range.begin() >= element3_range.buffer_end);
    FFL_CODDING_ERROR_IF_NOT(element3_range.begin() == element2_range.buffer_end_aligned());
    FFL_CODDING_ERROR_IF(true == ffl.contains(element3_it, element3_range.begin() - 1));
    FFL_CODDING_ERROR_IF(false == ffl.contains(element3_it, element3_range.begin()));
    FFL_CODDING_ERROR_IF(false == ffl.contains(element3_it, element3_range.begin() + 1));
    FFL_CODDING_ERROR_IF(false == ffl.contains(element3_it, element3_range.buffer_end - 1));
    FFL_CODDING_ERROR_IF(true == ffl.contains(element3_it, element3_range.buffer_end));
    FFL_CODDING_ERROR_IF(true == ffl.contains(element3_it, element3_range.buffer_end + 1));

    auto const element4_it = element3_it + 1;
    auto const element4_range = ffl.range(element4_it);
    FFL_CODDING_ERROR_IF(element4_range.begin() < 0 || element4_range.begin() >= element4_range.buffer_end);
    FFL_CODDING_ERROR_IF_NOT(element4_range.begin() == element3_range.buffer_end_aligned());
    FFL_CODDING_ERROR_IF(true == ffl.contains(element4_it, element4_range.begin() - 1));
    FFL_CODDING_ERROR_IF(false == ffl.contains(element4_it, element4_range.begin()));
    FFL_CODDING_ERROR_IF(false == ffl.contains(element4_it, element4_range.begin() + 1));
    FFL_CODDING_ERROR_IF(false == ffl.contains(element4_it, element4_range.buffer_end - 1));
    FFL_CODDING_ERROR_IF(true == ffl.contains(element4_it, element4_range.buffer_end));
    FFL_CODDING_ERROR_IF(true == ffl.contains(element4_it, element4_range.buffer_end + 1));

    [[maybe_unused]] auto const last_it = ffl.last();
    auto const last_range = ffl.range(last_it);
    FFL_CODDING_ERROR_IF(last_range.begin() < 0 || last_range.begin() >= last_range.buffer_end);
    FFL_CODDING_ERROR_IF_NOT(element4_range.begin() <= last_range.buffer_end);
    FFL_CODDING_ERROR_IF(true == ffl.contains(last_it, last_range.begin() - 1));
    FFL_CODDING_ERROR_IF(false == ffl.contains(last_it, last_range.begin()));
    FFL_CODDING_ERROR_IF(false == ffl.contains(last_it, last_range.begin() + 1));
    FFL_CODDING_ERROR_IF(false == ffl.contains(last_it, last_range.buffer_end - 1));
    FFL_CODDING_ERROR_IF(true == ffl.contains(last_it, last_range.buffer_end));
    FFL_CODDING_ERROR_IF(true == ffl.contains(last_it, last_range.buffer_end + 1));


    auto const before_element1_start = ffl.find_element_before(element1_range.begin());
    FFL_CODDING_ERROR_IF_NOT(before_element1_start == element0_it);
    auto before_element1_end = ffl.find_element_before(element1_range.buffer_end);
    FFL_CODDING_ERROR_IF_NOT(before_element1_end == element1_it);
    before_element1_end = ffl.find_element_before(element1_range.buffer_end_aligned());
    FFL_CODDING_ERROR_IF_NOT(before_element1_end == element1_it);

    auto const at_element1_start = ffl.find_element_at(element1_range.begin());
    FFL_CODDING_ERROR_IF_NOT(at_element1_start == element1_it);
    auto const at_element1_end = ffl.find_element_at(element1_range.buffer_end);
    FFL_CODDING_ERROR_IF_NOT(at_element1_end == element2_it);

    auto const after_element1_start = ffl.find_element_after(element1_range.begin());
    FFL_CODDING_ERROR_IF_NOT(after_element1_start == element2_it);
    auto const after_element1_end = ffl.find_element_after(element1_range.buffer_end);
    FFL_CODDING_ERROR_IF_NOT(after_element1_end == element3_it);


    auto const before_element2_start = ffl.find_element_before(element2_range.begin());
    FFL_CODDING_ERROR_IF_NOT(before_element2_start == element1_it);
    auto const before_element2_end = ffl.find_element_before(element2_range.buffer_end);
    FFL_CODDING_ERROR_IF_NOT(before_element2_end == element2_it);

    auto const at_element2_start = ffl.find_element_at(element2_range.begin());
    FFL_CODDING_ERROR_IF_NOT(at_element2_start == element2_it);
    auto const at_element2_end = ffl.find_element_at(element2_range.buffer_end);
    FFL_CODDING_ERROR_IF_NOT(at_element2_end == element3_it);

    auto const after_element2_start = ffl.find_element_after(element2_range.begin());
    FFL_CODDING_ERROR_IF_NOT(after_element2_start == element3_it);
    auto const after_element2_end = ffl.find_element_after(element2_range.buffer_end);
    FFL_CODDING_ERROR_IF_NOT(after_element2_end == element4_it);


    auto const before_last_start = ffl.find_element_before(last_range.begin());
    FFL_CODDING_ERROR_IF_NOT(before_last_start != ffl.cend());
    auto const before_last_end = ffl.find_element_before(last_range.buffer_end);
    FFL_CODDING_ERROR_IF_NOT(before_last_end == last_it);

    auto const at_last_start = ffl.find_element_at(last_range.begin());
    FFL_CODDING_ERROR_IF_NOT(at_last_start == last_it);
    auto const at_last_end = ffl.find_element_at(last_range.buffer_end);
    FFL_CODDING_ERROR_IF_NOT(at_last_end == ffl.cend());

    auto const after_last_start = ffl.find_element_after(last_range.begin());
    FFL_CODDING_ERROR_IF_NOT(after_last_start == ffl.cend());
    auto const after_last_end = ffl.find_element_after(last_range.buffer_end);
    FFL_CODDING_ERROR_IF_NOT(after_last_end == ffl.cend());
}

void flat_forward_list_erase_range_test1() {
    iffl::debug_memory_resource dbg_memory_resource;
    iffl::pmr_flat_forward_list<FLAT_FORWARD_LIST_TEST> ffl{ &dbg_memory_resource };
    fill_container_with_data(ffl, 10);

    FFL_CODDING_ERROR_IF_NOT(10 <= ffl.size());

    size_t elements_count = ffl.size();
    size_t prev_elements_count = elements_count;

    auto const element0_it = ffl.begin();
    auto element1_it = element0_it + 1;
    auto const element2_it = element1_it + 1;
    [[maybe_unused]] auto const last_it = ffl.last();

    ffl.erase(element1_it, element2_it);

    elements_count = ffl.size();
    FFL_CODDING_ERROR_IF_NOT(elements_count == prev_elements_count - 1);
    prev_elements_count = elements_count;

    element1_it = element0_it + 1;
    auto const element3_it = element1_it + 2;

    ffl.erase(element1_it, element3_it);

    elements_count = ffl.size();
    FFL_CODDING_ERROR_IF_NOT(elements_count == prev_elements_count - 2);
    prev_elements_count = elements_count;

    ffl.erase(element0_it, ffl.end());
    FFL_CODDING_ERROR_IF_NOT(0 == ffl.size());
}

void flat_forward_list_erase_test1() {
    iffl::debug_memory_resource dbg_memory_resource;
    iffl::pmr_flat_forward_list<FLAT_FORWARD_LIST_TEST> ffl{ &dbg_memory_resource };
    fill_container_with_data(ffl, 10);

    FFL_CODDING_ERROR_IF_NOT(10 <= ffl.size());

    size_t elements_count = ffl.size();
    size_t prev_elements_count = elements_count;


    ffl.erase(ffl.begin());

    elements_count = ffl.size();
    FFL_CODDING_ERROR_IF_NOT(elements_count == prev_elements_count - 1);
    prev_elements_count = elements_count;


    ffl.erase(ffl.begin() + 1);

    elements_count = ffl.size();
    FFL_CODDING_ERROR_IF_NOT(elements_count == prev_elements_count - 1);
    prev_elements_count = elements_count;

    ffl.erase(ffl.begin() + 2);

    elements_count = ffl.size();
    FFL_CODDING_ERROR_IF_NOT(elements_count == prev_elements_count - 1);
    prev_elements_count = elements_count;

    ffl.erase(ffl.last());

    elements_count = ffl.size();
    FFL_CODDING_ERROR_IF_NOT(elements_count == prev_elements_count - 1);
    prev_elements_count = elements_count;

    ffl.erase_all_from(ffl.begin() + 1);

    elements_count = ffl.size();
    FFL_CODDING_ERROR_IF_NOT(elements_count == 1);
    prev_elements_count = elements_count;

    ffl.erase_all_from(ffl.begin());

    FFL_CODDING_ERROR_IF_NOT(ffl.empty());
}

void flat_forward_list_erase_after_half_closed_test1() {
    iffl::debug_memory_resource dbg_memory_resource;
    iffl::pmr_flat_forward_list<FLAT_FORWARD_LIST_TEST> ffl{ &dbg_memory_resource };
    fill_container_with_data(ffl, 10);

    FFL_CODDING_ERROR_IF_NOT(10 <= ffl.size());

    size_t elements_count = ffl.size();
    size_t prev_elements_count = elements_count;

    auto const element0_it = ffl.begin();
    auto const element1_it = element0_it + 1;
    auto const element2_it = element1_it + 1;
    [[maybe_unused]] auto const last_it = ffl.last();

    ffl.erase_after_half_closed(element0_it, element2_it);

    elements_count = ffl.size();
    FFL_CODDING_ERROR_IF_NOT(elements_count == prev_elements_count - 2);
    prev_elements_count = elements_count;

    ffl.erase_after_half_closed(ffl.begin(), ffl.end());

    elements_count = ffl.size();
    FFL_CODDING_ERROR_IF_NOT(elements_count == 1);
    prev_elements_count = elements_count;

    ffl.erase_all();
    FFL_CODDING_ERROR_IF_NOT(ffl.empty());
}

void flat_forward_list_erase_after_test1() {
    iffl::debug_memory_resource dbg_memory_resource;
    iffl::pmr_flat_forward_list<FLAT_FORWARD_LIST_TEST> ffl{ &dbg_memory_resource };
    fill_container_with_data(ffl, 10);

    FFL_CODDING_ERROR_IF_NOT(10 <= ffl.size());

    size_t elements_count = ffl.size();
    size_t prev_elements_count = elements_count;


    ffl.erase_after(ffl.begin());

    elements_count = ffl.size();
    FFL_CODDING_ERROR_IF_NOT(elements_count == prev_elements_count - 1);
    prev_elements_count = elements_count;


    ffl.erase_after(ffl.begin() + 1);

    elements_count = ffl.size();
    FFL_CODDING_ERROR_IF_NOT(elements_count == prev_elements_count - 1);
    prev_elements_count = elements_count;

    ffl.erase_after(ffl.begin() + 2);

    elements_count = ffl.size();
    FFL_CODDING_ERROR_IF_NOT(elements_count == prev_elements_count - 1);
    prev_elements_count = elements_count;

    ffl.clear();

    FFL_CODDING_ERROR_IF_NOT(ffl.empty());
}

void flat_forward_list_resize_buffer_test1() {
    iffl::debug_memory_resource dbg_memory_resource;
    iffl::pmr_flat_forward_list<FLAT_FORWARD_LIST_TEST> ffl{ &dbg_memory_resource };
    fill_container_with_data(ffl, 10);

    FFL_CODDING_ERROR_IF_NOT(10 <= ffl.size());

    size_t elements_count = ffl.size();
    size_t prev_elements_count = elements_count;

    ffl.resize_buffer(ffl.total_capacity() + 1);

    elements_count = ffl.size();
    FFL_CODDING_ERROR_IF_NOT(elements_count == prev_elements_count);
    prev_elements_count = elements_count;

    ffl.resize_buffer(ffl.total_capacity() - 1);

    elements_count = ffl.size();
    FFL_CODDING_ERROR_IF_NOT(elements_count == prev_elements_count);
    prev_elements_count = elements_count;

    auto element_range{ ffl.range(ffl.last()) };

    ffl.resize_buffer(element_range.buffer_end);

    elements_count = ffl.size();
    FFL_CODDING_ERROR_IF_NOT(elements_count == prev_elements_count);
    prev_elements_count = elements_count;

    ffl.resize_buffer(element_range.begin());

    elements_count = ffl.size();
    FFL_CODDING_ERROR_IF_NOT(elements_count == prev_elements_count - 1);
    prev_elements_count = elements_count;

    element_range = ffl.range(ffl.last());
    ffl.resize_buffer(element_range.buffer_end - 1);

    elements_count = ffl.size();
    FFL_CODDING_ERROR_IF_NOT(elements_count == prev_elements_count - 1);
    prev_elements_count = elements_count;

    element_range = ffl.range(ffl.last());
    ffl.resize_buffer(element_range.begin() + 1);

    elements_count = ffl.size();
    FFL_CODDING_ERROR_IF_NOT(elements_count == prev_elements_count - 1);
    prev_elements_count = elements_count;

    element_range = ffl.range(ffl.begin());
    ffl.resize_buffer(element_range.buffer_end);

    elements_count = ffl.size();
    FFL_CODDING_ERROR_IF_NOT(elements_count == 1);
    prev_elements_count = elements_count;

    element_range = ffl.range(ffl.begin());
    ffl.resize_buffer(element_range.begin());

    elements_count = ffl.size();
    FFL_CODDING_ERROR_IF_NOT(elements_count == 0);
    prev_elements_count = elements_count;
}

//
// Sorting will be done using Type field.
// All lists are valid
//
// 1,2,3,4
//
FLAT_FORWARD_LIST_TEST const list_ordered1_len_4[] = {
    {2 * sizeof(FLAT_FORWARD_LIST_TEST), 1, 1 * sizeof(FLAT_FORWARD_LIST_TEST)},
    {0xFFFFFFF1, 0xFFFFFFF2, 0xFFFFFFF3},
    {3 * sizeof(FLAT_FORWARD_LIST_TEST), 2, 2 * sizeof(FLAT_FORWARD_LIST_TEST)},
    {0xFFFFFFF1, 0xFFFFFFF2, 0xFFFFFFF3},
    {0xFFFFFFF4, 0xFFFFFFF5, 0xFFFFFFF6},
    {4 * sizeof(FLAT_FORWARD_LIST_TEST), 3, 3 * sizeof(FLAT_FORWARD_LIST_TEST)},
    {0xFFFFFFF1, 0xFFFFFFF2, 0xFFFFFFF3},
    {0xFFFFFFF4, 0xFFFFFFF5, 0xFFFFFFF6},
    {0xFFFFFFF7, 0xFFFFFFF8, 0xFFFFFFF9},
    {0, 4, 4 * sizeof(FLAT_FORWARD_LIST_TEST)},
    {0xFFFFFFF1, 0xFFFFFFF2, 0xFFFFFFF3},
    {0xFFFFFFF4, 0xFFFFFFF5, 0xFFFFFFF6},
    {0xFFFFFFF7, 0xFFFFFFF8, 0xFFFFFFF9},
    {0xFFFFFFFA, 0xFFFFFFFB, 0xFFFFFFFC},
};
//
// 2,4,3,1
//
FLAT_FORWARD_LIST_TEST const list_unordered1_len_4[] = {
    {2 * sizeof(FLAT_FORWARD_LIST_TEST), 2, 1 * sizeof(FLAT_FORWARD_LIST_TEST)},
    {0xFFFFFFF1, 0xFFFFFFF2, 0xFFFFFFF3},
    {3 * sizeof(FLAT_FORWARD_LIST_TEST), 4, 2 * sizeof(FLAT_FORWARD_LIST_TEST)},
    {0xFFFFFFF1, 0xFFFFFFF2, 0xFFFFFFF3},
    {0xFFFFFFF4, 0xFFFFFFF5, 0xFFFFFFF6},
    {4 * sizeof(FLAT_FORWARD_LIST_TEST), 3, 3 * sizeof(FLAT_FORWARD_LIST_TEST)},
    {0xFFFFFFF1, 0xFFFFFFF2, 0xFFFFFFF3},
    {0xFFFFFFF4, 0xFFFFFFF5, 0xFFFFFFF6},
    {0xFFFFFFF7, 0xFFFFFFF8, 0xFFFFFFF9},
    {0, 1, 4 * sizeof(FLAT_FORWARD_LIST_TEST)},
    {0xFFFFFFF1, 0xFFFFFFF2, 0xFFFFFFF3},
    {0xFFFFFFF4, 0xFFFFFFF5, 0xFFFFFFF6},
    {0xFFFFFFF7, 0xFFFFFFF8, 0xFFFFFFF9},
    {0xFFFFFFFA, 0xFFFFFFFB, 0xFFFFFFFC},
};
//
// 2,3,4,5
//
FLAT_FORWARD_LIST_TEST const list_ordered2_len_4[] = {
    {2 * sizeof(FLAT_FORWARD_LIST_TEST), 2, 1 * sizeof(FLAT_FORWARD_LIST_TEST)},
    {0xFFFFFFF1, 0xFFFFFFF2, 0xFFFFFFF3},
    {3 * sizeof(FLAT_FORWARD_LIST_TEST), 3, 2 * sizeof(FLAT_FORWARD_LIST_TEST)},
    {0xFFFFFFF1, 0xFFFFFFF2, 0xFFFFFFF3},
    {0xFFFFFFF4, 0xFFFFFFF5, 0xFFFFFFF6},
    {4 * sizeof(FLAT_FORWARD_LIST_TEST), 4, 3 * sizeof(FLAT_FORWARD_LIST_TEST)},
    {0xFFFFFFF1, 0xFFFFFFF2, 0xFFFFFFF3},
    {0xFFFFFFF4, 0xFFFFFFF5, 0xFFFFFFF6},
    {0xFFFFFFF7, 0xFFFFFFF8, 0xFFFFFFF9},
    {0, 5, 4 * sizeof(FLAT_FORWARD_LIST_TEST)},
    {0xFFFFFFF1, 0xFFFFFFF2, 0xFFFFFFF3},
    {0xFFFFFFF4, 0xFFFFFFF5, 0xFFFFFFF6},
    {0xFFFFFFF7, 0xFFFFFFF8, 0xFFFFFFF9},
    {0xFFFFFFFA, 0xFFFFFFFB, 0xFFFFFFFC},
};
//
// 3,5,2,4
//
FLAT_FORWARD_LIST_TEST const list_unordered2_len_4[] = {
    {2 * sizeof(FLAT_FORWARD_LIST_TEST), 3, 1 * sizeof(FLAT_FORWARD_LIST_TEST)},
    {0xFFFFFFF1, 0xFFFFFFF2, 0xFFFFFFF3},
    {3 * sizeof(FLAT_FORWARD_LIST_TEST), 5, 2 * sizeof(FLAT_FORWARD_LIST_TEST)},
    {0xFFFFFFF1, 0xFFFFFFF2, 0xFFFFFFF3},
    {0xFFFFFFF4, 0xFFFFFFF5, 0xFFFFFFF6},
    {4 * sizeof(FLAT_FORWARD_LIST_TEST), 2, 3 * sizeof(FLAT_FORWARD_LIST_TEST)},
    {0xFFFFFFF1, 0xFFFFFFF2, 0xFFFFFFF3},
    {0xFFFFFFF4, 0xFFFFFFF5, 0xFFFFFFF6},
    {0xFFFFFFF7, 0xFFFFFFF8, 0xFFFFFFF9},
    {0, 4, 4 * sizeof(FLAT_FORWARD_LIST_TEST)},
    {0xFFFFFFF1, 0xFFFFFFF2, 0xFFFFFFF3},
    {0xFFFFFFF4, 0xFFFFFFF5, 0xFFFFFFF6},
    {0xFFFFFFF7, 0xFFFFFFF8, 0xFFFFFFF9},
    {0xFFFFFFFA, 0xFFFFFFFB, 0xFFFFFFFC},
};
//
// 2,5
//
FLAT_FORWARD_LIST_TEST const list_ordered1_len_2[] = {
    {2 * sizeof(FLAT_FORWARD_LIST_TEST), 2, 1 * sizeof(FLAT_FORWARD_LIST_TEST)},
    {0xFFFFFFF1, 0xFFFFFFF2, 0xFFFFFFF3},
    {0, 5, 4 * sizeof(FLAT_FORWARD_LIST_TEST)},
    {0xFFFFFFF1, 0xFFFFFFF2, 0xFFFFFFF3},
    {0xFFFFFFF4, 0xFFFFFFF5, 0xFFFFFFF6},
    {0xFFFFFFF7, 0xFFFFFFF8, 0xFFFFFFF9},
    {0xFFFFFFFA, 0xFFFFFFFB, 0xFFFFFFFC},
};
//
// 5,2
//
FLAT_FORWARD_LIST_TEST const list_unordered1_len_2[] = {
    {2 * sizeof(FLAT_FORWARD_LIST_TEST), 5, 1 * sizeof(FLAT_FORWARD_LIST_TEST)},
    {0xFFFFFFF1, 0xFFFFFFF2, 0xFFFFFFF3},
    {0, 2, 4 * sizeof(FLAT_FORWARD_LIST_TEST)},
    {0xFFFFFFF1, 0xFFFFFFF2, 0xFFFFFFF3},
    {0xFFFFFFF4, 0xFFFFFFF5, 0xFFFFFFF6},
    {0xFFFFFFF7, 0xFFFFFFF8, 0xFFFFFFF9},
    {0xFFFFFFFA, 0xFFFFFFFB, 0xFFFFFFFC},
};

void print_flat_forward_list(char const * title, 
                             iffl::pmr_flat_forward_list<FLAT_FORWARD_LIST_TEST> const &container) {
    std::printf("-----\"%s\"-----\n", title);
    std::for_each(container.begin(), container.end(), 
                  [](FLAT_FORWARD_LIST_TEST const &e) noexcept {
                      print_element(e);
                  });
}

void flat_forward_list_sort_test1() {

    auto const elements_less{ [](FLAT_FORWARD_LIST_TEST const &lhs,
                               FLAT_FORWARD_LIST_TEST const &rhs) noexcept -> bool {
                                return lhs.Type == rhs.Type ? lhs.DataLength < rhs.DataLength
                                                            : lhs.Type < rhs.Type;
                            } };

    iffl::debug_memory_resource dbg_memory_resource;
    iffl::pmr_flat_forward_list<FLAT_FORWARD_LIST_TEST> unordered_ffl1{ reinterpret_cast<char const *>(list_unordered1_len_4),
                                                                        sizeof(list_unordered1_len_4),
                                                                        &dbg_memory_resource };

    print_flat_forward_list("flat_forward_list_sort_test1-unordered-ffl1", unordered_ffl1);

    unordered_ffl1.sort(elements_less);

    print_flat_forward_list("flat_forward_list_sort_test1-ordered-ffl1", unordered_ffl1);

    iffl::pmr_flat_forward_list<FLAT_FORWARD_LIST_TEST> unordered_ffl2{ reinterpret_cast<char const *>(list_unordered2_len_4),
                                                                        sizeof(list_unordered2_len_4),
                                                                        &dbg_memory_resource };

    print_flat_forward_list("flat_forward_list_sort_test1-unordered-ffl2", unordered_ffl2);

    unordered_ffl2.sort(elements_less);

    print_flat_forward_list("flat_forward_list_sort_test1-ordered-ffl2", unordered_ffl2);

    iffl::pmr_flat_forward_list<FLAT_FORWARD_LIST_TEST> ffl_merge{ unordered_ffl1 };

    ffl_merge.merge(unordered_ffl2, elements_less);

    print_flat_forward_list("flat_forward_list_sort_test1-ordered-merged", ffl_merge);

    ffl_merge.remove_if([](FLAT_FORWARD_LIST_TEST const &lhs) -> bool {
                            return lhs.Type == 2;
                       });

    print_flat_forward_list("flat_forward_list_sort_test1-removed_all-2", ffl_merge);

    ffl_merge.unique([](FLAT_FORWARD_LIST_TEST const &lhs,
                        FLAT_FORWARD_LIST_TEST const &rhs) -> bool {
                            return lhs.Type == rhs.Type;
                     });

    print_flat_forward_list("flat_forward_list_sort_test1-ordered-merged-unique", ffl_merge);
}

void flat_forward_list_traits_traits_test1() noexcept {
    using test_traits = iffl::flat_forward_list_traits<FLAT_FORWARD_LIST_TEST>;
    using test_traits_traits = iffl::flat_forward_list_traits_traits<test_traits>;
    
    static_assert (test_traits_traits::has_minimum_size_v, 
                   "FLAT_FORWARD_LIST_TEST must have minimum_size");

    static_assert (test_traits_traits::has_get_size_v, 
                   "FLAT_FORWARD_LIST_TEST must have get_size");

    static_assert (test_traits_traits::has_next_offset_v, 
                   "FLAT_FORWARD_LIST_TEST must have get_next_offset");

    static_assert (test_traits_traits::can_set_next_offset_v, 
                   "FLAT_FORWARD_LIST_TEST must have set_next_element_offset");

    static_assert (test_traits_traits::can_validate_v, 
                   "FLAT_FORWARD_LIST_TEST must have validate");

    test_traits_traits::print_traits_info();
}


void run_all_flat_forward_list_tests() {
    flat_forward_list_traits_traits_test1();
    flat_forward_list_validate_test1();
    flat_forward_list_iterator_test1();
    flat_forward_list_push_back_test1();
    flat_forward_list_push_front_test1();
    flat_forward_list_swap_test1();
    flat_forward_list_swap_test2();
    flat_forward_list_detach_attach_test1();
    flat_forward_list_detach_attach_test2();
    flat_forward_list_detach_attach_test3();
    flat_forward_list_resize_elements_test1();
    flat_forward_list_find_by_offset_test1();
    flat_forward_list_erase_range_test1();
    flat_forward_list_erase_test1();
    flat_forward_list_erase_after_half_closed_test1();
    flat_forward_list_erase_after_test1();
    flat_forward_list_resize_buffer_test1();
    flat_forward_list_sort_test1();
}
