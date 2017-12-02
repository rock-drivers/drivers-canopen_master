#ifndef CANOPEN_MASTER_STATE_MACHINE_HPP
#define CANOPEN_MASTER_STATE_MACHINE_HPP

#include <map>
#include <vector>
#include <base/Time.hpp>
#include <canmessage.hh>
#include <canopen_master/Frame.hpp>
#include <canopen_master/Exceptions.hpp>
#include <canopen_master/PDOMapping.hpp>
#include <canopen_master/PDOCommunicationParameters.hpp>

namespace canopen_master
{
    /** A state machine that handles data transfers between a CANOpen server and the master
     */
    class StateMachine
    {
    public:
        typedef std::pair<uint16_t, uint8_t> ObjectIdentifier;

    private:
        /** The ID of the node we're talking to
         */
        uint8_t nodeId;

        struct ObjectValue
        {
            uint16_t objectId;
            uint8_t subId;

            base::Time lastUpdate;
            std::vector<uint8_t> data;
        };

        typedef std::map<ObjectIdentifier, ObjectValue> Dictionary;
        typedef std::vector<PDOMapping> PDOMappings;

        base::Time lastMessageTime;

        base::Time lastStateUpdate;
        NODE_STATE state;

        PDOMappings pdoMappings;
        Dictionary dictionary;
        Dictionary::iterator declareInternal(uint16_t objectId, uint8_t subId, uint8_t size);

    public:
        StateMachine(uint8_t nodeId);

        /** Returns the time of the last state update message received */
        base::Time getLastStateUpdate() const;

        /** Returns the time of the last message received from this node */
        base::Time getLastMessageTime() const;

        /** Returns whether we have received a node state update */
        bool hasState() const;

        /** Returns the last known state
         *
         * @throw std::runtime_error if the state is unknown
         */
        NODE_STATE getState() const;

        /** Returns a message that requires the node to send its current state
         */
        canbus::Message queryState() const;

        /** Returns the node ID this state machine is talking to */
        uint8_t getNodeID() const;

        /** Process a message received from nodeId */
        bool process(canbus::Message const& msg);

        /** Request reading the given dictionary object */
        canbus::Message upload(uint16_t objectId, uint8_t subId) const;

        /** Download the value of a SDO object using raw data
         *
         * @throw InvalidObjectType
         */
        canbus::Message download(uint16_t objectId, uint8_t subId, uint8_t const* value, uint32_t size) const;

        /** Request writing the given dictionary object */
        template<typename T>
        canbus::Message download(uint16_t objectId, uint8_t subId, T value) const
        {
            uint8_t data[4];
            toLittleEndian<uint16_t>(data, value);
            return download(objectId, subId, data, sizeof(value));
        }

        /** Declare an object in the dictionary
         *
         * Note that objects are declared automatically the first time an upload
         * is processed. You would want to use this to add some level of validation,
         * and/or to pre-allocate the object storage
         */
        void declare(uint16_t objectId, uint8_t subId, uint32_t size);

        /** Test if the given object ID and sub ID has ever been read */
        bool has(uint16_t objectId, uint8_t subId) const;

        /** Returns the size of the given entry in the dictionary
         * @return the size, or zero if this entry is not defined
         */
        uint32_t sizeOf(uint16_t objectId, uint8_t subId) const;

        /** Returns the timestamp of the last read value for this object
         *
         * Returns a null time if the object has never been read.
         * Null times can be tested against with base::Time::isNull(),
         * or check availability first with the has method
         */
        base::Time timestamp(uint16_t objectId, uint8_t subId) const;

        /** Returns the SYNC message
         *
         * The SYNC message triggers sending the PDOs that have been
         * configured to
         */
        canbus::Message sync() const;

        /** Get raw data from a given object */
        uint32_t get(uint16_t objectId, uint16_t subId, uint8_t* data, uint32_t bufferSize) const;

        /** Get the currently known value for the given object */
        template<typename T>
        T get(uint16_t objectId, uint8_t subId) const
        {
            uint8_t data[4];
            uint16_t size = get(objectId, subId, data, 4);
            if (size == 0)
                throw ObjectNotRead("attempting to get an object that has never been read");
            if (size != sizeof(T))
                throw InvalidObjectType("unexpected requested object size in get");

            return fromLittleEndian<T>(data);
        }

        /** Configure the communication parameters for the given PDO */
        canbus::Message configurePDOParameters(bool transmit, uint8_t pdoIndex,
            PDOCommunicationParameters const& parameters);

        /** Configures the mapping for one of the predefined PDOs */
        std::vector<canbus::Message> configurePDOMapping(bool transmit, uint8_t pdoIndex,
            PDOMapping const& mapping);

        /** Declare a PDO mapping to the state machine
         *
         * After this, the process method will map a PDO message to the
         * object dictionary.
         */
        void declarePDOMapping(uint8_t pdoIndex, PDOMapping const& mapping);

    private:
        void validatePDOMapping(PDOMapping const& mapping);
        bool processEmergency(canbus::Message const& msg);
        bool processSDOReceive(canbus::Message const& msg);
        bool processHeartbeat(canbus::Message const& msg);
        bool processPDOReceive(int pdoIndex, canbus::Message const& msg);
        void setObjectValue(uint16_t objectId, uint8_t subId, base::Time const& time, uint8_t const* data, uint32_t dataSize);
    };
}

#endif
