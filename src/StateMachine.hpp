#ifndef CANOPEN_MASTER_STATE_MACHINE_HPP
#define CANOPEN_MASTER_STATE_MACHINE_HPP

#include <base/Time.hpp>
#include <canmessage.hh>
#include <canopen_master/Exceptions.hpp>
#include <canopen_master/Frame.hpp>
#include <canopen_master/PDOCommunicationParameters.hpp>
#include <canopen_master/PDOMapping.hpp>
#include <map>
#include <vector>

namespace canopen_master {
    /** A state machine that handles data transfers between a CANOpen server and the
     * master
     */
    class StateMachine {
    public:
        typedef std::pair<uint16_t, uint8_t> ObjectIdentifier;

        enum UPDATE_EVENT {
            PROCESSED_IGNORED_MESSAGE,
            /** Message was not from the node handled by this StateMachine */
            PROCESSED_NOT_FOR_ME,
            /** Processed a TPDO object that had been declared with declareTPDO */
            PROCESSED_PDO,
            /** Received a TPDO object not declared with declareTPDO */
            PROCESSED_PDO_UNEXPECTED,
            /**
             * Updated an object with data received from the remote device
             */
            PROCESSED_SDO,
            /**
             * Ack for a SDO download request, i.e. the object has been written
             */
            PROCESSED_SDO_INITIATE_DOWNLOAD,
            PROCESSED_SDO_IGNORED_COMMAND,
            PROCESSED_SDO_UNKNOWN_COMMAND,
            /** Received a heartbeat */
            PROCESSED_HEARTBEAT,
            /** Received an emergency message with no error in it */
            PROCESSED_EMERGENCY_NO_ERROR
        };

        enum QUIRKS {
            PDO_COBID_MESSAGE_RESERVED_BIT_QUIRK = 0x1
        };

        struct Update {
            UPDATE_EVENT mode;
            int update_count;
            ObjectIdentifier updated[8];

            Update();
            Update(UPDATE_EVENT mode);
            Update(UPDATE_EVENT mode, uint16_t objectId, uint8_t subId);

            /** Adds an object to the update set */
            void addUpdate(uint16_t objectId, int8_t subId);

            /**
             * Adds an object to the update set using a type representing the
             * dictionary object
             */
            template <typename T>
            void addUpdate(uint16_t objectIdOffset = 0, int8_t subIdOffset = 0)
            {
                return addUpdate(T::OBJECT_ID + objectIdOffset,
                    T::OBJECT_SUB_ID + subIdOffset);
            }

            /** Whether this update has updated objects */
            bool hasUpdatedObjects() const;

            /** Whether this update has updated objects */
            bool hasUpdatedObject(uint16_t objectId, int8_t subId) const;

            /** Whether this update has updated objects */
            template <typename T>
            bool hasUpdatedObject(int objectIdOffset = 0, int objectSubIdOffset = 0) const
            {
                return hasUpdatedObject(T::OBJECT_ID + objectIdOffset,
                    T::OBJECT_SUB_ID + objectSubIdOffset);
            }

            bool operator==(Update const& other) const;

            ObjectIdentifier* begin()
            {
                return updated;
            }
            ObjectIdentifier* end()
            {
                return updated + update_count;
            }
            const ObjectIdentifier* begin() const
            {
                return updated;
            }
            const ObjectIdentifier* end() const
            {
                return updated + update_count;
            }
        };

    private:
        /** The ID of the node we're talking to
         */
        uint8_t nodeId;

        uint64_t quirks = 0;

        struct ObjectValue {
            uint16_t objectId;
            uint8_t subId;

            base::Time lastUpdate;
            uint8_t data[4];
            mutable uint8_t size;
            mutable bool knownSize;
        };

        typedef std::map<ObjectIdentifier, ObjectValue> Dictionary;
        typedef std::vector<PDOMapping> PDOMappings;

        base::Time lastMessageTime;

        base::Time lastStateUpdate;
        NODE_STATE state;

        PDOMappings rpdoMappings;
        PDOMappings tpdoMappings;
        Dictionary dictionary;
        bool useUnknownSizes;
        Dictionary::iterator declareInternal(uint16_t objectId,
            uint8_t subId,
            uint8_t size,
            bool knownSize);

    public:
        StateMachine(uint8_t nodeId, bool useUnknownSizes = false);

        /** Returns the time of the last state update message received */
        base::Time getLastStateUpdate() const;

        /** Returns the time of the last message received from this node */
        base::Time getLastMessageTime() const;

        /** Returns whether we have received a node state update */
        bool hasState() const;

        /** Change the node state
         */
        canbus::Message queryStateTransition(NODE_STATE_TRANSITION transition) const;

        /** Set out-of-spec of ill-specified behaviors
         */
        void setQuirks(uint64_t field);

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

        /** Returns whether data size field will be unset in SDO communications */
        uint8_t getUseUnknownSizes() const;

        /** Sets whether data size field will be unset in SDO communications */
        void setUseUnknownSizes(bool toggle);

        /** Process a message received from nodeId */
        Update process(canbus::Message const& msg);

        /** Request reading the given dictionary object */
        canbus::Message upload(uint16_t objectId, uint8_t subId) const;

        /** Download the value of a SDO object using raw data
         *
         * @throw InvalidObjectType
         */
        canbus::Message download(uint16_t objectId,
            uint8_t subId,
            uint8_t const* value,
            uint32_t size) const;

        /** Request writing the given dictionary object */
        template <typename T>
        canbus::Message download(uint16_t objectId, uint8_t subId, T value) const
        {
            uint8_t data[4];
            toLittleEndian<T>(data, value);
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
        static canbus::Message sync();

        /** Get raw data from a given object */
        uint32_t get(uint16_t objectId,
            uint16_t subId,
            uint8_t* data,
            uint32_t bufferSize) const;

        /** Set an object's value in the dictionary */
        template <typename T>
        void set(uint16_t objectId,
            uint8_t subId,
            T value,
            base::Time const& time = base::Time::now())
        {
            uint8_t buffer[sizeof(value)];
            toLittleEndian(buffer, value);
            setObjectValue(objectId, subId, time, buffer, sizeof(value));
        }

        /** Get the currently known value for the given object */
        template <typename T> T get(uint16_t objectId, uint8_t subId) const
        {
            uint8_t data[4];
            uint16_t size = get(objectId, subId, data, 4);
            if (size == 0)
                throw ObjectNotRead(
                    "attempting to get an object that has never been read");

            const ObjectValue& object =
                dictionary.find(ObjectIdentifier(objectId, subId))->second;
            if (size != sizeof(T) && object.knownSize) {
                throw InvalidObjectType("unexpected requested object size in get");
            }
            else if (!object.knownSize) {
                object.size = sizeof(T);
                object.knownSize = true;
            }
            return fromLittleEndian<T>(data);
        }

        /** Disable a previously configured PDO */
        canbus::Message disablePDO(bool transmit,
            uint8_t pdoIndex,
            uint32_t cob_id = 0) const;

        /** Configures a whole PDO */
        std::vector<canbus::Message> configurePDO(bool transmit,
            uint8_t pdoIndex,
            PDOCommunicationParameters const& parameters,
            PDOMapping const& mapping) const;

        /** Configure the communication parameters for the given PDO */
        std::vector<canbus::Message> configurePDOParameters(bool transmit,
            uint8_t pdoIndex,
            PDOCommunicationParameters const& parameters) const;

        /** Configures the mapping for one of the predefined PDOs */
        std::vector<canbus::Message> configurePDOMapping(bool transmit,
            uint8_t pdoIndex,
            PDOMapping const& mapping) const;

        /** Declare a TPDO mapping to the state machine
         *
         * TPDOs are PDOs sent by the slave
         *
         * After this, whenever the master receives a TPDO, it will update the
         * corresponding objects in the dictionary
         */
        void declareTPDOMapping(uint8_t pdoIndex, PDOMapping const& mapping);

        /** Declare a RPDO mapping to the state machine
         *
         * RPDOs are PDOs sent to the slave
         *
         * Use this to be able to use .getPDO to build your PDO messages
         */
        void declareRPDOMapping(uint8_t pdoIndex, PDOMapping const& mapping);

        /** Return the RPDO message that corresponds to the mapping declared with
         * declareRPDOMapping
         */
        canbus::Message getRPDOMessage(unsigned int pdoIndex);

    private:
        void validatePDOMapping(PDOMapping const& mapping) const;
        Update processEmergency(canbus::Message const& msg);
        Update processSDOReceive(canbus::Message const& msg);
        Update processHeartbeat(canbus::Message const& msg);
        Update processPDOReceive(int pdoIndex, canbus::Message const& msg);
        void setObjectValue(uint16_t objectId,
            uint8_t subId,
            base::Time const& time,
            uint8_t const* data,
            uint32_t dataSize);

        /** Helper method for declareTPDOMapping and declareRPDOMapping */
        void declarePDOMapping(uint8_t pdoIndex,
            PDOMapping const& mapping,
            std::vector<PDOMapping>& mappings);
    };
}

#endif
