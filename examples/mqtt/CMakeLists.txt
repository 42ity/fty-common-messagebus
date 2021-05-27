# Mqtt samples

etn_target(exe fty_common_messagebus_mqtt_sample_pub_sub
    SOURCES
        src/*PubSub.cpp
    INCLUDE_DIRS
        private_include
    USES
        ${PROJECT_NAME_UNDERSCORE}
        cxxtools
)

etn_target(exe fty_common_messagebus_mqtt_sample_req_rep
    SOURCES
        src/*SampleReqRep.cpp
    INCLUDE_DIRS
        private_include
    USES
        ${PROJECT_NAME_UNDERSCORE}
        cxxtools
)

etn_target(exe fty_common_messagebus_mqtt_sample_rep
    SOURCES
        src/*SampleRep.cpp
    INCLUDE_DIRS
        private_include
    USES
        ${PROJECT_NAME_UNDERSCORE}
        cxxtools
)

etn_target(exe fty_common_messagebus_mqtt_sample_req
    SOURCES
        src/*SampleReq.cpp
    INCLUDE_DIRS
        private_include
    USES
        ${PROJECT_NAME_UNDERSCORE}
        cxxtools
)

# etn_target(exe rpc_math_cli
#     SOURCES
#         src/rpc*cli.cpp
#     USES
#         ${PROJECT_NAME_UNDERSCORE}
#         cxxtools
# )

# etn_target(exe rpc_math_srvr
#     SOURCES
#       src/rpc*srv.cpp
#     USES
#         ${PROJECT_NAME_UNDERSCORE}
#         cxxtools
# )
