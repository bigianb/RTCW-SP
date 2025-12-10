cmake_minimum_required(VERSION 3.15)
option(ENABLE_TESTS "Enables building the unit tests" ON)

project(WolfSP)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

find_package(OpenGL REQUIRED COMPONENTS OpenGL)
find_package(JPEG)
find_package(SDL3 REQUIRED)
if(ENABLE_TESTS)
	find_package(Catch2 REQUIRED)
endif()

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
	#set(CMAKE_OSX_DEPLOYMENT_TARGET 11.0)
endif()


set(BOTAI_INCLUDES 
	src/botai/ai_dmnet.h
	src/botai/ai_dmq3.h
	src/botai/ai_main.h
	src/botai/botai.h
	src/botai/chars.h
	src/botai/inv.h
	src/botai/match.h
	src/botai/syn.h
)

set(BOTAI_SOURCES 
	src/botai/ai_dmnet.cpp
	src/botai/ai_dmq3.cpp
	src/botai/ai_main.cpp
)

set(BOTLIB_INCLUDES
	src/botlib/aasfile.h
	src/botlib/be_aas.h
	src/botlib/be_aas_bsp.h
	src/botlib/be_aas_cluster.h
	src/botlib/be_aas_debug.h
	src/botlib/be_aas_def.h
	src/botlib/be_aas_entity.h
	src/botlib/be_aas_file.h
	src/botlib/be_aas_funcs.h
	src/botlib/be_aas_main.h
	src/botlib/be_aas_move.h
	src/botlib/be_aas_optimize.h
	src/botlib/be_aas_reach.h
	src/botlib/be_aas_route.h
	src/botlib/be_aas_routealt.h
	src/botlib/be_aas_routetable.h
	src/botlib/be_aas_sample.h
	src/botlib/be_ai_weight.h
	src/botlib/be_interface.h
	src/botlib/botlib.h
	src/botlib/l_crc.h
	src/botlib/l_libvar.h
	src/botlib/l_log.h
	src/botlib/l_memory.h
	src/botlib/l_precomp.h
	src/botlib/l_script.h
	src/botlib/l_struct.h
	src/botlib/l_utils.h
)

set(BOTLIB_SOURCES
	src/botlib/be_aas_bspq3.cpp
	src/botlib/be_aas_cluster.cpp
	src/botlib/be_aas_debug.cpp
	src/botlib/be_aas_entity.cpp
	src/botlib/be_aas_file.cpp
	src/botlib/be_aas_main.cpp
	src/botlib/be_aas_move.cpp
	src/botlib/be_aas_optimize.cpp
	src/botlib/be_aas_reach.cpp
	src/botlib/be_aas_route.cpp
	src/botlib/be_aas_routealt.cpp
	src/botlib/be_aas_routetable.cpp
	src/botlib/be_aas_sample.cpp
	src/botlib/be_ai_char.cpp
	src/botlib/be_ai_chat.cpp
	src/botlib/be_ai_gen.cpp
	src/botlib/be_ai_goal.cpp
	src/botlib/be_ai_move.cpp
	src/botlib/be_ai_weap.cpp
	src/botlib/be_ai_weight.cpp
	src/botlib/be_ea.cpp
	src/botlib/be_interface.cpp
	src/botlib/l_crc.cpp
	src/botlib/l_libvar.cpp
	src/botlib/l_log.cpp
	src/botlib/l_memory.cpp
	src/botlib/l_precomp.cpp
	src/botlib/l_script.cpp
	src/botlib/l_struct.cpp
)

set(CGAME_INCLUDES 
	src/cgame/cg_local.h
	src/cgame/cg_public.h
	src/cgame/tr_types.h
)

set(CGAME_SOURCES 
	src/cgame/cg_consolecmds.cpp
	src/cgame/cg_draw.cpp
	src/cgame/cg_drawtools.cpp
	src/cgame/cg_effects.cpp
	src/cgame/cg_ents.cpp
	src/cgame/cg_event.cpp
	src/cgame/cg_flamethrower.cpp
	src/cgame/cg_info.cpp
	src/cgame/cg_localents.cpp
	src/cgame/cg_main.cpp
	src/cgame/cg_marks.cpp
	src/cgame/cg_newDraw.cpp
	src/cgame/cg_particles.cpp
	src/cgame/cg_players.cpp
	src/cgame/cg_playerstate.cpp
	src/cgame/cg_predict.cpp
	src/cgame/cg_servercmds.cpp
	src/cgame/cg_snapshot.cpp
	src/cgame/cg_sound.cpp
	src/cgame/cg_syscalls.cpp
	src/cgame/cg_trails.cpp
	src/cgame/cg_view.cpp
	src/cgame/cg_weapons.cpp
)

set(CLIENT_INCLUDES 
	src/client/client.h
	src/client/keys.h
	src/client/snd_local.h
	src/client/snd_public.h
)

set(CLIENT_SOURCES 
	src/client/cl_cgame.cpp
	src/client/cl_cin.cpp
	src/client/cl_console.cpp
	src/client/cl_input.cpp
	src/client/cl_keys.cpp
	src/client/cl_main.cpp
	src/client/cl_net_chan.cpp
	src/client/cl_parse.cpp
	src/client/cl_scrn.cpp
	src/client/cl_ui.cpp
	src/client/snd_adpcm.cpp
	src/client/snd_dma.cpp
	src/client/snd_mem.cpp
	src/client/snd_mix.cpp
	src/client/snd_wavelet.cpp
)

set(GAME_INCLUDES 
	src/game/ai_cast.h 
	src/game/ai_cast_fight.h
	src/game/ai_cast_global.h  
	src/game/be_aas.h
	src/game/be_ai_char.h
	src/game/be_ai_chat.h 
	src/game/be_ai_gen.h        
	src/game/be_ai_goal.h    
	src/game/be_ai_move.h           
	src/game/be_ai_weap.h
	src/game/be_ea.h       
	src/game/bg_local.h      
	src/game/bg_public.h     
	src/game/botlib.h         
	src/game/g_func_decs.h     
	src/game/g_funcs.h      
	src/game/g_local.h    
	src/game/g_public.h    
	src/game/q_shared.h            
	src/game/surfaceflags.h
)

set(GAME_SOURCES 
	src/game/ai_cast.cpp
	src/game/ai_cast_characters.cpp
	src/game/ai_cast_debug.cpp
	src/game/ai_cast_events.cpp
	src/game/ai_cast_fight.cpp
	src/game/ai_cast_func_attack.cpp
	src/game/ai_cast_func_boss1.cpp
	src/game/ai_cast_funcs.cpp
	src/game/ai_cast_script.cpp
	src/game/ai_cast_script_actions.cpp
	src/game/ai_cast_script_ents.cpp
	src/game/ai_cast_sight.cpp
	src/game/ai_cast_think.cpp
	src/game/bg_animation.cpp
	src/game/bg_misc.cpp
	src/game/bg_pmove.cpp
	src/game/bg_slidemove.cpp
	src/game/g_active.cpp
	src/game/g_alarm.cpp
	src/game/g_bot.cpp
	src/game/g_client.cpp
	src/game/g_cmds.cpp
	src/game/g_combat.cpp
	src/game/g_items.cpp
	src/game/g_main.cpp
	src/game/g_mem.cpp
	src/game/g_misc.cpp
	src/game/g_missile.cpp
	src/game/g_mover.cpp
	src/game/g_props.cpp
	src/game/g_save.cpp
	src/game/g_script.cpp
	src/game/g_script_actions.cpp
	src/game/g_session.cpp
	src/game/g_spawn.cpp
	src/game/g_svcmds.cpp
	src/game/g_syscalls.cpp
	src/game/g_target.cpp
	src/game/g_tramcar.cpp
	src/game/g_trigger.cpp
	src/game/g_utils.cpp
	src/game/g_weapon.cpp
	src/game/q_math.cpp
	src/game/q_shared.cpp
)

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
	src/server/world.h
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
	src/server/world.cpp
)

set(IDLIB_INCLUDES
	src/idlib/math/Angles.h
	src/idlib/math/Math.h
	src/idlib/math/Matrix.h
	src/idlib/math/Rotation.h
	src/idlib/math/Quat.h
	src/idlib/math/Vector.h
)

set(IDLIB_SOURCES
	src/idlib/math/Angles.cpp
	src/idlib/math/Math.cpp
	src/idlib/math/Matrix.cpp
	src/idlib/math/Rotation.cpp
	src/idlib/math/Quat.cpp
	src/idlib/math/Vector.cpp
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
    ${SPLINES_SOURCES}
    ${UI_SOURCES}
    ${SDL_SOURCES}
)

add_library(idlib STATIC ${IDLIB_SOURCES} ${IDLIB_INCLUDES})
add_library(server STATIC ${SERVER_SOURCES} ${SERVER_INCLUDES})

add_executable(wolf WIN32 MACOSX_BUNDLE ${WOLF_INCLUDES} ${WOLF_SOURCES})
target_link_libraries(wolf idlib server SDL3::SDL3 OpenGL::GL JPEG::JPEG)

if(ENABLE_TESTS)
	enable_testing()
	include(CTest)
	include(Catch)
	add_subdirectory(tests)
endif()
