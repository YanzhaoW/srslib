add_executable(srscli srscli.c)
target_link_libraries(srscli PRIVATE srs)

add_executable(srsread srsread.c)
target_link_libraries(srsread PRIVATE srs)

add_executable(control control.cpp)
target_link_libraries(control PRIVATE srs)
