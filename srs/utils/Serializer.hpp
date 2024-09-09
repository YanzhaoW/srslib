#pragma once
#include <fmt/ostream.h>

#include "CommonDefitions.hpp"
#include <asio/buffer.hpp>
// #include <cista.h>
#include <zpp_bits.h>

namespace srs
{

    class MsgBuffer
    {
      public:
        MsgBuffer() = default;

        auto serialize(auto&&... structs)
        {
            auto serialize_to = zpp::bits::out{ data_, zpp::bits::endian::network{}, zpp::bits::no_size{} };
            auto size = data_.size();
            serialize_to.position() += sizeof(BufferElementType) * size;

            // cista::buf<WriteBufferType&> serializer{ buffer };
            serialize_multi(serialize_to, std::forward<decltype(structs)>(structs)...);
            return asio::buffer(data_);
        }

        auto deserialize(auto&&... structs) {}

        [[nodiscard]] auto data() const -> const auto& { return data_; }

        void clear() { data_.clear(); }

      private:
        BufferType data_;

        void serialize_multi(auto&& serializer_to, auto&& data)
        {
            // cista::serialize<cista::mode::SERIALIZE_BIG_ENDIAN>(serializer, std::forward<decltype(data)>(data));
            serializer_to(data).or_throw();
        }

        void serialize_multi(auto&& serializer_to, auto&& data_head, auto&&... data_tail)
        {
            serialize_multi(serializer_to, std::forward<decltype(data_head)>(data_head));
            serialize_multi(serializer_to, std::forward<decltype(data_tail)>(data_tail)...);
        }
    };

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
