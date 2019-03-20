#include <iffl.h>
#include "iffl_ea.h"        
#include "iffl_ea_usecase.h"
//
// This use case demonstrates 
//
// - What specialization of flat_forward_list_traits for FILE_FULL_EA_INFORMATION
//   should look like.
//
// - How you can utilize flat_forward_list to interact with
//   an API that require a flat forward list as 
//   an input. 
//   Caller fills buffer with FILE_FULL_EA_INFORMATION entries.
//   See prepare_ea_and_call_handler
//
// - How to write a server that recieves a buffer with a list
//   of these structures. 
//
//   Fnction handle_ea1 demonstrates how to validate and iterate in one
//   loop
//
//   Fnction handle_ea2 demonstrates how to first validate and then 
//   iterate over trusted buffer
//   
//
char const ea_name0[] = "TEST_EA_0";
//char const ea_data1[]; no data
char const ea_name1[] = "TEST_EA_1";
char const ea_data1[] = {1,2,3};
char const ea_name2[] = "TEST_EA_2";
char const ea_data2[] = { 1,2,3,4,5,6,7,8,9,0xa,0xb,0xc,0xd,0xf };

//
// Validates and handle elements in one loop.
// Handling in this case simply prints element content
//
void handle_ea1(char const *buffer, size_t buffer_lenght) {

    std::printf("-----\"handle_ea1\"-----\n");

    size_t idx{ 0 };
    char const *failed_validation{ nullptr };
    size_t invalid_element_length{ 0 };
    //
    // Validate and iterate in one loop
    //
    auto[is_valid, buffer_view] =
        iffl::flat_forward_list_validate<FILE_FULL_EA_INFORMATION>(
            buffer,
            buffer + buffer_lenght,
            [buffer, &idx, &failed_validation, &invalid_element_length] (size_t buffer_size,
                                                                         FILE_FULL_EA_INFORMATION const &e) -> bool {
                //
                // validate element
                //
                bool const is_valid{ iffl::flat_forward_list_traits<FILE_FULL_EA_INFORMATION>::validate(buffer_size, e) };
                //
                // if element is valid then process element
                //
                if (is_valid) {
                    print_ea(idx, reinterpret_cast<char const *>(&e) - buffer, e);
                    ++idx;
                } else {
                    //
                    // Safe additional information about element that failed
                    // validation
                    //
                    invalid_element_length = buffer_size;
                    failed_validation = buffer;
                }
                //
                // validation loop is aborted if element is not valid
                //
                return is_valid;
            });

    std::printf("\n");
    std::printf("valid                            : %s\n", is_valid ? "yes" : "no");
    std::printf("found elements                   : %zi\n", idx);
    std::printf("last valid element               : 0x%p\n", iffl::cast_to_void_ptr(buffer_view.last().get_ptr()));
    std::printf("element failed validation        : 0x%p\n", iffl::cast_to_void_ptr(failed_validation));
    std::printf("element failed validation length : %zi\n", invalid_element_length);
    std::printf("\n");
}

//
// Validates and handle elements in one loop.
// Handling in this case simply prints element content
//
void handle_ea2(char const *buffer, size_t buffer_lenght) {

    std::printf("-----\"handle_ea2\"-----\n");

    size_t idx{ 0 };
    //
    // First validate buffer
    //
    auto[is_valid, buffer_view] =
        iffl::flat_forward_list_validate<FILE_FULL_EA_INFORMATION>( buffer, buffer + buffer_lenght);
    //
    // Using iterators go over elements of trusted buffer
    //
    if (is_valid) {
        std::for_each(buffer_view.begin(), 
                      buffer_view.end(),
                      [buffer, &idx](FILE_FULL_EA_INFORMATION const &e) {
                          print_ea(idx, reinterpret_cast<char const *>(&e) - buffer, e);
                          ++idx;
                      });
    }

    std::printf("\n");
    std::printf("valid                            : %s\n", is_valid ? "yes" : "no");
    std::printf("found elements                   : %zi\n", idx);
    std::printf("last valid element               : 0x%p\n", iffl::cast_to_void_ptr(buffer_view.last().get_ptr()));
    std::printf("\n");
}

//
// Creates extended attribute list with 3 entries and 
// calls handle_ea1 and handle_ea2
//
void prepare_ea_and_call_handler() {

    ea_iffl eas;

    eas.emplace_front(FFL_SIZE_THROUGH_FIELD(FILE_FULL_EA_INFORMATION, EaValueLength) 
                     + sizeof(ea_name0)-1, 
                     [](FILE_FULL_EA_INFORMATION &e,
                        size_t new_element_size) noexcept {
                        e.Flags = 0;
                        e.EaNameLength = sizeof(ea_name0)-1;
                        e.EaValueLength = 0;
                        iffl::copy_data(e.EaName,
                                        ea_name0,
                                        sizeof(ea_name0)-1);
                      });

    eas.emplace_back(FFL_SIZE_THROUGH_FIELD(FILE_FULL_EA_INFORMATION, EaValueLength) 
                     + sizeof(ea_name1)-1
                     + sizeof(ea_data1),
                     [](FILE_FULL_EA_INFORMATION &e,
                        size_t new_element_size) noexcept {
                        e.Flags = 1;
                        e.EaNameLength = sizeof(ea_name1)-1;
                        e.EaValueLength = sizeof(ea_data1);
                        iffl::copy_data(e.EaName,
                                        ea_name1,
                                        sizeof(ea_name1)-1);
                        iffl::copy_data(e.EaName + sizeof(ea_name1)-1,
                                        ea_data1,
                                        sizeof(ea_data1));
                      });

    eas.emplace_front(FFL_SIZE_THROUGH_FIELD(FILE_FULL_EA_INFORMATION, EaValueLength) 
                     + sizeof(ea_name2)-1
                     + sizeof(ea_data2),
                     [](FILE_FULL_EA_INFORMATION &e,
                        size_t new_element_size) noexcept {
                        e.Flags = 2;
                        e.EaNameLength = sizeof(ea_name2)-1;
                        e.EaValueLength = sizeof(ea_data2);
                        iffl::copy_data(e.EaName,
                                        ea_name2,
                                        sizeof(ea_name2)-1);
                        iffl::copy_data(e.EaName + sizeof(ea_name2)-1,
                                        ea_data2,
                                        sizeof(ea_data2));
                      });

    handle_ea1(eas.data(), eas.used_capacity());
    handle_ea2(eas.data(), eas.used_capacity());

}

void run_ffl_ea_usecase() {
    iffl::flat_forward_list_traits_traits<FILE_FULL_EA_INFORMATION>::print_traits_info();
    prepare_ea_and_call_handler();
}
