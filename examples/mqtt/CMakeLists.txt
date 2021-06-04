project(fty-common-messagebus-mqtt-sample
        VERSION ${PROJECT_VERSION}
        DESCRIPTION "Fty common message bus sample with mqtt library"
        LANGUAGES CXX)

# Pub/Sub
etn_target(exe ${PROJECT_NAME}-pub-sub
    SOURCES
        src/*PubSub.cpp
    INCLUDE_DIRS
        private_include
    USES
        ${PROJECT_NAME_UNDERSCORE}
        cxxtools
)

# Math sample replyer
etn_target(exe ${PROJECT_NAME}-async-reply
    SOURCES
        src/*SampleAsyncReply.cpp
        src/*Dto.cpp
    INCLUDE_DIRS
        private_include
    USES
        ${PROJECT_NAME_UNDERSCORE}
        cxxtools
)

# Math sample asynchrone and synchrone requester
etn_target(exe ${PROJECT_NAME}-send-request
    SOURCES
        src/*SampleSendRequest.cpp
        src/*Dto.cpp
    INCLUDE_DIRS
        private_include
    USES
        ${PROJECT_NAME_UNDERSCORE}
        cxxtools
)

# Stress lib sample
etn_target(exe ${PROJECT_NAME}-stress-lib
    SOURCES
        src/*SampleStressLib.cpp
        src/*Dto.cpp
    INCLUDE_DIRS
        private_include
    USES
        ${PROJECT_NAME_UNDERSCORE}
        cxxtools
)
