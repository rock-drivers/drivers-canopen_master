#ifndef CANOPEN_MASTER_FRAME_HPP
#define CANOPEN_MASTER_FRAME_HPP

#include <canmessage.hh>

namespace canopen_master
{
    enum PREDEFINED_BROADCAST_IDS
    {
        BROADCAST_NMT_MODULE_CONTROL = 0x000,
        BROADCAST_SYNC               = 0x080,
        BROADCAST_TIMESTAMP          = 0x100
    };

    /** Function codes in the predefined set
     *
     * "Transmit" and "Receive" are as seen from the remote node
     */
    enum PREDEFINED_FUNCTION_CODES
    {
        FUNCTION_EMERGENCY     =  0x080,

        FUNCTION_PDO0_TRANSMIT =  0x180,
        FUNCTION_PDO0_RECEIVE  =  0x200,
        FUNCTION_PDO1_TRANSMIT =  0x280,
        FUNCTION_PDO1_RECEIVE  =  0x300,
        FUNCTION_PDO2_TRANSMIT =  0x380,
        FUNCTION_PDO2_RECEIVE  =  0x400,
        FUNCTION_PDO3_TRANSMIT =  0x480,
        FUNCTION_PDO3_RECEIVE  =  0x500,

        FUNCTION_SDO_TRANSMIT  =  0x580,
        FUNCTION_SDO_RECEIVE   =  0x600,

        FUNCTION_NMT_HEARTBEAT =  0x700,

        FUNCTION_MASK          =  0x780
    };

    static const int MAX_PDO = 3;

    enum NODE_STATE
    {
        NODE_INITIALIZING = 0x00,
        NODE_STOPPED = 0x04,
        NODE_OPERATIONAL = 0x05,
        NODE_PRE_OPERATIONAL = 0x7f
    };

    enum NODE_STATE_TRANSITION
    {
        NODE_START = 0x01,
        NODE_STOP  = 0x02,
        NODE_ENTER_PRE_OPERATIONAL = 0x80,
        NODE_RESET = 0x81,
        NODE_RESET_COMMUNICATION = 0x82
    };

    inline uint16_t getFunctionCode(canbus::Message const& msg)
    {
        return msg.can_id & FUNCTION_MASK;
    }

    inline bool testFunctionCode(canbus::Message const& msg, PREDEFINED_FUNCTION_CODES code)
    {
        return (msg.can_id & FUNCTION_MASK) == code;
    }

    inline bool isBroadcast(canbus::Message const& msg)
    {
        return msg.can_id == BROADCAST_NMT_MODULE_CONTROL ||
            msg.can_id == BROADCAST_TIMESTAMP ||
            msg.can_id == BROADCAST_SYNC;
    }

    inline bool testBroadcastMessage(canbus::Message const& msg, PREDEFINED_BROADCAST_IDS code)
    {
        return msg.can_id == code;
    }


    inline uint8_t getNodeID(canbus::Message const& msg)
    {
        return (msg.can_id & 0x7F);
    }

    template<typename T> void toLittleEndian(uint8_t* data, T value);
    template<> inline void toLittleEndian(uint8_t* data, uint8_t value)
    {
        data[0] = value;
    }
    template<> inline void toLittleEndian(uint8_t* data, uint16_t value)
    {
        data[0] = (value >> 0) & 0xFF;
        data[1] = (value >> 8) & 0xFF;
    }
    template<> inline void toLittleEndian(uint8_t* data, uint32_t value)
    {
        data[0] = (value >>  0) & 0xFF;
        data[1] = (value >>  8) & 0xFF;
        data[2] = (value >> 16) & 0xFF;
        data[3] = (value >> 24) & 0xFF;
    }
    template<> inline void toLittleEndian(uint8_t* data, int8_t value)
    {
        return toLittleEndian<uint8_t>(data, reinterpret_cast<uint8_t&>(value));
    }
    template<> inline void toLittleEndian(uint8_t* data, int16_t value)
    {
        return toLittleEndian<uint16_t>(data, reinterpret_cast<uint16_t&>(value));
    }
    template<> inline void toLittleEndian(uint8_t* data, int32_t value)
    {
        return toLittleEndian<uint32_t>(data, reinterpret_cast<uint32_t&>(value));
    }

    template<typename T> T fromLittleEndian(uint8_t const* data);
    template<> inline uint8_t fromLittleEndian(uint8_t const* data)
    {
        return data[0];
    }
    template<> inline uint16_t fromLittleEndian(uint8_t const* data)
    {
        return
            static_cast<uint16_t>(data[0]) << 0 |
            static_cast<uint16_t>(data[1]) << 8;
    }
    template<> inline uint32_t fromLittleEndian(uint8_t const* data)
    {
        return
            static_cast<uint32_t>(data[0]) << 0  |
            static_cast<uint32_t>(data[1]) << 8  |
            static_cast<uint32_t>(data[2]) << 16 |
            static_cast<uint32_t>(data[3]) << 24;
    }
    template<> inline int8_t fromLittleEndian(uint8_t const* data)
    {
        return reinterpret_cast<int8_t const&>(data[0]);
    }
    template<> inline int16_t fromLittleEndian(uint8_t const* data)
    {
        uint16_t result = fromLittleEndian<uint16_t>(data);
        return reinterpret_cast<int16_t const&>(result);
    }
    template<> inline int32_t fromLittleEndian(uint8_t const* data)
    {
        uint32_t result = fromLittleEndian<uint32_t>(data);
        return reinterpret_cast<int32_t const&>(result);
    }
}

#endif
