rock_library(canopen_master
    SOURCES NMT.cpp SDO.cpp StateMachine.cpp Emergency.cpp
    HEADERS Frame.hpp NMT.hpp SDO.hpp StateMachine.hpp Exceptions.hpp Emergency.hpp
    DEPS_PKGCONFIG canbus base-types)
