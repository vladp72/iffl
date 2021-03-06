﻿# CMakeList.txt : 
# based on https://rix0r.nl/blog/2015/08/13/cmake-guide/ 
# also see 
#     https://www.slideshare.net/DanielPfeifer1/cmake-48475415
#     https://cliutils.gitlab.io/modern-cmake/
#     https://codingnest.com/basic-cmake/
#     https://codingnest.com/basic-cmake-part-2/
#     https://pabloariasal.github.io/2018/02/19/its-time-to-do-cmake-right/
# CppCon 2017 Effective CMake
#     https://www.youtube.com/watch?v=bsXLMQ6WgIk
#

#
# We will follow Modern CMake guidelines
#
cmake_minimum_required ( VERSION 3.11 )

#
# Load library version from the header file
#
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/include/iffl_config.h" IFFL_CONFIG_FILE)
string(REGEX MATCH "IFFL_VERSION_MAJOR ([0-9]+)" _ "${IFFL_CONFIG_FILE}")
set(IFFL_VERSION_MAJOR ${CMAKE_MATCH_1})
string(REGEX MATCH "IFFL_VERSION_MINOR ([0-9]+)" _ "${IFFL_CONFIG_FILE}")
set(IFFL_VERSION_MINOR ${CMAKE_MATCH_1})
string(REGEX MATCH "IFFL_VERSION_PATCH ([0-9]+)" _ "${IFFL_CONFIG_FILE}")
set(IFFL_VERSION_PATCH ${CMAKE_MATCH_1})
message("IFFL version from include/iffl_config.h is ${IFFL_VERSION_MAJOR}.${IFFL_VERSION_MINOR}.${IFFL_VERSION_PATCH}")

#
# iffl is a C++ project
#
project ("iffl" VERSION ${IFFL_VERSION_MAJOR}.${IFFL_VERSION_MINOR}.${IFFL_VERSION_PATCH} LANGUAGES CXX )

#
# Suppress CMake Warning: Manually-specified variables were not used by the project, CMAKE_C_COMPILER
# Most solutions are suggesting to use --no-warn-unused-cli instead of touching variable,
# but at least for now I like touching better because I do not have to look up help on Internet
# name of that parameter every time I forget it.
#
message(STATUS "Suppressing stupid warning that CMAKE_C_COMPILER is not used. Here I am using it! Happy now?: ${CMAKE_C_COMPILER}.")

#
# Not yet sure what that really does
#
enable_testing( )

set(VERSION_MAJOR ${IFFL_VERSION_MAJOR} CACHE STRING "Project major version number.")
set(VERSION_MINOR ${IFFL_VERSION_MINOR} CACHE STRING "Project minor version number.")
set(VERSION_PATCH ${IFFL_VERSION_PATCH} CACHE STRING "Project patch version number.")
mark_as_advanced(VERSION_MAJOR VERSION_MINOR VERSION_PATCH)

#
# This is a static library
#
add_library ( iffl 
              INTERFACE )

#
# If we have compiler requirements for this library, list them
# here
# https://cmake.org/cmake/help/v3.8/prop_gbl/CMAKE_CXX_KNOWN_FEATURES.html#prop_gbl:CMAKE_CXX_KNOWN_FEATURES 
#
target_compile_features ( iffl
                          INTERFACE
                              cxx_std_17
                              cxx_auto_type
                              cxx_final
                              cxx_deleted_functions
                              cxx_defaulted_functions
                              cxx_attributes
                              cxx_variadic_templates )

#
# Define headers for this library. PUBLIC headers are used for
# compiling the library, and will be added to consumers' build
# paths.
# PRIVATE headers are used only while building this project 
#
target_include_directories ( iffl 
                             INTERFACE
                                 $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
                                 $<INSTALL_INTERFACE:include>)

install ( TARGETS iffl 
          EXPORT ifflconfig
        )
#
# Without that line
#          INCLUDES DESTINATION include "include"
#
# install generates ifflconfig.cmake with
# INTERFACE_INCLUDE_DIRECTORIES "${_IMPORT_PREFIX}/include
# if we add line above the include header shows up here 
# 2 more times
#

#
# We do not need to copy includes to the install 
# folder. install above will add include directory
# on the exported targets
# #This is for Windows
#
#install ( DIRECTORY include/ 
#          DESTINATION include ${CMAKE_INSTALL_INCLUDEDIR} )

#
# This makes the project importable from the install directory
# Put config file in per-project dir (name MUST match), can also
# just go into 'cmake'.
#
install ( EXPORT ifflconfig 
          DESTINATION share/iffl/cmake )

#
# This makes the project importable from the build directory
#
export ( TARGETS iffl FILE iiflconfig.cmake )


#
# ----------------------------------Enable Doxygen----------------------
#
# first we can indicate the documentation build as an option and set it to ON by default
#
option(BUILD_DOC "Build documentation" ON)

# check if Doxygen is installed
# https://p5r.uk/blog/2014/cmake-doxygen.html
# http://www.doxygen.nl/manual/docblocks.html
#
find_package(Doxygen)
option(BUILD_DOCUMENTATION "Create and install the HTML based API documentation (requires Doxygen)" ${DOXYGEN_FOUND})

message(STATUS "Build type  ${CMAKE_BUILD_TYPE}")

if (CMAKE_BUILD_TYPE MATCHES "^[Rr]el")
    if(BUILD_DOCUMENTATION)
        if (DOXYGEN_FOUND)
            # set input and output files
            set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile.in)
            set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

            # request to configure the file
            configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)
            message("Doxygen build started")

            # note the option ALL which allows to build the docs together with the application
            add_custom_target( iffl_doc ALL
                COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
                WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                COMMENT "Generating API documentation with Doxygen"
                VERBATIM )

            install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/doc_doxygen/html DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/docs)

        else (DOXYGEN_FOUND)
          message("Doxygen need to be installed to generate the doxygen documentation")
        endif (DOXYGEN_FOUND)
    endif()
endif()

#
# --------------------------------- test --------------------------------
#

#
# Unit tests, of course
#
add_executable ( iffl_test
                 test/iffl_test.cpp
                 test/iffl_test_cases.cpp
                 test/iffl_ea_usecase.cpp
                 test/iffl_c_api_usecase1.cpp
                 test/iffl_c_api_usecase2.cpp
                 test/iffl_views.cpp
                 test/iffl_unaligned.cpp
               )

#
# MSVC specific hacks
#
if ( MSVC )
   
    # append("/W4" msvc_warning_flags)
    #
    # CMake appends /W3 by default, and having /W3 followed by /W4 will result in 
    # cl : Command line warning D9025 : overriding '/W3' with '/W4'.  Since this is
    # a command line warning and not a compiler warning, it cannot be suppressed except
    # by fixing the command line.
    #
    string (REGEX REPLACE " /W[0-4]" "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}" )
    string (REGEX REPLACE " /W[0-4]" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}" )

    #
    # Downgrade following warnings to level W4
    #
    # 4061 - enumerator 'identifier' in switch of enum 'enumeration' is not explicitly handled by a case label
    # 4062 - enumerator 'identifier' in switch of enum 'enumeration' is not handled
    # 4265 - 'classname': class has virtual functions, but destructor is not virtual\n instances of this class may not be destructed correctly
    #        
    target_compile_options ( iffl_test
                             PRIVATE
                                 /W3
                                 /w44265
                                 /w44061
                                 /w44062
                                 /WX )

    #
    # generate preprocessor header
    #
    #target_compile_options( iffl_test PRIVATE /P)

    #
    # Warning C26490 Don't use reinterpret_cast (type.1).
    # [[gsl::suppress(type.1)]]
    # Warning C26429 Symbol 'size_ptr' is never tested for nullness, it can be marked as not_null(f.23).
    # [[gsl::suppress(f.23)]]
    # Warning C26481 Don't use pointer arithmetic. Use span instead (bounds.1).
    # [[gsl::suppress(bounds.1)]]
    # 

    #
    # Make sure we are MSVC extensions free
    #
    target_compile_options( iffl_test 
                            PRIVATE 
#                                /permissive- /analyze:plugin EspXEngine.dll /wd26490 /wd26429 /wd26481)
                                /permissive- )
elseif(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pedantic -Werror")
endif( )

target_link_libraries ( iffl_test
                        iffl
                      )

target_compile_definitions ( iffl_test
                            PRIVATE
                                FFL_DBG_CHECK_DATA_VALID
                                FFL_DBG_CHECK_ITERATOR_VALID )

add_test ( iffl_test
           iffl_test
         )
