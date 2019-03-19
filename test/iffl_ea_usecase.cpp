//
// This use case demonstrates 
//
// - What specialization of flat_forward_list_traits for FILE_FULL_EA_INFORMATION
//   should look like.
//
// - How you can utilize flat_forward_list to interact with
//   an API that require something like flat forward list as 
//   and input. 
//   Caller fill in buffer with flat list of FILE_FULL_EA_INFORMATION.
//   See prepare_ea_and_call_handler
//
// - And how to write a server that recieves a buffer with a list
//   of these structures, and needs to validate and safely iterate 
//   over this list. 
//   Fnction handle_ea1 demonstrates how to validate and iterate in one
//   loop
//   Fnction handle_ea2 demonstrates how to first validate and then 
//   iterate over trusted buffer
//   
//

#if !defined(__GNUC__) && !defined(__clang__)
#include <windows.h>
#endif

#include "iffl.h"
#include "iffl_ea_usecase.h"

#if !defined(_WINDOWS_)
using ULONG = unsigned int;
using USHORT = unsigned short;
using UCHAR = unsigned char;
using CHAR = char;
#endif


typedef struct _FILE_FULL_EA_INFORMATION {
    ULONG  NextEntryOffset;
    UCHAR  Flags;
    UCHAR  EaNameLength;
    USHORT EaValueLength;
    CHAR   EaName[1];
} FILE_FULL_EA_INFORMATION, *PFILE_FULL_EA_INFORMATION;

namespace iffl {
    template <>
    struct flat_forward_list_traits<FILE_FULL_EA_INFORMATION> {

        //
        // By default entries in container are not aligned.
        // By providing this member we are telling container to
        // align entries as they are inserved in container.
        // Container still would support lists that are not propertly
        // aligned, but when inserting new elements it will make sure
        // that elements are padded to guarantee that next entry
        // is alligned, assuming current entry is alligned.
        // For example we have following array
        //
        // element1 - alignment correct, 
        //            but not correctly padded at the end
        // element2 - is not aligned correctly because element1 is not correctly padded,
        //            but is correctly padded
        //
        // If we insert element 1.5 between 1 and 2 we will make sure it is correctly padded
        // but because 1 is not correctly padded, element 1.5 will not be correctly aligned.
        // 
        // Padding can be fixed for all elements in the container using shrink_to_fit.
        //

        constexpr static size_t const alignment{ alignof(FILE_FULL_EA_INFORMATION) };
        //
        // This is the only method required by flat_forward_list_iterator.
        //
        static size_t get_next_offset(FILE_FULL_EA_INFORMATION const &e) noexcept {
            return e.NextEntryOffset;
        }
        //
        // This method is requiered for validate algorithm
        //
        static size_t minimum_size() noexcept {
            return FFL_SIZE_THROUGH_FIELD(FILE_FULL_EA_INFORMATION, EaValueLength);
        }
        //
        // Helper method that calculates buffer size
        // Not required.
        // Is used by validate below
        //
        static size_t get_size(FILE_FULL_EA_INFORMATION const &e) {
            return  FFL_SIZE_THROUGH_FIELD(FILE_FULL_EA_INFORMATION, EaValueLength) +
                    e.EaNameLength +
                    e.EaValueLength;
        }
        //
        // This method is required for validate algorithm
        //
        static bool validate_size(FILE_FULL_EA_INFORMATION const &e, size_t buffer_size) noexcept {
            //
            // if this is last element then make sure it fits in the reminder of buffer
            // otherwise make sure that pointer to the next element fits reminder of buffer
            // and this element size fits before next element starts
            //
            // Note that when we evaluate if buffer is valid we are not requiering
            // that it is propertly alligned. An implementation can choose to enforce 
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

        static bool validate_data(FILE_FULL_EA_INFORMATION const &e) noexcept {
            //
            // This is an example of a validation you might want to do.
            // Extended attribute name does not have to be 0 terminated 
            // so it is not strictly speaking nessesary here.
            //
            // char const * const end = e.EaName + e.EaNameLength;
            // return end != std::find_if(e.EaName,
            //                            end, 
            //                            [](char c) -> bool {
            //                                 return c == '\0'; 
            //                            });
            return true;
        }
        static bool validate(size_t buffer_size, FILE_FULL_EA_INFORMATION const &e) noexcept {
            return validate_size(e, buffer_size) && validate_data(e);
        }
        //
        // This method is required by container
        //
        static void set_next_offset(FILE_FULL_EA_INFORMATION &e, size_t size) noexcept {
            FFL_CODDING_ERROR_IF_NOT(size == 0 ||
                                     size >= get_size(e));
            e.NextEntryOffset = static_cast<ULONG>(size);
        }
    };
}

using ea_iffl = iffl::flat_forward_list<FILE_FULL_EA_INFORMATION>;
using ea_iffl_iterator = iffl::flat_forward_list_iterator<FILE_FULL_EA_INFORMATION>;
using ea_iffl_const_iterator = iffl::flat_forward_list_const_iterator<FILE_FULL_EA_INFORMATION>;

char const ea_name0[] = "TEST_EA_0";
//char const ea_data1[]; no data
char const ea_name1[] = "TEST_EA_1";
char const ea_data1[] = {1,2,3};
char const ea_name2[] = "TEST_EA_2";
char const ea_data2[] = { 1,2,3,4,5,6,7,8,9,0xa,0xb,0xc,0xd,0xf };

//
// Helper function that prints EA information
//
void print_ea(size_t idx, 
              size_t offset,
              FILE_FULL_EA_INFORMATION const &e) {
    std::printf("FILE_FULL_EA_INFORMATION[%zi] @ = 0x%p, buffer offset %zi\n",
           idx,
           iffl::cast_to_void_ptr(&e),
           offset);
    std::printf("FILE_FULL_EA_INFORMATION[%zi].NextEntryOffset = %u\n",
           idx,
           e.NextEntryOffset);
    std::printf("FILE_FULL_EA_INFORMATION[%zi].Flags = %i\n",
           idx,
           static_cast<int>(e.Flags));
    std::printf("FILE_FULL_EA_INFORMATION[%zi].EaNameLength = %i \"%s\"\n",
           idx,
           static_cast<int>(e.EaNameLength),
           (e.EaNameLength ? std::string{ e.EaName, e.EaName + e.EaNameLength }
    : std::string{}).c_str());
    std::printf("FILE_FULL_EA_INFORMATION[%zi].EaValueLength = %hu\n",
           idx,
           e.EaValueLength);
    if (e.EaValueLength) {
        char const * data_first = e.EaName + e.EaNameLength;
        char const * const data_end = e.EaName + e.EaNameLength + e.EaValueLength;
        for (; data_first != data_end; ++data_first) {
            std::printf("%x", static_cast<int>(*data_first));
        }
        std::printf("\n");
    }
}

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
