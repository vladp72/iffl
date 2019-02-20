# CMakeList.txt : 
# based on https://rix0r.nl/blog/2015/08/13/cmake-guide/ 
# alse see 
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
cmake_minimum_required ( VERSION 3.8 )

#
# ac is a C++ project
#
project ("iffl" VERSION 0.1 LANGUAGES CXX )

#
# Supress CMake Warning: Manually-specified variables were not used by the project, CMAKE_C_COMPILER
# Most solutions are suggesting to use --no-warn-unused-cli instead of touching variable,
# but at least for now I like touching better because I do not have to look up helpon internet
# name of that parameter every time I forget it.
#
message(STATUS "Supressing stupid warning that CMAKE_C_COMPILER is not used. Here I am using it! Happy now?: ${CMAKE_C_COMPILER}.")

#
# Not yet sure what thar really does
#
enable_testing( )

#
# This is a static library
#
add_library ( iffl 
              STATIC
              "src/dummy.cpp")

#
# If we have compiler requirements for this library, list them
# here
# https://cmake.org/cmake/help/v3.8/prop_gbl/CMAKE_CXX_KNOWN_FEATURES.html#prop_gbl:CMAKE_CXX_KNOWN_FEATURES 
#
target_compile_features ( iffl
                          PUBLIC
                              cxx_std_17
                              cxx_auto_type
                              cxx_final
                              cxx_deleted_functions
                              cxx_defaulted_functions
                              cxx_attributes
                              cxx_variadic_templates )

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
    target_compile_options ( iffl
                             PRIVATE
                                 /W3
                                 /w44265
                                 /w44061
                                 /w44062
                                 /WX )

    #
    # generate preprocessor header
    #
    #target_compile_options( ac PRIVATE /P)

    #
    # Make sure we are MSVC extensions free
    #
    target_compile_options( iffl 
                            PRIVATE 
                                /permissive- )
endif( )

#
# We are building in unicode mode, 
# and we support only clients that are build 
#in unicode mode
#
# target_compile_definitions ( iffl 
#                             PUBLIC
#                                 UNICODE
#                                 _UNICODE
#                                 JET_UNICODE )

#
# Define headers for this library. PUBLIC headers are used for
# compiling the library, and will be added to consumers' build
# paths.
# PRIVATE headers are used only while building this project 
#
target_include_directories ( iffl 
                             PUBLIC
                                 $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
                                 $<INSTALL_INTERFACE:include>
                             PRIVATE 
                                 src )

#
# 'make install' to the correct locations (provided by GNUInstallDirs).
#
install ( TARGETS iffl 
          EXPORT ifflconfig
          ARCHIVE  DESTINATION lib ${CMAKE_INSTALL_LIBDIR}
          LIBRARY  DESTINATION lib ${CMAKE_INSTALL_LIBDIR}
          RUNTIME  DESTINATION bin ${CMAKE_INSTALL_BINDIR}
        )

#
# This is for Windows
#
install ( DIRECTORY include/ 
          DESTINATION include ${CMAKE_INSTALL_INCLUDEDIR} )

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
export ( TARGETS iffl FILE aiiflconfig.cmake )


#
# ----------------------------------Enable Doxygen----------------------
#
# first we can indicate the documentation build as an option and set it to ON by default
#
option(BUILD_DOC "Build documentation" ON)

# check if Doxygen is installed
find_package(Doxygen)
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
else (DOXYGEN_FOUND)
  message("Doxygen need to be installed to generate the doxygen documentation")
endif (DOXYGEN_FOUND)


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
                 test/iffl_c_api_usecase.cpp
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
    # Make sure we are MSVC extensions free
    #
    target_compile_options( iffl_test 
                            PRIVATE 
                                /permissive- )
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
