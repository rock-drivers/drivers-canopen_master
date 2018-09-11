#ifndef canopen_master_EXCEPTIONS_HPP
#define canopen_master_EXCEPTIONS_HPP

#include <stdexcept>
#include <sstream>
#include <canopen_master/Emergency.hpp>

namespace canopen_master
{
    struct ProtocolError : public std::runtime_error
    {
        using std::runtime_error::runtime_error;
    };

    struct ObjectNotRead : public std::runtime_error
    {
        using std::runtime_error::runtime_error;
    };

    struct BufferSizeTooSmall : public std::runtime_error
    {
        using std::runtime_error::runtime_error;
    };

    struct InvalidObjectType : public std::runtime_error
    {
        using std::runtime_error::runtime_error;
    };

    struct ObjectSizeMismatch : public std::runtime_error
    {
        using std::runtime_error::runtime_error;
    };

    struct Unsupported : public std::runtime_error
    {
        using std::runtime_error::runtime_error;
    };

    struct EmergencyMessageReceived : public std::runtime_error
    {
        const Emergency message;

        static std::string formatMessage(Emergency const& message)
        {
            std::stringstream cv;
            cv << std::hex << "emergency message received ("
                << "code=" << message.code
                << ", register=" << static_cast<int>(message.errorRegister)
                << ", vendor=";

            for (int i = 0; i < 5; ++i)
                cv << " " << static_cast<int>(message.vendorSpecific[i]);
            return cv.str();
        }

        EmergencyMessageReceived(Emergency message)
            : std::runtime_error(formatMessage(message))
            , message(message) {}
    };

    struct SDODomainTransferAborted : public std::runtime_error
    {
        const uint16_t objectId;
        const uint8_t  subId;
        const uint32_t rawCode;

        static std::string formatMessage(uint16_t objectId, uint8_t subId, uint32_t rawCode);

        SDODomainTransferAborted(uint16_t objectId, uint8_t subId, uint32_t rawCode)
            : std::runtime_error(formatMessage(objectId, subId, rawCode))
            , objectId(objectId)
            , subId(subId)
            , rawCode(rawCode) {}
    };

    struct PDOMappingTooBig : public std::runtime_error
    {
        PDOMappingTooBig()
            : std::runtime_error("trying to create a PDO mapping bigger than 8 bytes") {}
    };
}

#endif
