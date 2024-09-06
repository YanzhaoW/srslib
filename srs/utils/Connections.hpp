#pragma once

#include "ConnectionBase.hpp"

namespace srs
{

    class DataProcessor;

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
            const auto data = std::vector<EntryType>{ 0, 15, 1 };
            communicate(data, NULL_ADDRESS);
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
        DataReader(ConnectionInfo info, DataProcessor* processor)
            : data_processor_{ processor }
            , ConnectionBase(info, "DataReader")
        {
        }

        void start()
        {
            get_control().set_status_is_reading(true);
            listen(true);
        }
        void end_of_read();

        void read_data_handle(std::span<char> read_data);

      private:
        gsl::not_null<DataProcessor*> data_processor_;
    };

} // namespace srs
