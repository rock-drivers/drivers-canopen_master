#include <iostream>
#include <canbus.hh>
#include <memory>
#include <iodrivers_base/Driver.hpp>
#include <canopen_master/StateMachine.hpp>
#include <string>
#include <iomanip>
#include <stdexcept>

using namespace std;
using namespace canopen_master;

int usage()
{
    cout << "canopen_ctl CAN_DEVICE CAN_DEVICE_TYPE CAN_ID COMMAND\n";
    cout << "  all objects ID and SUB_ID are in hexadecimal without a 0x prefix,\n";
    cout << "  as e.g. 103f 0\n";
    cout << "\n";
    cout << "  state-get # get the node state using heartbeats\n";
    cout << "  state-get --query # get the node state using a node guard\n";
    cout << "  state-set STATE # change the node state. Valid transitions are:\n";
    cout << "      START STOP ENTER_PRE_OPERATIONAL RESET RESET_COMMUNICATION\n";
    cout << "  sdo-get ID SUB_ID # get a SDO object\n";
    cout << "  sdo-set ID SUB_ID B0 B1 B2 B3 # set a SDO object,\n";
    cout << "        bytes are in hex as e.g. FF\n";
    cout << "  read # read one CAN message and display it\n";
    cout << endl;
    return 1;
}

template<typename T>
struct TextMapping
{
    std::string text;
    T value;
};
template<typename T>
static T fromText(TextMapping<T> const* mappings, std::string const& text)
{
    for (int i = 0; !mappings[i].text.empty(); ++i) {
        if (mappings[i].text == text)
            return mappings[i].value;
    }

    string known_text = mappings[0].text;
    for (int i = 1; !mappings[i].text.empty(); ++i) {
        known_text += ", " + mappings[i].text;
    }
    throw std::invalid_argument(text + " is unknown, valid values are: " + known_text);
}
template<typename T>
static string toText(TextMapping<T> const* mappings, T value)
{
    for (int i = 0; !mappings[i].text.empty(); ++i) {
        if (mappings[i].value == value)
            return mappings[i].text;
    }
    throw std::runtime_error("no text for given value");
}

static TextMapping<NODE_STATE_TRANSITION> TEXT_TO_TRANSITION_MAPPING[] = {
    { string("START"), NODE_START },
    { string("STOP"), NODE_STOP },
    { string("ENTER_PRE_OPERATIONAL"), NODE_ENTER_PRE_OPERATIONAL },
    { string("RESET"), NODE_RESET },
    { string("RESET_COMMUNICATION"), NODE_RESET_COMMUNICATION },
    { string(), NODE_START }
};
static TextMapping<NODE_STATE> TEXT_TO_STATE_MAPPING[] = {
    { string("INITIALIZING"), NODE_INITIALIZING },
    { string("STOPPED"), NODE_STOPPED },
    { string("OPERATIONAL"), NODE_OPERATIONAL },
    { string("PRE_OPERATIONAL"), NODE_PRE_OPERATIONAL },
    { string(), NODE_INITIALIZING }
};

struct DisplayStats
{
    iodrivers_base::Driver* driver;

    DisplayStats(iodrivers_base::Driver* driver)
        : driver(driver) {}

    ~DisplayStats() {
        if (driver) {
            auto status = driver->getStatus();
            std::cerr << "tx=" << status.tx << " good_rx=" << status.good_rx << " bad_rx=" << status.bad_rx << std::endl;
        }
    }
};

int main(int argc, char** argv)
{
    if (argc < 5) {
        return usage();
    }

    std::string can_device(argv[1]);
    std::string can_device_type(argv[2]);
    int8_t node_id(stoi(argv[3]));
    std::string cmd(argv[4]);

    unique_ptr<canbus::Driver> device(canbus::openCanDevice(can_device, can_device_type));
    DisplayStats stats(dynamic_cast<iodrivers_base::Driver*>(device.get()));
    device->setReadTimeout(2000);
    StateMachine canopen(node_id);

    if (cmd == "state-get") {
        if (argc != 5 && argc != 6)
            return usage();

        // There are two possible ways to query the node state. Either explicitely
        // with the NMT Guard message, or implicitly by asking for a periodic
        // heartbeat. Nodes have to support one OR the other.
        bool use_state_query = (argc == 6 && string(argv[5]) == "--query");

        if (use_state_query)
            device->write(canopen.queryState());
        else
        {
            uint8_t data[8] = { 10, 0, 0 };
            canbus::Message download = canopen.download(0x1017, 0, data, 3);
            device->write(download);
        }

        while(true) {
            canbus::Message msg = device->read();
            auto update = canopen.process(msg);
            if (update.mode == StateMachine::PROCESSED_HEARTBEAT) {
                std::cout << toText(TEXT_TO_STATE_MAPPING, canopen.getState()) << std::endl;
                break;
            }
        }

        if (!use_state_query) {
            uint8_t data[8] = { 0, 0, 0 };
            canbus::Message download = canopen.download(0x1017, 0, data, 3);
            device->write(download);
        }
    }
    else if (cmd == "state-set") {
        if (argc != 6)
            return usage();

        auto transition = fromText(TEXT_TO_TRANSITION_MAPPING, argv[5]);
        device->write(canopen.queryStateTransition(transition));
    }
    else if (cmd == "sdo-get") {
        if (argc != 7)
            return usage();

        int16_t objectId(std::stol(argv[5], nullptr, 16));
        int16_t subId(stoi(argv[6]));

        canbus::Message upload = canopen.upload(objectId, subId);
        device->write(upload);
        while(true) {
            canbus::Message msg = device->read();
            auto update = canopen.process(msg);
            if (update.hasUpdatedObject(objectId, subId)) {
                uint8_t buffer[256];
                int size = canopen.get(objectId, subId, buffer, 256);
                for (int i = 0; i < size; ++i) {
                    std::cout << " " << hex << (int)buffer[i];
                }
                std::cout << std::endl;
                break;
            }
        }
    }
    else if (cmd == "sdo-set") {
        if (argc < 7)
            return usage();

        int16_t objectId(std::stol(argv[5], nullptr, 16));
        int16_t subId(stoi(argv[6]));
        uint8_t data[8];
        int size = argc - 7;
        for (int i = 0; i < size; ++i)
            data[i] = std::stol(argv[7 + i], nullptr, 16);

        canbus::Message download = canopen.download(objectId, subId, data, size);
        device->write(download);
        while(true)
        {
            canbus::Message msg = device->read();
            auto update = canopen.process(msg);
            if (update.mode == canopen_master::StateMachine::PROCESSED_SDO_INITIATE_DOWNLOAD)
                break;
            else
                std::cout << "unexpected message with mode " << update.mode << std::endl;
        }
    }
    else if (cmd == "sync") {
        canbus::Message msg = canopen.sync();
        device->write(msg);
    }
    else if (cmd == "read") {
        device->setReadTimeout(2000);
        canbus::Message msg = device->read();
        std::cout << msg.time << " size=" << (int)msg.size;
        for (int i = 0; i < msg.size; ++i)
            std::cout << " " << hex << (int)msg.data[i];
        std::cout << std::endl;
    }
    else {
        std::cerr << "unknown subcommand " + cmd << std::endl;
        return usage();
    }
    return 0;
}
