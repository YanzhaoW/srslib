#include <fmt/format.h>
#include <srs/Control.hpp>

auto main() -> int
{
    auto control = srs::Control{};

    control.set_remote_endpoint("10.0.0.2", 6600);

    control.switch_on();

    return 0;
}
