#include <windows.h>

#include "iffl.h"
#include "iffl_ea_usecase.h"


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
        // This is the only method required by flat_forward_list_iterator.
        //
        constexpr static size_t get_next_element_offset(char const *buffer) noexcept {
            FILE_FULL_EA_INFORMATION const &e = *reinterpret_cast<FILE_FULL_EA_INFORMATION const *>(buffer);
            return e.NextEntryOffset;
        }
        //
        // This method is requiered for validate algorithm
        //
        constexpr static size_t minimum_size() noexcept {
            return FFL_SIZE_THROUGH_FIELD(FILE_FULL_EA_INFORMATION, EaValueLength);
        }
        //
        // Helper method that calculates buffer size
        // Not required.
        // Is used by validate below
        //
        constexpr static size_t get_size(FILE_FULL_EA_INFORMATION const &e) {
            return  FFL_SIZE_THROUGH_FIELD(FILE_FULL_EA_INFORMATION, EaValueLength) +
                    e.EaNameLength +
                    e.EaValueLength;
        }
        constexpr static size_t get_size(char const *buffer) {
            return get_size(*reinterpret_cast<FILE_FULL_EA_INFORMATION const *>(buffer));
        }
        //
        // This method is required for validate algorithm
        //
        constexpr static bool validate(size_t buffer_size, char const *buffer) noexcept {
            FILE_FULL_EA_INFORMATION const &e = *reinterpret_cast<FILE_FULL_EA_INFORMATION const *>(buffer);
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
        //
        // This method is required by container
        //
        constexpr static void set_next_element_offset(char *buffer, size_t size) noexcept {
            FILE_FULL_EA_INFORMATION &e = *reinterpret_cast<FILE_FULL_EA_INFORMATION *>(buffer);
            FFL_CODDING_ERROR_IF_NOT(size == 0 ||
                                     size >= get_size(e));
            e.NextEntryOffset = static_cast<ULONG>(size);
        }
        //
        // This method is required by container
        //
        constexpr static size_t calculate_next_element_offset(char const *buffer) noexcept {
            //
            // When calculating offset we use size padded 
            // to guarantee alignment
            //
            return get_size(buffer);
        }

    };
}

using ea_iffl = iffl::flat_forward_list<FILE_FULL_EA_INFORMATION>;

char const ea_name0[] = "TEST_EA_0";
//char const ea_data1[]; no data
char const ea_name1[] = "TEST_EA_1";
char const ea_data1[] = {1,2,3};
char const ea_name2[] = "TEST_EA_2";
char const ea_data2[] = { 1,2,3,4,5,6,7,8,9,0xa,0xb,0xc,0xd,0xf };

//
// Validates adn prints extended attribute list
//
void handle_ea(char const *buffer, size_t buffer_lenght) {
    size_t idx{ 0 };
    char const *failed_validation{ nullptr };
    size_t invalid_element_length{ 0 };

    auto[is_valid, last_valid] =
        iffl::flat_forward_list_validate<FILE_FULL_EA_INFORMATION>(
            buffer,
            buffer + buffer_lenght,
            [&idx, &failed_validation, &invalid_element_length] (size_t buffer_size,
                                                               char const *buffer) -> bool {
                bool is_valid = iffl::flat_forward_list_traits<FILE_FULL_EA_INFORMATION>::validate(buffer_size, buffer);
                if (is_valid) {
                    FILE_FULL_EA_INFORMATION const &e = 
                        *reinterpret_cast<FILE_FULL_EA_INFORMATION const *>(buffer);
                    printf("FILE_FULL_EA_INFORMATION[%zi].NextEntryOffset = %u\n", 
                           idx,
                           e.NextEntryOffset);
                    printf("FILE_FULL_EA_INFORMATION[%zi].Flags = %u\n", 
                           idx,
                           static_cast<int>(e.Flags));
                    printf("FILE_FULL_EA_INFORMATION[%zi].EaNameLength = %u \"%s\"\n", 
                           idx,
                           static_cast<int>(e.EaNameLength),
                           e.EaNameLength ? e.EaName : "");
                    printf("FILE_FULL_EA_INFORMATION[%zi].EaValueLength = %hu\n", 
                           idx,
                           e.EaValueLength);
                    if (e.EaValueLength) {
                        char const * data_first = e.EaName + e.EaNameLength;
                        char const * const data_end = e.EaName + e.EaNameLength + e.EaValueLength;
                        for (; data_first != data_end; ++data_first) {
                            printf("%x", static_cast<int>(*data_first));
                        }
                        printf("\n");
                    }
                } else {
                    invalid_element_length = buffer_size;
                    failed_validation = buffer;
                }
                return is_valid;
            });

    printf("\n");
    printf("valid                            : %s\n", is_valid ? "yes" : "no");
    printf("found elements                   : %zi\n", idx);
    printf("last valid element               : %p\n", last_valid);
    printf("element failed validation        : %p\n", failed_validation);
    printf("element failed validation length : %zi\n", invalid_element_length);
    printf("\n");
}

//
// Creates extended attribute list with 3 entries and 
// calls handle_ea
//
void prepare_ea_and_call_handler() {

    ea_iffl eas;

    eas.emplace_front(FFL_SIZE_THROUGH_FIELD(FILE_FULL_EA_INFORMATION, EaValueLength) 
                     + sizeof(ea_name0), 
                     [](char *buffer,
                        size_t new_element_size) {
                        FILE_FULL_EA_INFORMATION &e = *reinterpret_cast<FILE_FULL_EA_INFORMATION *>(buffer);
                        e.Flags = 0;
                        e.EaNameLength = sizeof(ea_name0);
                        e.EaValueLength = 0;
                        iffl::copy_data(e.EaName,
                                        ea_name0,
                                        sizeof(ea_name0));
                      });

    eas.emplace_back(FFL_SIZE_THROUGH_FIELD(FILE_FULL_EA_INFORMATION, EaValueLength) 
                     + sizeof(ea_name1)
                     + sizeof(ea_data1),
                     [](char *buffer,
                        size_t new_element_size) {
                        FILE_FULL_EA_INFORMATION &e = *reinterpret_cast<FILE_FULL_EA_INFORMATION *>(buffer);
                        e.Flags = 1;
                        e.EaNameLength = sizeof(ea_name1);
                        e.EaValueLength = sizeof(ea_data1);
                        iffl::copy_data(e.EaName,
                                        ea_name1,
                                        sizeof(ea_name1));
                        iffl::copy_data(e.EaName + sizeof(ea_name1),
                                        ea_data1,
                                        sizeof(ea_data1));
                      });

    eas.emplace_front(FFL_SIZE_THROUGH_FIELD(FILE_FULL_EA_INFORMATION, EaValueLength) 
                     + sizeof(ea_name2)
                     + sizeof(ea_data2),
                     [](char *buffer,
                        size_t new_element_size) {
                        FILE_FULL_EA_INFORMATION &e = *reinterpret_cast<FILE_FULL_EA_INFORMATION *>(buffer);
                        e.Flags = 2;
                        e.EaNameLength = sizeof(ea_name2);
                        e.EaValueLength = sizeof(ea_data2);
                        iffl::copy_data(e.EaName,
                                        ea_name2,
                                        sizeof(ea_name2));
                        iffl::copy_data(e.EaName + sizeof(ea_name2),
                                        ea_data2,
                                        sizeof(ea_data2));
                      });

    handle_ea(eas.data(), eas.used_capacity());

}

void run_ffl_ea_usecase() {
    prepare_ea_and_call_handler();
}
