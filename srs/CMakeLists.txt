add_library(srslib SHARED)

target_sources(srslib PRIVATE fec.c udpsocket.c util.c)

target_sources(
    srslib
    PRIVATE FILE_SET
            HEADERS
            FILES
            fec.h
            udp_socket.h
            util.h
            vmm3_common_data.h
            vmm3_srs_header.h
            vmm3_srs_hit.h)
target_include_directories(srslib PUBLIC ${CMAKE_SOURCE_DIR})
