include_directories(${PROJECT_SOURCE_DIR}/src)

add_executable(check_skilobot check_skilobot.c ../skilobot.c ../kbapi.c)

if(APPLE)
    target_link_libraries(check_skilobot check m)
else(APPLE)
    target_link_libraries(check_skilobot check pthread rt m)
endif()

add_test(check_skilobot check_skilobot)
