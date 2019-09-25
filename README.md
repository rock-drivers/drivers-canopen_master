# drivers/canopen_master - A CANOpen master protocol implementation

This library provides the protocol layer necessary to implement a CANopen
master. Its aim is to really only provide the interpretation to/from CAN
messages and the necessary tooling to handle an object dictionary and a node
state.

Its focus is on the CANopen predefined communication set, as it turns out to be
used by most CANopen devices.

It is unlike other open-source implementations (as e.g. CAN festival) as its
aim is not to provide a full integration stack where the driver is stored deep
inside, but rather a "inside-out" protocol library. The application is
meant to interface with the CAN hardware, using this library as a mean to
interpret the CAN stream. This makes is a much better target for integration
in different "active" frameworks such as Rock

## High-level Slave API

The main entry point to create specific device drivers is the Slave class.
This class provides a higher-level API on top of the basic CANOpen state
machine and object dictionary.

The general process is to

1. declare objects. They are represented as C++ types.
2. create a high-level API specific to your device that interacts with
   the object dictionary and/or the remote device

All messages received from the device should be passed to `Slave::process` so that the information they contain is saved in the object dictionary. The value
returned by `process` allows you to query what was received, and act accordingly.

### Integration

Methods in `Slave` that interact with the remote device return vectors of
`canbus::Message`. These messages are meant to be sent and acked one-by-one.
They are all SDO messages. The general process is:

~~~ cpp
auto messages = slave.querySomething();
for (auto msg : messages) {
    device_driver.write(msg);
    do {
        // Wait for SDO ack message
    } while (slave.process(received_can_message).isAck());
}
~~~

The `Slave::process` method should be fed every message that is being
received from the CAN bus. It will ignore messages that are not meant for the
device it represents, but will process the rest and update the object
dictionary accordingly.

### Interacting with the object dictionary

The Slave class maintains a representation of the "best known" state of
the device's object dictionary. Two methods, `setRaw` and `getRaw` allow
to access this local view to the dictionary.

In addition, the class allows to interact with the remote device by querying
for a download/upload. Whenever a SDO download reply message is passed to
`Slave::process`, the object dictionary will be updated. This also happens
with PDOs (see next section).

Create an `Objects.hpp` file where you will define your custom objects
using the `CANOPEN_DEFINE_OBJECT` macro, as e.g.:

~~~ cpp
CANOPEN_DEFINE_RO_OBJECT(OBJECT_ID, OBJECT_SUB_ID, Name, Type)
CANOPEN_DEFINE_WO_OBJECT(OBJECT_ID, OBJECT_SUB_ID, Name, Type)
CANOPEN_DEFINE_RW_OBJECT(OBJECT_ID, OBJECT_SUB_ID, Name, Type)
~~~

Name is the name you will use afterwards to access the object, Type is the
data type inside the CANOpen dictionary itself. The slave class allows
you to access the **local** object dictionary - that is the currently known
values for objects in the dictionary - with

~~~ cpp
getRaw<Name>()
setRaw<Name>(Type type);
~~~

and get the SDO messages necessary to download/upload objects with

~~~ cpp
queryUpload<Name>();
queryDownloadRaw<Name>(Type type);
~~~

In addition, one can download the value currently stored in the local
object dictionary with

~~~ cpp
queryDownload<Name>();
~~~~

### PDOs

`Slave` provides a way to setup PDOs and handle them relatively transparently.

First, one has to create a PDO mapping using the `PDOMapping` class:

~~~ cpp
PDOMapping pdo;
pdo.add<Voltage>();
pdo.add<Current>();
~~~

From there, call `m_can_open.configurePDO` to get the SDO messages that will set up the PDO.
Call either `m_can_open.declareRPDOMapping` or `m_can_open.declareTPDOMapping` to let the
object know the mapping between PDOs and objects in the dictionary. It will ensure that:

- you can build the RPDOs using `getRPDOMessage` and send them on the bus
- `Slave::process` automatically updates the object dictionary when it receives
  a TPDO message.
