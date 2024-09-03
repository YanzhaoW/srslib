#include <srs/Control.hpp>

auto main() -> int
{
    asio::io_context io_context;
    auto control = srs::Control{ io_context };
    control.set_remote_endpoint("10.0.0.2", 6600);
    // control.set_remote_endpoint("127.0.0.1", 6600);
    // control.set_remote_endpoint("10.20.4.17", 6006);
    control.switch_on();

    io_context.run();

    return 0;
}
