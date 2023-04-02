cmake_minimum_required(VERSION 3.13)
cmake_policy(VERSION 3.13)

etn_target(exe ${PROJECT_NAME}-example
    SOURCES
        src/fty_common_messagebus_example.cc
    USES
        ${PROJECT_NAME_UNDERSCORE}
        mlm
)

etn_target(exe ${PROJECT_NAME}-example2
    SOURCES
        src/fty_common_messagebus_example2.cc
    USES
        ${PROJECT_NAME_UNDERSCORE}
        mlm
)

etn_target(exe ${PROJECT_NAME}-example_req
    SOURCES
        src/fty_common_messagebus_example_req.cc
    USES
        ${PROJECT_NAME_UNDERSCORE}
        mlm
)

etn_target(exe ${PROJECT_NAME}-example_rep
    SOURCES
        src/fty_common_messagebus_example_rep.cc
    USES
        ${PROJECT_NAME_UNDERSCORE}
        mlm
)

etn_target(exe ${PROJECT_NAME}-example_rep2
    SOURCES
        src/fty_common_messagebus_example_rep2.cc
    USES
        ${PROJECT_NAME_UNDERSCORE}
        mlm
)
