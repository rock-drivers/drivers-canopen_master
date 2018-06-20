#ifndef CANOPEN_MASTER_PDO_MAPPING_HPP
#define CANOPEN_MASTER_PDO_MAPPING_HPP

#include <cstdint>
#include <vector>

namespace canopen_master
{
    struct PDOMapping
    {
        struct MappedObject
        {
            uint16_t objectId;
            uint8_t  subId;
            uint8_t  size;
        };

        uint8_t currentSize = 0;
        std::vector<MappedObject> mappings;
        void add(uint16_t objectId, uint8_t subId, uint8_t size);

        bool empty() const;
    };
}

#endif
