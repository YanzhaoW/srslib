add_subdirectory(c_impl)

add_library(srs_cpp SHARED)

target_sources(srs_cpp PRIVATE App.cpp utils/Connections.cpp analysis/DataProcessor.cpp
                               analysis/DataStructs.cpp)

target_sources(
    srs_cpp
    PRIVATE FILE_SET
            HEADERS
            BASE_DIRS
            ${CMAKE_CURRENT_SOURCE_DIR}
            FILES
            utils/Serializer.hpp
            utils/CommonDefitions.hpp
            devices/Fec.hpp
            devices/VmmDevice.hpp
            App.hpp)

target_include_directories(
    srs_cpp
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/devices ${CMAKE_CURRENT_SOURCE_DIR}/utils
            ${CMAKE_CURRENT_SOURCE_DIR}/analysis
    PUBLIC ${CMAKE_SOURCE_DIR})

target_link_libraries(
    srs_cpp
    PUBLIC asio::asio gsl::gsl-lite
    PRIVATE fmt::fmt zpp_bits::zpp_bits spdlog::spdlog onetbb::onetbb
    INTERFACE srs_c)
