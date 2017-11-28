#include <iostream>
#include <canopen_protocol/Dummy.hpp>

int main(int argc, char** argv)
{
    canopen_protocol::DummyClass dummyClass;
    dummyClass.welcome();

    return 0;
}
