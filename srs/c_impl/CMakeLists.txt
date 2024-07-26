add_library(srs_c)

target_sources(srs_c PRIVATE fec.c udpsocket.c util.c)

target_sources(
    srs_c
    PRIVATE FILE_SET
            HEADERS
            BASE_DIRS
            ${CMAKE_CURRENT_SOURCE_DIR}
            FILES
            fec.h
            udp_socket.h
            util.h
            vmm3_common_data.h
            vmm3_srs_header.h
            vmm3_srs_hit.h)
