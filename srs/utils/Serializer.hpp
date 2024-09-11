#pragma once

#include <asio/buffer.hpp>
#include <srs/CommonDefitions.hpp>
#include <zpp_bits.h>

namespace srs
{
    namespace rng = std::ranges;

    class SerializableMsgBuffer
    {
      public:
        SerializableMsgBuffer() = default;
        explicit SerializableMsgBuffer(std::span<BufferElementType> read_data)
        {
            data_.reserve(read_data.size());
            std::copy(read_data.begin(), read_data.end(), std::back_inserter(data_));
        }

        auto serialize(auto&&... structs)
        {
            auto serialize_to = zpp::bits::out{ data_, zpp::bits::endian::network{}, zpp::bits::no_size{} };
            auto size = data_.size();
            serialize_to.position() += sizeof(BufferElementType) * size;

            // cista::buf<WriteBufferType&> serializer{ buffer };
            // serialize_multi(serialize_to, std::forward<decltype(structs)>(structs)...);
            serialize_to(std::forward<decltype(structs)>(structs)...).or_throw();
            return asio::buffer(data_);
        }

        template <typename T>
        auto deserialize(auto&& header, std::vector<T>& body)
        {
            auto deserialize_to = zpp::bits::in{ data_, zpp::bits::endian::network{}, zpp::bits::no_size{} };

            auto read_bytes = data_.size() * sizeof(BufferElementType);
            constexpr auto header_bytes = sizeof(header);
            constexpr auto element_bytes = sizeof(T);
            auto vector_size = (read_bytes - header_bytes) / element_bytes;
            if (vector_size < 0)
            {
                throw std::runtime_error("Deserialization: Wrong header type!");
            }
            body.resize(vector_size);
            rng::fill(body, 0);

            deserialize_to(header, body).or_throw();
        }

        [[nodiscard]] auto data() const -> const auto& { return data_; }

        void clear() { data_.clear(); }

      private:
        WriteBufferType data_;

        // void serialize_multi(auto&& serializer_to, auto&& data)
        // {
        //     // cista::serialize<cista::mode::SERIALIZE_BIG_ENDIAN>(serializer, std::forward<decltype(data)>(data));
        //     serializer_to(data).or_throw();
        // }

        // void serialize_multi(auto&& serializer_to, auto&& data_head, auto&&... data_tail)
        // {
        //     serialize_multi(serializer_to, std::forward<decltype(data_head)>(data_head));
        //     serialize_multi(serializer_to, std::forward<decltype(data_tail)>(data_tail)...);
        // }
    };
} // namespace srs
