cmake_minimum_required(VERSION 3.13)
cmake_policy(VERSION 3.13)

project(fty-common-messagebus
    VERSION 1.0.0
    DESCRIPTION "Common messagebus interface"
)

set(PROJECT_NAME_UNDERSCORE fty_common_messagebus)

##############################################################################################################
find_package(fty-cmake PATHS ${CMAKE_BINARY_DIR}/fty-cmake)
##############################################################################################################


##############################################################################################################
etn_target(shared ${PROJECT_NAME_UNDERSCORE}
    SOURCES
        src/fty_common_messagebus_dto.cc
        src/fty_common_messagebus_interface.cc
        src/fty_common_messagebus_malamute.cc
        src/fty_common_messagebus_pool_worker.cc
    PUBLIC_INCLUDE_DIR
        public_include
    PUBLIC_HEADERS
        fty_common_messagebus_dispatcher.h
        fty_common_messagebus_dto.h
        fty_common_messagebus_exception.h
        fty_common_messagebus.h
        fty_common_messagebus_interface.h
        fty_common_messagebus_library.h
        fty_common_messagebus_message.h
        fty_common_messagebus_pool_worker.h
    USES_PUBLIC
        fty_common_logging
    USES
        cxxtools
        czmq
        fty_common_mlm
        mlm
        pthread
)

etn_target(exe fty-msgbus-cli
    SOURCES
        src/fty-msgbus-cli.cc
    USES
        ${PROJECT_NAME_UNDERSCORE}
        cxxtools
)

##############################################################################################################

#examples 
etn_target(exe fty_common_messagebus_example
    SOURCES
        examples/fty_common_messagebus_example.cc
    USES
        ${PROJECT_NAME_UNDERSCORE}
        cxxtools
)

# etn_test_target(fty_common_messagebus_example
#     SOURCES
#         test/main.cpp
# )

etn_target(exe fty_common_messagebus_example2
    SOURCES
        examples/fty_common_messagebus_example2.cc
    USES
        ${PROJECT_NAME_UNDERSCORE}
        cxxtools
)

etn_target(exe fty_common_messagebus_example_req
    SOURCES
        examples/fty_common_messagebus_example_req.cc
    USES
        ${PROJECT_NAME_UNDERSCORE}
        cxxtools
)

etn_target(exe fty_common_messagebus_example_rep
    SOURCES
        examples/fty_common_messagebus_example_rep.cc
    USES
        ${PROJECT_NAME_UNDERSCORE}
        cxxtools
)

etn_target(exe fty_common_messagebus_example_rep2
    SOURCES
        examples/fty_common_messagebus_example_rep2.cc
    PUBLIC_INCLUDE_DIR
        public_include
    USES
        ${PROJECT_NAME_UNDERSCORE}
        cxxtools
)

##############################################################################################################

# Create a target for the tests
etn_test_target(${PROJECT_NAME_UNDERSCORE}
    SOURCES
        test/main.cpp
        test/dispatcher.cpp
        test/pool_worker.cpp
)

##############################################################################################################
