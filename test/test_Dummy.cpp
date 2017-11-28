#include <boost/test/unit_test.hpp>
#include <canopen_protocol/Dummy.hpp>

using namespace canopen_protocol;

BOOST_AUTO_TEST_CASE(it_should_not_crash_when_welcome_is_called)
{
    canopen_protocol::DummyClass dummy;
    dummy.welcome();
}
