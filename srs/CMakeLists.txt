add_subdirectory(c_impl)

add_library(srs_cpp SHARED)

target_sources(srs_cpp PRIVATE Control.cpp Connection.cpp)

target_sources(
    srs_cpp
    PRIVATE FILE_SET
            HEADERS
            BASE_DIRS
            ${CMAKE_CURRENT_SOURCE_DIR}
            FILES
            utils/Serializer.hpp
            devices/Fec.hpp
            devices/VmmDevice.hpp
            Control.hpp
            CommonDefitions.hpp)

target_include_directories(
    srs_cpp
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/devices
    PUBLIC ${CMAKE_SOURCE_DIR})

target_link_libraries(
    srs_cpp
    PUBLIC asio::asio gsl::gsl-lite
    PRIVATE fmt::fmt cista::cista spdlog::spdlog
    INTERFACE srs_c)
