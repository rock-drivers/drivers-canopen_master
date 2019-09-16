#ifndef CANOPEN_MASTER_SLAVE_HPP
#define CANOPEN_MASTER_SLAVE_HPP

#include <canopen_master/StateMachine.hpp>
#include <canopen_master/Frame.hpp>
#include <canopen_master/Update.hpp>
#include <canopen_master/Objects.hpp>

namespace canopen_master {
    /** Create a Sync message */
    canbus::Message querySync();

    enum StandardUpdates {
        UPDATE_HEARTBEAT      = 0x00000001,
        UPDATE_CUSTOM_START   = 0x00000010
    };

    /**
     * Controlling class for a single slave on the bus
     *
     * See the README for a more in-depth usage description
     */
    class Slave {
    public:
        Slave(uint8_t nodeId);
        virtual ~Slave();

        /** Query the canopen node state */
        canbus::Message queryNodeState() const;

        /** Return the last known node state */
        canbus::Message queryNodeStateTransition(
            NODE_STATE_TRANSITION transition) const;

        /** Return the last known node state */
        NODE_STATE getNodeState() const;

        /** Query the upload (= read from slave) of an object
         *
         * @arg offsetId an offset that should be applied to the object ID
         * @arg offsetSubId an offset that should be applied to the object sub ID
         */
        template<typename T>
        canbus::Message queryUpload(int offsetId = 0, int offsetSubId = 0) const {
            return mCANOpen.upload(T::OBJECT_ID + offsetId,
                                   T::OBJECT_SUB_ID + offsetSubId);
        }

        /** Query the download (= write to slave) of an object as it is in the database
         *
         * @arg offsetId an offset that should be applied to the object ID
         * @arg offsetSubId an offset that should be applied to the object sub ID
         */
        template<typename T>
        canbus::Message queryDownload() const {
            return mCANOpen.download(T::OBJECT_ID, T::OBJECT_SUB_ID,
                                     get<T>());
        }

        /** Query the download (= write to slave) of an object
         *
         * @arg offsetId an offset that should be applied to the object ID
         * @arg offsetSubId an offset that should be applied to the object sub ID
         */
        template<typename T>
        canbus::Message queryDownload(typename T::OBJECT_TYPE value,
                                      int offsetId = 0, int offsetSubId = 0) const {
            return mCANOpen.download(T::OBJECT_ID + offsetId,
                                     T::OBJECT_SUB_ID + offsetSubId,
                                     value);
        }

        /** Get an object from the object database
         */
        template<typename T>
        typename T::OBJECT_TYPE get(int offsetId = 0, int offsetSubId = 0) const {
            return mCANOpen.get<typename T::OBJECT_TYPE>(
                T::OBJECT_ID + offsetId, T::OBJECT_SUB_ID + offsetSubId
            );
        }

        /** Check whether the given object has been initialized in the object database
         *
         * @arg offsetId an offset that should be applied to the object ID
         * @arg offsetSubId an offset that should be applied to the object sub ID
         */
        template<typename T>
        bool has(int offsetId = 0, int offsetSubId = 0) const {
            return mCANOpen.has(T::OBJECT_ID + offsetId,
                                T::OBJECT_SUB_ID + offsetSubId);
        }

        /** Timestamp of the last written value for the given object (might be zero)
         *
         * @arg offsetId an offset that should be applied to the object ID
         * @arg offsetSubId an offset that should be applied to the object sub ID
         */
        template<typename T>
        base::Time timestamp(int offsetId = 0, int offsetSubId = 0) const {
            return mCANOpen.timestamp(T::OBJECT_ID + offsetId,
                                      T::OBJECT_SUB_ID + offsetSubId);
        }

        template<typename T>
        void set(typename T::OBJECT_TYPE value,
                 base::Time const& time = base::Time::now()) {
            return mCANOpen.set<typename T::OBJECT_TYPE>(
                T::OBJECT_ID, T::OBJECT_SUB_ID,
                value, time
            );
        }

        template<typename T>
        void set(typename T::OBJECT_TYPE value,
                 int offsetId, int offsetSubId = 0,
                 base::Time const& time = base::Time::now()) {
            return mCANOpen.set<typename T::OBJECT_TYPE>(
                T::OBJECT_ID + offsetId, T::OBJECT_SUB_ID + offsetSubId,
                value, time
            );
        }

        virtual Update process(canbus::Message const& message);

        StateMachine& getStateMachine();

        /** Create the given RPDO message
         *
         * The PDO message has to have been declared first with
         * declareRPDOMessage
         *
         * Which messages should be sent at what frequency is a matter of
         * documentation of the actual device driver.
         */
        canbus::Message getRPDOMessage(unsigned int pdoIndex);

    protected:
        StateMachine mCANOpen;
    };

    #define CANOPEN_SDO_UPDATE_CASE(object, update_id) \
        case (static_cast<uint32_t>(object::OBJECT_ID) << 8 | object::OBJECT_SUB_ID): \
            update |= update_id; \
            break;

}

#endif