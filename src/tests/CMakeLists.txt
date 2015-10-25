include_directories(${PROJECT_SOURCE_DIR}/src)
link_directories(/usr/local/lib)
include_directories(/usr/local/include)


add_executable(check_skilobot check_skilobot.c ../skilobot.c ../kbapi.c ../neighbors.c)


if(APPLE)
    target_link_libraries(check_skilobot check m)
else(APPLE)
    target_link_libraries(check_skilobot check pthread rt m)
endif()

add_test(check_skilobot check_skilobot)
