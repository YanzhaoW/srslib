add_executable(srscli srscli.c)
target_link_libraries(srscli PRIVATE srs)

add_executable(srsread srsread.c)
target_link_libraries(srsread PRIVATE srs)

add_executable(srs_control srs.cpp)
target_link_libraries(srs_control PRIVATE srs fmt::fmt CLI11::CLI11 spdlog::spdlog)
