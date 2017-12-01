#include <canopen_master/PDOMapping.hpp>
#include <canopen_master/Exceptions.hpp>

using namespace canopen_master;

void PDOMapping::add(uint16_t objectId, uint8_t subId, uint8_t size)
{
    if (currentSize + size > 8)
        throw PDOMappingTooBig();

    currentSize += size;
    mappings.push_back(MappedObject { objectId, subId, size });
}
