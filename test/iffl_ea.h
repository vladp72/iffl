#pragma once 
#include "iffl.h"
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
// - How to write a server that receives a buffer with a list
//   of these structures. 
//
//   Function handle_ea1 demonstrates how to validate and iterate in one
//   loop
//
//   Function handle_ea2 demonstrates how to first validate and then 
//   iterate over trusted buffer
//   
//

#if !defined(__GNUC__) && !defined(__clang__)
#include <windows.h>
#endif

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
        // align entries as they are inserted in container.
        // Container still would support lists that are not properly
        // aligned, but when inserting new elements it will make sure
        // that elements are padded to guarantee that next entry
        // is aligned, assuming current entry is aligned.
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
        // This method is required for validate algorithm
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
            // Note that when we evaluate if buffer is valid we are not requiring
            // that it is properly aligned. An implementation can choose to enforce 
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
            // so it is not strictly speaking necessary here.
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

//
// Helper function that prints EA information
//
inline void print_ea(size_t idx, 
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

