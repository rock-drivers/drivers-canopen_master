#ifndef CANOPEN_MASTER_UPDATE_HPP
#define CANOPEN_MASTER_UPDATE_HPP

#include <cstdint>

namespace canopen_master {
    /**
     * Class used to represent the state update as returned by
     * Slave::process
     */
    class Update {
        uint32_t mAckedObjectID;
        uint32_t mAckedObjectSubID;
        uint64_t mUpdatedObjects;

    public:
        static Update Ack(int objectId, int objectSubId)
        {
            Update update;
            update.mAckedObjectID = objectId;
            update.mAckedObjectSubID = objectSubId;
            return update;
        }

        static Update UpdatedObjects(uint64_t updates)
        {
            Update update;
            update.mUpdatedObjects = updates;
            return update;

        }
        Update()
            : mAckedObjectID(0)
            , mAckedObjectSubID(0)
            , mUpdatedObjects(0) {}

        bool isAck() const
        {
            return mAckedObjectID != 0;
        }

        bool isAcked(uint16_t objectId, uint8_t objectSubID) const
        {
            return (mAckedObjectID == objectId) &&
                (mAckedObjectSubID == objectSubID);
        }

        template<typename T>
        bool isAcked(int offsetId = 0, int offsetSubId = 0) const
        {
            return this->isAcked(T::OBJECT_ID + offsetId, T::OBJECT_SUB_ID + offsetSubId);
        }

        bool hasOneUpdated(int64_t updateId) const
        {
            return (mUpdatedObjects & updateId) != 0;
        }

        bool isUpdated(uint64_t updateId) const
        {
            return (mUpdatedObjects & updateId) == updateId;
        }

        void merge(Update const& update)
        {
            mUpdatedObjects |= update.mUpdatedObjects;
        }
    };
}

#endif