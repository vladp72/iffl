#include "iffl_test_cases.h"
#include "iffl_ea_usecase.h"
#include "iffl_c_api_usecase1.h"
#include "iffl_c_api_usecase2.h"

#include <cstdio>

int main() {

    //std::vector<int> foo;

    std::printf("\n--------- Starting tests ----------\n\n");
    run_all_flat_forward_list_tests();
    std::printf("\n------ Starting EA usecase --------\n\n");
    run_ffl_ea_usecase();
    std::printf("\n----- Starting C API usecase 1-----\n\n");
    run_ffl_c_api_usecase1();
    std::printf("\n----- Starting C API usecase 2-----\n\n");
    run_ffl_c_api_usecase2();
    std::printf("\n--------- Tests Complete ----------\n\n");
    return 0;
}
