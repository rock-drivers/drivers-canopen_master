rock_library(canopen_protocol
    SOURCES Dummy.cpp
    HEADERS Dummy.hpp)

rock_executable(canopen_protocol_bin Main.cpp
    DEPS canopen_protocol)

