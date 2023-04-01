cmake_minimum_required(VERSION 3.13)
cmake_policy(VERSION 3.13)

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
        czmq
        mlm
        pthread
        fty_common
        fty_common_mlm
)

set_target_properties(${PROJECT_NAME_UNDERSCORE} PROPERTIES SOVERSION ${PROJECT_VERSION_MAJOR})

etn_test_target(${PROJECT_NAME_UNDERSCORE}
    SOURCES
        test/main.cpp
        test/dispatcher.cpp
        test/pool_worker.cpp
)
