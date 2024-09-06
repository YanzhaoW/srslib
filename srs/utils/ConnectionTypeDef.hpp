#pragma once

#include <asio.hpp>

namespace srs
{
    class Control;

    using udp = asio::ip::udp;

    template <typename T>
    struct is_shared_ptr : std::false_type
    {
    };
    template <typename T>
    struct is_shared_ptr<std::shared_ptr<T>> : std::true_type
    {
    };

    template <typename T>
    concept SharedPtr = is_shared_ptr<T>::value and requires(T obj) { obj->get_name(); };

    struct ConnectionInfo
    {
        explicit ConnectionInfo(Control* control_ptr)
            : control{ control_ptr }
        {
        }
        Control* control = nullptr;
        int local_port_number = 0;
        udp::endpoint* endpoint = nullptr;
    };

} // namespace srs
