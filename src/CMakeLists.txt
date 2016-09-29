add_library(sim display.c skilobot.c kbapi.c params.c stateio.c runsim.c neighbors.c distribution.c gfx/SDL_framerate.c gfx/SDL_gfxPrimitives.c gfx/SDL_gfxBlitFunc.c gfx/SDL_rotozoom.c)

add_library(headless skilobot.c kbapi.c params.c stateio.c runsim.c neighbors.c distribution.c)
set_target_properties(headless PROPERTIES COMPILE_DEFINITIONS "SKILO_HEADLESS")
 
if(CMAKE_COMPILER_IS_GNUCXX)
    add_definitions(-std=c99)
    add_definitions("-Wall -O2 -g")
#    add_definitions("-Wall -O3 -march=native -g")	
endif()

INSTALL(TARGETS sim headless
  ARCHIVE DESTINATION lib
)

INSTALL(FILES kilombo.h DESTINATION include)

INSTALL(FILES kilolib.h message.h message_crc.h params.h skilobot.h
	DESTINATION include/kilombo)

add_subdirectory(tests)
