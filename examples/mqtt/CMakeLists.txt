# Mqtt samples

etn_target(exe fty_common_messagebus_mqtt_sample_pub_sub
    SOURCES
        src/*PubSub.cpp
    USES
        ${PROJECT_NAME_UNDERSCORE}
        cxxtools
)

etn_target(exe fty_common_messagebus_mqtt_sample_req_rep
    SOURCES
        src/*ReqRep.cpp
    USES
        ${PROJECT_NAME_UNDERSCORE}
        cxxtools
)
