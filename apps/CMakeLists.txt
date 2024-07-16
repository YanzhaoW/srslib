add_executable(srscli srscli.c)
target_link_libraries(srscli PRIVATE srslib)

add_executable(srsread srsread.c)
target_link_libraries(srsread PRIVATE srslib)
