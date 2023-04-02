cmake_minimum_required(VERSION 3.13)
cmake_policy(VERSION 3.13)

etn_target(exe ${PROJECT_NAME}-cli
    SOURCES
        src/main.cc
    USES
        ${PROJECT_NAME_UNDERSCORE}
        czmq
        mlm
        fty_common
        fty_common_mlm
)
