cmake_minimum_required(VERSION 3.13)
cmake_policy(VERSION 3.13)

project(fty-common-messagebus
    VERSION 1.0.0
    DESCRIPTION "Common messagebus interface"
)

set(PROJECT_NAME_UNDERSCORE fty_common_messagebus)

option(ENABLE_EXAMPLES "Build samples" TRUE)

##############################################################################################################
find_package(fty-cmake PATHS ${CMAKE_BINARY_DIR}/fty-cmake REQUIRED)
find_package(PahoMqttCpp REQUIRED)
##############################################################################################################


##############################################################################################################
etn_target(shared ${PROJECT_NAME_UNDERSCORE}
    SOURCES
        # src/*.cc
        src/fty_common_messagebus_dto.cc
        src/fty_common_messagebus_interface.cc
        src/fty_common_messagebus_malamute.cc
        src/fty_common_messagebus_pool_worker.cc
        # src/fty-msgbus-cli.cc
        src/fty_common_messagebus_mqtt.cpp
        src/*.cpp
    PUBLIC_INCLUDE_DIR
        public_include
    INCLUDE_DIRS
        private_include
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
        paho-mqttpp3
)

# etn_target(exe fty-msgbus-cli
#     SOURCES
#         src/fty-msgbus-cli.cc
#     USES
#         ${PROJECT_NAME_UNDERSCORE}
#         cxxtools
# )

set_target_properties(${PROJECT_NAME_UNDERSCORE} PROPERTIES SOVERSION ${PROJECT_VERSION_MAJOR})

##############################################################################################################

#### Examples
if(ENABLE_EXAMPLES)
  add_subdirectory(examples)
endif()

##############################################################################################################

# Create a target for the tests
etn_test_target(${PROJECT_NAME_UNDERSCORE}
    SOURCES
        test/main.cpp
        test/dispatcher.cpp
        test/pool_worker.cpp
)

##############################################################################################################
