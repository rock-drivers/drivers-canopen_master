canopen_master
=============
CANOpen master protocol implementation

This library provides the protocol layer necessary to implement a CANopen
master. Its aim is to really only provide the interpretation to/from CAN
messages and the necessary tooling to handle an object dictionary and a node
state.

Its focus is on the CANopen predefined communication set, as it turns out to be
used by most CANopen devices.

It is unlike other open-source implementations (as e.g. CAN festival) as its
aim is not to provide a full integration stack where the driver is stored deep
inside, but rather a "inside-out" protocol library. Rather, the application is
meant to interface with the CAN hardware, using this library as a mean to
interpret the CAN stream. This makes is a much better target for integration
in different "active" frameworks such as Rock
