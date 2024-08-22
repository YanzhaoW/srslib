#include <srs/Control.hpp>

auto main() -> int
{

    auto control = srs::Control{};
    control.set_remote_endpoint("10.0.0.2", 6006);
    // control.set_remote_endpoint("localhost", 6007);
    // control.set_remote_endpoint("10.20.4.17", 6006);
    control.switch_on();

    control.run();

    return 0;
}
