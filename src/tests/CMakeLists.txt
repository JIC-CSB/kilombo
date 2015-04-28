include_directories(${PROJECT_SOURCE_DIR}/src)

add_executable(check_skilobot check_skilobot.c ../skilobot.c ../kbapi.c)
target_link_libraries(check_skilobot check m jansson SDL)

add_test(check_skilobot check_skilobot)
