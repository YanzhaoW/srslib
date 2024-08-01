add_subdirectory(c_impl)

add_library(srs_cpp SHARED)

find_package(asio REQUIRED)
find_package(fmt REQUIRED)
find_package(cista REQUIRED)

target_sources(srs_cpp PRIVATE Control.cpp)

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
            Control.hpp)

target_include_directories(srs_cpp PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/devices PUBLIC ${CMAKE_SOURCE_DIR})

target_link_libraries(
    srs_cpp
    PUBLIC asio::asio
    PRIVATE fmt::fmt cista::cista
    INTERFACE srs_c)
