#ifndef PDO_MASTER_PDO_COMMUNICATION_PARAMETERS_HPP
#define PDO_MASTER_PDO_COMMUNICATION_PARAMETERS_HPP

#include <cstdint>

namespace canopen_master
{
    enum TRANSMISSION_MODE
    {
        PDO_SYNC_AND_STANDARD_EVENT,
        PDO_SYNC,
        PDO_SYNC_AFTER_RTR,
        PDO_ASYNC_RTR,
        PDO_ASYNC_RTR_AND_MANUFACTURER_EVENT,
        PDO_ASYNC_RTR_AND_DEVICE_PROFILE_EVENT
    };

    struct PDOCommunicationParameters
    {
        TRANSMISSION_MODE transmission_mode = PDO_SYNC;
        /** How many SYNC between two PDOs in transmission modes * using SYNC
         */
        uint16_t sync_period = 0;
        /** Minimum time between two transimssions in multiple of 100us */
        uint16_t inhibit_time = 0;
        /** Timer period in ms */
        uint16_t timer_period = 0;
    };
}

#endif
