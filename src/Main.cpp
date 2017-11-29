#include <iostream>
#include <canopen_master/Dummy.hpp>

int main(int argc, char** argv)
{
    canopen_master::DummyClass dummyClass;
    dummyClass.welcome();

    return 0;
}
