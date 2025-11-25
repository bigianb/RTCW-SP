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
	src/botlib/be_aas_bspq3.c
	src/botlib/be_aas_cluster.c
	src/botlib/be_aas_debug.c
	src/botlib/be_aas_entity.c
	src/botlib/be_aas_file.c
	src/botlib/be_aas_main.c
	src/botlib/be_aas_move.c
	src/botlib/be_aas_optimize.c
	src/botlib/be_aas_reach.c
	src/botlib/be_aas_route.c
	src/botlib/be_aas_routealt.c
	src/botlib/be_aas_routetable.c
	src/botlib/be_aas_sample.c
	src/botlib/be_ai_char.c
	src/botlib/be_ai_chat.c
	src/botlib/be_ai_gen.c
	src/botlib/be_ai_goal.c
	src/botlib/be_ai_move.c
	src/botlib/be_ai_weap.c
	src/botlib/be_ai_weight.c
	src/botlib/be_ea.c
	src/botlib/be_interface.c
	src/botlib/l_crc.c
	src/botlib/l_libvar.c
	src/botlib/l_log.c
	src/botlib/l_memory.c
	src/botlib/l_precomp.c
	src/botlib/l_script.c
	src/botlib/l_struct.c
)

set(CGAME_INCLUDES 
	src/cgame/cg_local.h
	src/cgame/cg_public.h
	src/cgame/tr_types.h
)

set(CGAME_SOURCES 
	src/cgame/cg_consolecmds.c
	src/cgame/cg_draw.c
	src/cgame/cg_drawtools.c
	src/cgame/cg_effects.c
	src/cgame/cg_ents.c
	src/cgame/cg_event.c
	src/cgame/cg_flamethrower.c
	src/cgame/cg_info.c
	src/cgame/cg_localents.c
	src/cgame/cg_main.c
	src/cgame/cg_marks.c
	src/cgame/cg_newDraw.c
	src/cgame/cg_particles.c
	src/cgame/cg_players.c
	src/cgame/cg_playerstate.c
	src/cgame/cg_predict.c
	src/cgame/cg_servercmds.c
	src/cgame/cg_snapshot.c
	src/cgame/cg_sound.c
	src/cgame/cg_syscalls.c
	src/cgame/cg_trails.c
	src/cgame/cg_view.c
	src/cgame/cg_weapons.c
)

set(CLIENT_INCLUDES 
	src/client/client.h
	src/client/keys.h
	src/client/snd_local.h
	src/client/snd_public.h
)

set(CLIENT_SOURCES 
	src/client/cl_cgame.c
	src/client/cl_cin.c
	src/client/cl_console.c
	src/client/cl_input.c
	src/client/cl_keys.c
	src/client/cl_main.c
	src/client/cl_net_chan.c
	src/client/cl_parse.c
	src/client/cl_scrn.c
	src/client/cl_ui.c
	src/client/snd_adpcm.c
	src/client/snd_dma.c
	src/client/snd_mem.c
	src/client/snd_mix.c
	src/client/snd_wavelet.c
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
	src/game/g_team.h
	src/game/surfaceflags.h
)

set(GAME_SOURCES 
	src/game/ai_cast.c
	src/game/ai_cast_characters.c
	src/game/ai_cast_debug.c
	src/game/ai_cast_events.c
	src/game/ai_cast_fight.c
	src/game/ai_cast_func_attack.c
	src/game/ai_cast_func_boss1.c
	src/game/ai_cast_funcs.c
	src/game/ai_cast_script.c
	src/game/ai_cast_script_actions.c
	src/game/ai_cast_script_ents.c
	src/game/ai_cast_sight.c
	src/game/ai_cast_think.c
	src/game/bg_animation.c
	src/game/bg_misc.c
	src/game/bg_pmove.c
	src/game/bg_slidemove.c
	src/game/g_active.c
	src/game/g_alarm.c
	src/game/g_bot.c
	src/game/g_client.c
	src/game/g_cmds.c
	src/game/g_combat.c
	src/game/g_items.c
	src/game/g_main.c
	src/game/g_mem.c
	src/game/g_misc.c
	src/game/g_missile.c
	src/game/g_mover.c
	src/game/g_props.c
	src/game/g_save.c
	src/game/g_script.c
	src/game/g_script_actions.c
	src/game/g_session.c
	src/game/g_spawn.c
	src/game/g_svcmds.c
	src/game/g_syscalls.c
	src/game/g_target.c
	src/game/g_team.c
	src/game/g_tramcar.c
	src/game/g_trigger.c
	src/game/g_utils.c
	src/game/g_weapon.c
	src/game/q_math.c
	src/game/q_shared.c
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
