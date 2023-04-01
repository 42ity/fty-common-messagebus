cmake_minimum_required(VERSION 3.13)
cmake_policy(VERSION 3.13)

project(fty-common-messagebus
    VERSION 1.0.0
    DESCRIPTION "Common messagebus malamute interface"
)

set(PROJECT_NAME_UNDERSCORE fty_common_messagebus)

########################################################################################################################

find_package(fty-cmake PATHS ${CMAKE_BINARY_DIR}/fty-cmake)

########################################################################################################################

add_subdirectory(lib)
add_subdirectory(examples)
add_subdirectory(cli)
