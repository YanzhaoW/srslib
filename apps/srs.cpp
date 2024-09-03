#include <fmt/format.h>
#include <srs/Control.hpp>
#include <thread>

auto main() -> int
{
    asio::io_context io_context;

    auto control = srs::Control{ io_context };
    auto thread = std::jthread{ [&io_context, &control]()
                                {
                                    auto work = asio::make_work_guard(io_context);
                                    asio::signal_set signals(io_context, SIGINT, SIGTERM);
                                    signals.async_wait(
                                        [&work, &io_context, &control](auto, auto)
                                        {
                                            fmt::print("Switching it off\n");
                                            control.switch_off();
                                            work.reset();
                                            // io_context.stop();
                                        });
                                    io_context.run();
                                } };

    // asio::signal_set signals(io_context, SIGINT, SIGTERM);

    control.set_remote_endpoint("10.0.0.2", 6600);
    // control.set_remote_endpoint("127.0.0.1", 6600);
    // control.set_remote_endpoint("10.20.4.17", 6006);
    // control.switch_on();

    control.switch_on();

    fmt::print("Waiting for all processes finished\n");

    return 0;
}
