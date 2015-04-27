include_directories(${PROJECT_SOURCE_DIR}/src)

add_executable(check_skilobot check_skilobot.c)
target_link_libraries(check_skilobot sim check)

add_test(check_skilobot check_skilobot)
