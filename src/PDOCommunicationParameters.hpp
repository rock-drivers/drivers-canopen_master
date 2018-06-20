#ifndef PDO_MASTER_PDO_COMMUNICATION_PARAMETERS_HPP
#define PDO_MASTER_PDO_COMMUNICATION_PARAMETERS_HPP

#include <cstdint>
#include <base/Time.hpp>

namespace canopen_master
{
    enum TRANSMISSION_MODE
    {
        PDO_SYNCHRONOUS,
        PDO_SYNCHRONOUS_RTR_ONLY,
        PDO_ASYNCHRONOUS_RTR_ONLY,
        PDO_ASYNCHRONOUS
    };

    struct PDOCommunicationParameters
    {
        TRANSMISSION_MODE transmission_mode = PDO_SYNCHRONOUS;
        /**
         * The COB-ID of this PDO. Leave to zero to use the default ID defined
         * by the CanOpen specification
         */
        uint16_t cob_id = 0;
        /** How many SYNC between two PDOs in PDO_CYCLIC_SYNCHRONOUS
         *
         * Ignored in the other modes or when configuring a receive PDO
         */
        uint16_t sync_period = 1;
        /**
         * Minimum time between two transimssions in asynchronous modes
         *
         * Must be lower than 6.5s
         */
        base::Time inhibit_time;
        /** Timer period in ms
         *
         * When in asynchronous mode, this is a minimum period between
         * which the PDO will be triggered
         *
         * Must be lower than 65s
         */
        base::Time timer_period;
    };
}

#endif
