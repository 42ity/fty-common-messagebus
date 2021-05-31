# Mqtt samples

# Pub/Sub
etn_target(exe fty_common_messagebus_mqtt_sample_pub_sub
    SOURCES
        src/*PubSub.cpp
    INCLUDE_DIRS
        private_include
    USES
        ${PROJECT_NAME_UNDERSCORE}
        cxxtools
)

# Math replyer
etn_target(exe fty_common_messagebus_mqtt_sample_async_reply
    SOURCES
        src/*SampleReply.cpp
        src/*Dto.cpp
    INCLUDE_DIRS
        private_include
    USES
        ${PROJECT_NAME_UNDERSCORE}
        cxxtools
)

# Math asynchrone requester
etn_target(exe fty_common_messagebus_mqtt_sample_async_request
    SOURCES
        src/*SampleAsyncRequest.cpp
        src/*Dto.cpp
    INCLUDE_DIRS
        private_include
    USES
        ${PROJECT_NAME_UNDERSCORE}
        cxxtools
)

# Math synchrone requester
etn_target(exe fty_common_messagebus_mqtt_sample_sync_request
    SOURCES
        src/*SampleSyncRequest.cpp
        src/*Dto.cpp
    INCLUDE_DIRS
        private_include
    USES
        ${PROJECT_NAME_UNDERSCORE}
        cxxtools
)
