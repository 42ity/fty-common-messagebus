# Mqtt samples

etn_target(exe fty_common_messagebus_mqtt_example
    SOURCES
        fty_common_messagebus_mqtt_example.cc
    USES
        ${PROJECT_NAME_UNDERSCORE}
        cxxtools
)
