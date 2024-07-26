#pragma once
#include <fmt/ostream.h>

#include "CommonDefitions.hpp"
#include <asio/buffer.hpp>
#include <cista.h>

namespace srs
{
    template <typename DataType>
    auto serialize(const DataType& data, BufferType& buffer)
    {
        buffer.clear();
        cista::buf<BufferType&> serializer_{ buffer };
        cista::serialize<cista::mode::SERIALIZE_BIG_ENDIAN>(serializer_, data);
        return asio::buffer(buffer);
    }
} // namespace srs

// using cereal archive:

// #include <cereal/types/vector.hpp>
// #include <iostream>
// #include <ranges>
// #include <span>
// #include <sstream>
// #include <vector>

// // Type your code here, or load an example.
// auto main() -> int {
//     std::vector<uint8_t> data1{1};
//     std::array<uint8_t, 4> data{1, 2, 3, 4};
//     auto sstream =
//         std::stringstream{std::ios::binary | std::ios::out | std::ios::in};
//     fmt::print("string before: {:?}\n", sstream.str());
//     {
//         auto archive = cereal::PortableBinaryOutputArchive{
//             sstream, cereal::PortableBinaryOutputArchive::Options::BigEndian()};
//         archive(data, data1);
//     }
//     fmt::print("string after: {:?}\n", sstream.str());
//     auto data_span = std::span{sstream.view()};
//     fmt::print("size: {}\n", data_span.size());
//     fmt::print("{:02x}\n",
//                fmt::join(std::ranges::views::drop(data_span, 1), ""));

//     return 0;

// }
