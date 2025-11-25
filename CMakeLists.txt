cmake_minimum_required(VERSION 3.15)

project(WolfSP)
set(CMAKE_CXX_STANDARD 17)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

find_package(OpenGL REQUIRED COMPONENTS OpenGL)
find_package(JPEG)
find_package(SDL3 REQUIRED)


# Enforced colored output
if (CMAKE_COMPILER_IS_GNUCC)
	add_compile_options(-fdiagnostics-color=always)
elseif(CMAKE_C_COMPILER_ID MATCHES "Clang")
	add_compile_options(-fcolor-diagnostics)
endif ()

if(NOT MSVC)
	message(STATUS CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE})
endif()

if(APPLE)
	set(CMAKE_FIND_FRAMEWORK LAST)
	add_definitions(-DMACOS_X=1)
	set(CMAKE_OSX_DEPLOYMENT_TARGET 11.0)
endif()


file(GLOB BOTAI_INCLUDES src/botai/*.h)
file(GLOB BOTAI_SOURCES src/botai/*.c)

file(GLOB BOTLIB_INCLUDES src/botlib/*.h)
file(GLOB BOTLIB_SOURCES src/botlib/*.c)

file(GLOB CGAME_INCLUDES src/cgame/*.h)
file(GLOB CGAME_SOURCES src/cgame/*.c)

file(GLOB CLIENT_INCLUDES src/client/*.h)
file(GLOB CLIENT_SOURCES src/client/*.c)

file(GLOB GAME_INCLUDES src/game/*.h)
file(GLOB GAME_SOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} src/game/*.c)
list(REMOVE_ITEM GAME_SOURCES src/game/bg_lib.c)

set(QCOMMON_INCLUDES
	src/qcommon/cm_local.h
	src/qcommon/cm_patch.h
	src/qcommon/cm_polylib.h
	src/qcommon/cm_public.h
	src/qcommon/entity_state.h
	src/qcommon/qcommon.h
	src/qcommon/qfiles.h
	src/qcommon/unzip.h
)

set(QCOMMON_SOURCES
	src/qcommon/cm_load.cpp
	src/qcommon/cm_patch.cpp
	src/qcommon/cm_polylib.cpp
	src/qcommon/cm_test.cpp
	src/qcommon/cm_trace.cpp
	src/qcommon/cmd.cpp
	src/qcommon/common.cpp
	src/qcommon/cvar.cpp
	src/qcommon/files.cpp
	src/qcommon/huffman.cpp
	src/qcommon/md4.cpp
	src/qcommon/msg.cpp
	src/qcommon/net_chan.cpp
	src/qcommon/unzip.cpp
)

set(RENDERER_INCLUDES
	src/renderer/anorms256.h
	src/renderer/qgl_linked.h
	src/renderer/qgl.h
	src/renderer/tr_local.h
	src/renderer/tr_public.h
)

set(RENDERER_SOURCES
	src/renderer/tr_animation.cpp
	src/renderer/tr_backend.cpp
	src/renderer/tr_bsp.cpp
	src/renderer/tr_cmds.cpp
	src/renderer/tr_cmesh.cpp
	src/renderer/tr_curve.cpp
	src/renderer/tr_flares.cpp
	src/renderer/tr_font.cpp
	src/renderer/tr_image.cpp
	src/renderer/tr_init.cpp
	src/renderer/tr_light.cpp
	src/renderer/tr_local_null.cpp
	src/renderer/tr_main.cpp
	src/renderer/tr_marks.cpp
	src/renderer/tr_mesh.cpp
	src/renderer/tr_model.cpp
	src/renderer/tr_noise.cpp
	src/renderer/tr_scene.cpp
	src/renderer/tr_shade_calc.cpp
	src/renderer/tr_shade.cpp
	src/renderer/tr_shader.cpp
	src/renderer/tr_shadows.cpp
	src/renderer/tr_sky.cpp
	src/renderer/tr_surface.cpp
	src/renderer/tr_world.cpp
)

set(SERVER_INCLUDES 
	src/server/server.h
)
set(SERVER_SOURCES 
	src/server/sv_bot.cpp
	src/server/sv_ccmds.cpp
	src/server/sv_client.cpp
	src/server/sv_game.cpp
	src/server/sv_init.cpp
	src/server/sv_main.cpp
	src/server/sv_net_chan.cpp
	src/server/sv_snapshot.cpp
	src/server/sv_world.cpp
)
	

file(GLOB SPLINES_INCLUDES src/splines/*.h)
file(GLOB SPLINES_SOURCES src/splines/*.cpp)

set (UI_INCLUDES
	src/ui/keycodes.h
	src/ui/ui_local.h
	src/ui/ui_public.h
	src/ui/ui_shared.h
)
set(UI_SOURCES
	src/ui/ui_atoms.cpp
	src/ui/ui_main.cpp
	src/ui/ui_shared.cpp
)

set(SDL_INCLUDES )
set(SDL_SOURCES
	src/sdl/sdl_gamma.cpp
	src/sdl/sdl_glimp.cpp
	src/sdl/sdl_input.cpp
	src/sdl/sdl_main.cpp
	src/sdl/sdl_null.cpp
	src/sdl/sdl_snd.cpp
	src/sdl/sdl_sys.cpp)

include_directories(.)
include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(${CMAKE_SOURCE_DIR}/src/qcommon)
include_directories(${CMAKE_SOURCE_DIR}/src/renderer)
include_directories(${CMAKE_SOURCE_DIR}/src/client)


source_group("botai" FILES ${BOTAI_INCLUDES})
source_group("botai" FILES ${BOTAI_SOURCES})

source_group("botlib" FILES ${BOTLIB_INCLUDES})
source_group("botlib" FILES ${BOTLIB_SOURCES})

source_group("cgame" FILES ${CGAME_INCLUDES})
source_group("cgame" FILES ${CGAME_SOURCES})

source_group("client" FILES ${CLIENT_INCLUDES})
source_group("client" FILES ${CLIENT_SOURCES})

source_group("game" FILES ${GAME_INCLUDES})
source_group("game" FILES ${GAME_SOURCES})

source_group("qcommon" FILES ${QCOMMON_INCLUDES})
source_group("qcommon" FILES ${QCOMMON_SOURCES})

source_group("renderer" FILES ${RENDERER_INCLUDES})
source_group("renderer" FILES ${RENDERER_SOURCES})

source_group("server" FILES ${SERVER_INCLUDES})
source_group("server" FILES ${SERVER_SOURCES})

source_group("splines" FILES ${SPLINES_INCLUDES})
source_group("splines" FILES ${SPLINES_SOURCES})

source_group("ui" FILES ${UI_INCLUDES})
source_group("ui" FILES ${UI_SOURCES})

source_group("sdl" FILES ${SDL_INCLUDES})
source_group("sdl" FILES ${SDL_SOURCES})

set(WOLF_INCLUDES 
	${BOTAI_INCLUDES}
	${BOTLIB_INCLUDES}
	${CGAME_INCLUDES}
	${CLIENT_INCLUDES}
	${GAME_INCLUDES}
	${QCOMMON_INCLUDES}
	${RENDERER_INCLUDES}
	${SERVER_INCLUDES}
	${SPLINES_INCLUDES}
	${UI_INCLUDES}
	${SDL_INCLUDES}
)

set(WOLF_SOURCES 
    ${BOTAI_SOURCES}
    ${BOTLIB_SOURCES}
    ${CGAME_SOURCES}
    ${CLIENT_SOURCES}
    ${GAME_SOURCES}
    ${QCOMMON_SOURCES}
    ${RENDERER_SOURCES}
    ${SERVER_SOURCES}
    ${SPLINES_SOURCES}
    ${UI_SOURCES}
    ${SDL_SOURCES}
)

add_executable(wolf WIN32 MACOSX_BUNDLE ${WOLF_INCLUDES} ${WOLF_SOURCES})
target_link_libraries(wolf SDL3::SDL3 OpenGL::GL JPEG::JPEG)
