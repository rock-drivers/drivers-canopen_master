#ifndef CANOPEN_MASTER_PDO_MAPPING_HPP
#define CANOPEN_MASTER_PDO_MAPPING_HPP

#include <cstdint>
#include <vector>

namespace canopen_master
{
    /** Builder class to build PDO mappings
     *
     * The process is to (1) build the mapping either by directly specifying
     * object ID and sub-ID as well as size, or by providing an object as defined
     * by the CANOPEN_DEFINE_OBJECT macro and (2) create the PDO configuration
     * messages using StateMachine::configurePDO. Set the state machine up using
     * StateMachine::declareRPDOMapping and StateMachine::declareTPDOMapping so
     * objects get updated when PDO messages are received (in the TPDO case) and
     * StateMachine::getRPDOMessage is able to build the PDO (in the RPDO case)
     */
    struct PDOMapping
    {
        struct MappedObject
        {
            uint16_t objectId;
            uint8_t  subId;
            uint8_t  size;

            bool operator ==(MappedObject const& other) const {
                return objectId == other.objectId &&
                    subId == other.subId &&
                    size == other.size;
            }
        };

        uint8_t currentSize = 0;
        std::vector<MappedObject> mappings;

        /** Add an object to the mapping
         */
        void add(uint16_t objectId, uint8_t subId, uint8_t size);

        /** Whether this mapping is empty (contains no objects)
         */
        bool empty() const;

        /** Add an object to the mapping using a type defined with CANOPEN_DEFINE_OBJECT
         */
        template<typename T>
        void add(int offsetID = 0, int offsetSubID = 0) {
            return add(T::OBJECT_ID + offsetID, T::OBJECT_SUB_ID + offsetSubID,
                       sizeof(typename T::OBJECT_TYPE));
        }
    };
}

#endif
