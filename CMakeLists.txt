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

file(GLOB QCOMMON_INCLUDES src/qcommon/*.h)
file(GLOB QCOMMON_SOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} src/qcommon/*.c)
list(REMOVE_ITEM QCOMMON_SOURCES src/qcommon/vm_x86.c)

file(GLOB RENDERER_INCLUDES src/renderer/*.h)
file(GLOB RENDERER_SOURCES src/renderer/*.c)

file(GLOB SERVER_INCLUDES src/server/*.h)
file(GLOB SERVER_SOURCES src/server/*.c)

file(GLOB SPLINES_INCLUDES src/splines/*.h)
file(GLOB SPLINES_SOURCES src/splines/*.cpp)

file(GLOB UI_INCLUDES src/ui/*.h)
file(GLOB UI_SOURCES src/ui/*.c)

file(GLOB SDL_INCLUDES src/sdl/*.h)
file(GLOB SDL_SOURCES src/sdl/*.c)

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
