#pragma once

#include "ConnectionBase.hpp"

namespace srs
{
    class Starter : public ConnectionBase<>
    {
      public:
        explicit Starter(ConnectionInfo info)
            : ConnectionBase(info, "Starter")
        {
        }

        void end_of_read();
        void acq_on()
        {
            spdlog::critical("passing here 5 ...");
            const auto data = std::vector<EntryType>{ 0, 15, 1 };
            spdlog::critical("passing here 6 ...");
            communicate(data, NULL_ADDRESS);
            spdlog::critical("passing here 7 ...");
        }
    };

    class Stopper : public ConnectionBase<>
    {
      public:
        explicit Stopper(ConnectionInfo info)
            : ConnectionBase(info, "Stopper")
        {
        }

        void acq_off();
    };

    class DataReader : public ConnectionBase<LARGE_READ_MSG_BUFFER_SIZE>
    {
      public:
        explicit DataReader(ConnectionInfo info)
            : ConnectionBase(info, "DataReader")
        {
        }

        void start()
        {
            get_control().set_status_is_reading(true);
            listen(true);
        }
        void end_of_read();

        void read_data_handle(std::span<char> read_data)
        {
            // std::copy(read_data.begin(), read_data.end(), std::back_inserter(data_queue_));
        }

      private:
        std::deque<char> data_queue_;
    };

} // namespace srs
