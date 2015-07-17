add_library(sim skilobot.c display.c kbapi.c params.c stateio.c runsim.c distribution.c gfx/SDL_framerate.c gfx/SDL_gfxPrimitives.c gfx/SDL_gfxBlitFunc.c gfx/SDL_rotozoom.c)
add_library(headless skilobot.c kbapi.c params.c stateio.c distribution.c headless.c)

if(CMAKE_COMPILER_IS_GNUCXX)
    add_definitions(-std=c99)
    add_definitions("-Wall -O2 -g")
endif()

INSTALL(TARGETS sim headless
  ARCHIVE DESTINATION lib
)

INSTALL(FILES kilolib.h message.h message_crc.h params.h
	DESTINATION include
)

add_subdirectory(tests)
