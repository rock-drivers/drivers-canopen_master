#ifndef CANOPEN_MASTER_OBJECTS_HPP
#define CANOPEN_MASTER_OBJECTS_HPP

#include <cstdint>
#include <stdexcept>
#include <canopen_master/Frame.hpp>

namespace canopen_master {
    #define CANOPEN_DEFINE_OBJECT(object_id, object_sub_id, name, type) \
        struct name {\
            static const int OBJECT_ID = object_id; \
            static const int OBJECT_SUB_ID = object_sub_id; \
            typedef type OBJECT_TYPE; \
        };

    CANOPEN_DEFINE_OBJECT(0x1000, 0, DeviceType,                    std::uint32_t);
    CANOPEN_DEFINE_OBJECT(0x1001, 0, ErrorRegister,                 std::uint8_t);
    CANOPEN_DEFINE_OBJECT(0x1002, 0, ManufacturerStatusRegister,    std::uint32_t);
    CANOPEN_DEFINE_OBJECT(0x1016, 2, ConsumerHeartbeatTime,         std::uint32_t);
    CANOPEN_DEFINE_OBJECT(0x1017, 0, ProducerHeartbeatTime,         std::uint32_t);
    CANOPEN_DEFINE_OBJECT(0x1018, 4, IdentityObject,                std::uint32_t);
}

#endif
