/*
===========================================================================

Return to Castle Wolfenstein single player GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company. 

This file is part of the Return to Castle Wolfenstein single player GPL Source Code (RTCW SP Source Code).  

RTCW SP Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

RTCW SP Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with RTCW SP Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the RTCW SP Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the RTCW SP Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#include "g_local.h"
#include "../server/server.h"

bool    G_SpawnString( const char *key, const char *defaultString, const char **out ) {
	int i;

	if ( !level.spawning ) {
		*out = (char *)defaultString;
	}

	for ( i = 0 ; i < level.numSpawnVars ; i++ ) {
		if ( !strcmp( key, level.spawnVars[i][0] ) ) {
			*out = level.spawnVars[i][1];
			return true;
		}
	}

	*out = (char *)defaultString;
	return false;
}

bool    G_SpawnFloat( const char *key, const char *defaultString, float *out ) {
	const char        *s;
	bool present;

	present = G_SpawnString( key, defaultString, &s );
	*out = atof( s );
	return present;
}

bool    G_SpawnInt( const char *key, const char *defaultString, int *out ) {
	const char        *s;
	bool present;

	present = G_SpawnString( key, defaultString, &s );
	*out = atoi( s );
	return present;
}

bool    G_SpawnVector( const char *key, const char *defaultString, float *out ) {
	const char        *s;
	bool present;

	present = G_SpawnString( key, defaultString, &s );
	sscanf( s, "%f %f %f", &out[0], &out[1], &out[2] );
	return present;
}



//
// fields are needed for spawning from the entity string
//
typedef enum {
	F_INT,
	F_FLOAT,
	F_LSTRING,          // string on disk, pointer in memory, TAG_LEVEL
	F_GSTRING,          // string on disk, pointer in memory, TAG_GAME
	F_VECTOR,
	F_ANGLEHACK,
	F_ENTITY,           // index on disk, pointer in memory
	F_ITEM,             // index on disk, pointer in memory
	F_CLIENT,           // index on disk, pointer in memory
	F_IGNORE
} fieldtype_t;

typedef struct
{
	const char    *name;
	intptr_t ofs;
	fieldtype_t type;
	int flags;
} gentity_field_t;

gentity_field_t fields[] = {
	{"classname",    FOFS( classname ),    F_LSTRING},
	{"origin",       FOFS( shared.s.origin ),     F_VECTOR},
	{"model",        FOFS( model ),        F_LSTRING},
	{"model2",       FOFS( model2 ),       F_LSTRING},
	{"spawnflags",   FOFS( spawnflags ),   F_INT},
	{"speed",        FOFS( speed ),        F_FLOAT},
	{"closespeed",   FOFS( closespeed ),   F_FLOAT},
	{"target",       FOFS( target ),       F_LSTRING},
	{"targetname",   FOFS( targetname ),   F_LSTRING},
	{"targetdeath",  FOFS( targetdeath ),  F_LSTRING},
	{"message",      FOFS( message ),      F_LSTRING},
	{"popup",        FOFS( message ),      F_LSTRING}, // (SA) mutually exclusive from 'message', but makes the ent more logical for the level designer
	{"book",     FOFS( message ),      F_LSTRING}, // (SA) mutually exclusive from 'message', but makes the ent more logical for the level designer
	{"team",     FOFS( team ),         F_LSTRING},
	{"wait",     FOFS( wait ),         F_FLOAT},
	{"random",       FOFS( random ),       F_FLOAT},
	{"count",        FOFS( count ),        F_INT},
	{"health",       FOFS( health ),       F_INT},
	{"light",        0,                  F_IGNORE},
	{"dmg",          FOFS( damage ),       F_INT},
	{"angles",       FOFS( shared.s.angles ),     F_VECTOR},
	{"angle",        FOFS( shared.s.angles ),     F_ANGLEHACK},
	{"duration", FOFS( duration ),     F_FLOAT},
	{"rotate",       FOFS( rotate ),       F_VECTOR},
	{"degrees",      FOFS( angle ),        F_FLOAT},
	{"time",     FOFS( speed ),        F_FLOAT},

	{"aiattributes", FOFS( aiAttributes ), F_LSTRING},
	{"ainame",       FOFS( aiName ),       F_LSTRING},
	{"aiteam",       FOFS( aiTeam ),       F_INT},
	{"skin",     FOFS( aiSkin ),       F_LSTRING},
	{"head",     FOFS( aihSkin ),      F_LSTRING},

	// (SA) dlight lightstyles (made all these unique variables for testing)
	{"_color",       FOFS( dl_color ),     F_VECTOR},      // color of the light	(the underscore is inserted by the color picker in QER)
	{"color",        FOFS( dl_color ),     F_VECTOR},      // color of the light
	{"stylestring",  FOFS( dl_stylestring ), F_LSTRING},   // user defined stylestring "fffndlsfaaaaaa" for example

	{"shader",       FOFS( dl_shader ), F_LSTRING},    // shader to use for a target_effect or dlight

	{"key",          FOFS( key ),      F_INT},

	{"stand",        FOFS( shared.s.frame ),      F_INT},

	// Rafael - mg42
	{"harc",     FOFS( harc ),         F_FLOAT},
	{"varc",     FOFS( varc ),         F_FLOAT},
	// done.

	// Rafael - sniper
	{"delay",    FOFS( delay ),        F_FLOAT},
	{"radius",   FOFS( radius ),       F_INT},

	// Rafel
	{"start_size", FOFS( start_size ), F_INT},
	{"end_size", FOFS( end_size ), F_INT},

	{"shard", FOFS( count ), F_INT},

	// Rafael
	{"spawnitem",        FOFS( spawnitem ),            F_LSTRING},

	{"track",            FOFS( track ),                F_LSTRING},

	{"scriptName",       FOFS( scriptName ),           F_LSTRING},

	{nullptr}
};


typedef struct {
	const char    *name;
	void ( *spawn )( GameEntity *ent );
} spawn_t;

void SP_info_player_start( GameEntity *ent );
void SP_info_player_deathmatch( GameEntity *ent );
void SP_info_player_intermission( GameEntity *ent );
void SP_info_firstplace( GameEntity *ent );
void SP_info_secondplace( GameEntity *ent );
void SP_info_thirdplace( GameEntity *ent );
void SP_info_podium( GameEntity *ent );

void SP_func_plat( GameEntity *ent );
void SP_func_static( GameEntity *ent );
void SP_func_leaky( GameEntity *ent ); //----(SA)	added
void SP_func_rotating( GameEntity *ent );
void SP_func_bobbing( GameEntity *ent );
void SP_func_pendulum( GameEntity *ent );
void SP_func_button( GameEntity *ent );
void SP_func_explosive( GameEntity *ent );
void SP_func_door( GameEntity *ent );
void SP_func_train( GameEntity *ent );
void SP_func_timer( GameEntity *self );
// JOSEPH 1-26-00
void SP_func_train_rotating( GameEntity *ent );
void SP_func_secret( GameEntity *ent );
// END JOSEPH
// Rafael
void SP_func_door_rotating( GameEntity *ent );
// RF
void SP_func_bats( GameEntity *self );

void SP_trigger_always( GameEntity *ent );
void SP_trigger_multiple( GameEntity *ent );
void SP_trigger_push( GameEntity *ent );
void SP_trigger_teleport( GameEntity *ent );
void SP_trigger_hurt( GameEntity *ent );

//---- (SA) Wolf triggers
void SP_trigger_once( GameEntity *ent );
//---- done

void SP_target_remove_powerups( GameEntity *ent );
void SP_target_give( GameEntity *ent );
void SP_target_delay( GameEntity *ent );
void SP_target_speaker( GameEntity *ent );
void SP_target_print( GameEntity *ent );
void SP_target_laser( GameEntity *self );
void SP_target_character( GameEntity *ent );
void SP_target_score( GameEntity *ent );
void SP_target_teleporter( GameEntity *ent );
void SP_target_relay( GameEntity *ent );
void SP_target_kill( GameEntity *ent );
void SP_target_position( GameEntity *ent );
void SP_target_location( GameEntity *ent );
void SP_target_push( GameEntity *ent );
void SP_target_script_trigger( GameEntity *ent );

//---- (SA) Wolf targets
// targets
void SP_target_alarm( GameEntity *ent );
void SP_target_counter( GameEntity *ent );
void SP_target_lock( GameEntity *ent );
void SP_target_effect( GameEntity *ent );
void SP_target_fog( GameEntity *ent );
void SP_target_autosave( GameEntity *ent );

// entity visibility dummy
void SP_misc_vis_dummy( GameEntity *ent );
void SP_misc_vis_dummy_multiple( GameEntity *ent );

//----(SA) done

void SP_light( GameEntity *self );
void SP_info_null( GameEntity *self );
void SP_info_notnull( GameEntity *self );
void SP_info_notnull_big( GameEntity *ent );  //----(SA)	added
void SP_info_camp( GameEntity *self );
void SP_path_corner( GameEntity *self );

void SP_misc_teleporter_dest( GameEntity *self );
void SP_misc_model( GameEntity *ent );
void SP_misc_gamemodel( GameEntity *ent );
void SP_misc_portal_camera( GameEntity *ent );
void SP_misc_portal_surface( GameEntity *ent );
void SP_misc_light_surface( GameEntity *ent );
void SP_misc_grabber_trap( GameEntity *ent );
void SP_misc_spotlight( GameEntity *ent ); //----(SA)	added

void SP_shooter_rocket( GameEntity *ent );
void SP_shooter_grenade( GameEntity *ent );

// JOSEPH 1-18-00
void SP_props_box_32( GameEntity *self );
void SP_props_box_48( GameEntity *self );
void SP_props_box_64( GameEntity *self );
// END JOSEPH

// Ridah
void SP_ai_soldier( GameEntity *ent );
void SP_ai_american( GameEntity *ent );
void SP_ai_zombie( GameEntity *ent );
void SP_ai_warzombie( GameEntity *ent );
void SP_ai_marker( GameEntity *ent );
void SP_ai_effect( GameEntity *ent );
void SP_ai_trigger( GameEntity *ent );
void SP_ai_venom( GameEntity *ent );
void SP_ai_loper( GameEntity *ent );
void SP_ai_boss_helga( GameEntity *ent );
void SP_ai_boss_heinrich( GameEntity *ent ); //----(SA)	added
void SP_ai_eliteguard( GameEntity *ent );
void SP_ai_stimsoldier_dual( GameEntity *ent );
void SP_ai_stimsoldier_rocket( GameEntity *ent );
void SP_ai_stimsoldier_tesla( GameEntity *ent );
void SP_ai_supersoldier( GameEntity *ent );
void SP_ai_blackguard( GameEntity *ent );
void SP_ai_protosoldier( GameEntity *ent );
void SP_ai_frogman( GameEntity *ent );
void SP_ai_partisan( GameEntity *ent );
void SP_ai_civilian( GameEntity *ent );
// done.

// Rafael particles
void SP_Snow( GameEntity *ent );
void SP_target_smoke( GameEntity *ent );
void SP_Bubbles( GameEntity *ent );
// done.

// (SA) dlights
void SP_dlight( GameEntity *ent );
// done
void SP_corona( GameEntity *ent );

// Rafael mg42
void SP_mg42( GameEntity *ent );
// done.

// Rafael sniper
void SP_shooter_sniper( GameEntity *ent );
void SP_sniper_brush( GameEntity *ent );
// done

//----(SA)
void SP_shooter_zombiespit( GameEntity *ent );
void SP_shooter_mortar( GameEntity *ent );
void SP_shooter_tesla( GameEntity *ent );

// alarm
void SP_alarm_box( GameEntity *ent );
//----(SA)	end


void SP_trigger_objective_info( GameEntity *ent );   // DHM - Nerve

void SP_gas( GameEntity *ent );
void SP_target_rumble( GameEntity *ent );
void SP_func_train_particles( GameEntity *ent );


// Rafael
void SP_trigger_aidoor( GameEntity *ent );
void SP_SmokeDust( GameEntity *ent );
void SP_Dust( GameEntity *ent );
void SP_props_sparks( GameEntity *ent );
void SP_props_gunsparks( GameEntity *ent );

// Props
void SP_Props_Bench( GameEntity *ent );
void SP_Props_Radio( GameEntity *ent );
void SP_Props_Chair( GameEntity *ent );
void SP_Props_ChairHiback( GameEntity *ent );
void SP_Props_ChairSide( GameEntity *ent );
void SP_Props_ChairChat( GameEntity *ent );
void SP_Props_ChairChatArm( GameEntity *ent );
void SP_Props_DamageInflictor( GameEntity *ent );
void SP_Props_Locker_Tall( GameEntity *ent );
void SP_Props_Desklamp( GameEntity *ent );
void SP_Props_Flamebarrel( GameEntity *ent );
void SP_crate_64( GameEntity *ent );
void SP_Props_Flipping_Table( GameEntity *ent );
void SP_crate_32( GameEntity *self );
void SP_Props_Crate32x64( GameEntity *ent );
void SP_Props_58x112tablew( GameEntity *ent );
void SP_props_castlebed( GameEntity *ent );
void SP_Props_RadioSEVEN( GameEntity *ent );
void SP_propsFireColumn( GameEntity *ent );
void SP_props_flamethrower( GameEntity *ent );

void SP_func_tramcar( GameEntity *ent );
void func_invisible_user( GameEntity *ent );

void SP_lightJunior( GameEntity *self );

void SP_props_me109( GameEntity *ent );
void SP_misc_flak( GameEntity *ent );
void SP_plane_waypoint( GameEntity *self );

void SP_props_snowGenerator( GameEntity *ent );
void SP_truck_cam( GameEntity *self );

void SP_screen_fade( GameEntity *ent );
void SP_camera_reset_player( GameEntity *ent );
void SP_camera_cam( GameEntity *ent );
void SP_props_decoration( GameEntity *ent );
void SP_props_decorBRUSH( GameEntity *ent );
void SP_props_statue( GameEntity *ent );
void SP_props_statueBRUSH( GameEntity *ent );
void SP_skyportal( GameEntity *ent );

// RF, scripting
void SP_script_model_med( GameEntity *ent );
void SP_script_mover( GameEntity *ent );

void SP_props_footlocker( GameEntity *self );
void SP_misc_firetrails( GameEntity *ent );
void SP_misc_tagemitter( GameEntity *ent );   //----(SA)	added
void SP_trigger_deathCheck( GameEntity *ent );
void SP_misc_spawner( GameEntity *ent );
void SP_props_decor_Scale( GameEntity *ent );

spawn_t spawns[] = {
	// info entities don't do anything at all, but provide positional
	// information for things controlled by other processes
	{"info_player_start", SP_info_player_start},
	{"info_player_deathmatch", SP_info_player_deathmatch},
	{"info_player_intermission", SP_info_player_intermission},
	{"info_null", SP_info_null},
	{"info_notnull", SP_info_notnull},
	{"info_notnull_big", SP_info_notnull_big},

	{"info_camp", SP_info_camp},

	{"func_plat",      SP_func_plat},
	{"func_button",    SP_func_button},
	{"func_explosive", SP_func_explosive},
	{"func_door",      SP_func_door},
	{"func_static",    SP_func_static},
	{"func_leaky",     SP_func_leaky},
	{"func_rotating",  SP_func_rotating},
	{"func_bobbing",   SP_func_bobbing},
	{"func_pendulum",  SP_func_pendulum},
	{"func_train",     SP_func_train},
	{"func_group",     SP_info_null},
	{"func_train_rotating", SP_func_train_rotating},
	{"func_secret", SP_func_secret},

	{"func_door_rotating", SP_func_door_rotating},

	{"func_train_particles", SP_func_train_particles},

	{"func_timer", SP_func_timer},           // rename trigger_timer?

	{"func_tramcar", SP_func_tramcar},
	{"func_invisible_user", func_invisible_user},

	{"func_bats", SP_func_bats},

	// Triggers are brush objects that cause an effect when contacted
	// by a living player, usually involving firing targets.
	// While almost everything could be done with
	// a single trigger class and different targets, triggered effects
	// could not be client side predicted (push and teleport).
	{"trigger_always",   SP_trigger_always},
	{"trigger_multiple", SP_trigger_multiple},
	{"trigger_push",     SP_trigger_push},
	{"trigger_teleport", SP_trigger_teleport},
	{"trigger_hurt",     SP_trigger_hurt},

	{"trigger_once",     SP_trigger_once},

	{"trigger_aidoor", SP_trigger_aidoor},
	{"trigger_deathCheck",SP_trigger_deathCheck},

	// targets perform no action by themselves, but must be triggered
	// by another entity
	{"target_give", SP_target_give},
	{"target_remove_powerups", SP_target_remove_powerups},
	{"target_delay", SP_target_delay},
	{"target_speaker", SP_target_speaker},
	{"target_print", SP_target_print},
	{"target_laser", SP_target_laser},
	{"target_score", SP_target_score},
	{"target_teleporter", SP_target_teleporter},
	{"target_relay",      SP_target_relay},
	{"target_kill",       SP_target_kill},
	{"target_position",   SP_target_position},
	{"target_location",   SP_target_location},
	{"target_push",       SP_target_push},
	{"target_script_trigger", SP_target_script_trigger},

	{"target_alarm",     SP_target_alarm},
	{"target_counter",   SP_target_counter},
	{"target_lock",      SP_target_lock},
	{"target_effect",    SP_target_effect},
	{"target_fog",       SP_target_fog},
	{"target_autosave",  SP_target_autosave},

	{"target_rumble", SP_target_rumble},

	{"light", SP_light},

	{"lightJunior", SP_lightJunior},

	{"path_corner", SP_path_corner},

	{"misc_teleporter_dest", SP_misc_teleporter_dest},
	{"misc_model", SP_misc_model},
	{"misc_gamemodel", SP_misc_gamemodel},
	{"misc_portal_surface", SP_misc_portal_surface},
	{"misc_portal_camera", SP_misc_portal_camera},

	{"misc_vis_dummy",       SP_misc_vis_dummy},
	{"misc_vis_dummy_multiple",      SP_misc_vis_dummy_multiple},
	{"misc_light_surface",   SP_misc_light_surface},
	{"misc_grabber_trap",    SP_misc_grabber_trap},
	{"misc_spotlight",       SP_misc_spotlight},

	{"misc_mg42", SP_mg42},
	{"misc_flak", SP_misc_flak},
	{"misc_firetrails", SP_misc_firetrails},

	{"misc_tagemitter", SP_misc_tagemitter},

	{"shooter_rocket", SP_shooter_rocket},
	{"shooter_grenade", SP_shooter_grenade},

	{"shooter_zombiespit", SP_shooter_zombiespit},
	{"shooter_mortar", SP_shooter_mortar},
	{"shooter_tesla", SP_shooter_tesla},

	{"alarm_box", SP_alarm_box},

	{"shooter_sniper", SP_shooter_sniper},
	{"sniper_brush", SP_sniper_brush},

	{"ai_soldier",       SP_ai_soldier},
	{"ai_american",      SP_ai_american},
	{"ai_zombie",        SP_ai_zombie},
	{"ai_warzombie",     SP_ai_warzombie},
	{"ai_venom",         SP_ai_venom},
	{"ai_loper",         SP_ai_loper},
	{"ai_boss_helga",    SP_ai_boss_helga},
	{"ai_boss_heinrich", SP_ai_boss_heinrich},
	{"ai_eliteguard",    SP_ai_eliteguard},
	{"ai_stimsoldier_dual",   SP_ai_stimsoldier_dual},
	{"ai_stimsoldier_rocket", SP_ai_stimsoldier_rocket},
	{"ai_stimsoldier_tesla",  SP_ai_stimsoldier_tesla},
	{"ai_supersoldier", SP_ai_supersoldier},
	{"ai_protosoldier", SP_ai_protosoldier},
	{"ai_frogman",    SP_ai_frogman},
	{"ai_blackguard", SP_ai_blackguard},
	{"ai_partisan",   SP_ai_partisan},
	{"ai_civilian",   SP_ai_civilian},
	{"ai_marker",     SP_ai_marker},
	{"ai_effect",     SP_ai_effect},
	{"ai_trigger",    SP_ai_trigger},

	{"misc_snow256", SP_Snow},
	{"misc_snow128", SP_Snow},
	{"misc_snow64",  SP_Snow},
	{"misc_snow32",  SP_Snow},
	{"target_smoke", SP_target_smoke},

	{"misc_bubbles8",  SP_Bubbles},
	{"misc_bubbles16", SP_Bubbles},
	{"misc_bubbles32", SP_Bubbles},
	{"misc_bubbles64", SP_Bubbles},

	{"misc_spawner", SP_misc_spawner},

	{"props_box_32", SP_props_box_32},
	{"props_box_48", SP_props_box_48},
	{"props_box_64", SP_props_box_64},

	{"props_smokedust", SP_SmokeDust},
	{"props_dust",      SP_Dust},
	{"props_sparks",    SP_props_sparks},
	{"props_gunsparks", SP_props_gunsparks},

	{"plane_waypoint", SP_plane_waypoint},

	{"props_me109", SP_props_me109},

	{"props_bench", SP_Props_Bench},
	{"props_radio", SP_Props_Radio},
	{"props_chair", SP_Props_Chair},
	{"props_chair_hiback",   SP_Props_ChairHiback},
	{"props_chair_side",     SP_Props_ChairSide},
	{"props_chair_chat",     SP_Props_ChairChat},
	{"props_chair_chatarm",  SP_Props_ChairChatArm},
	{"props_damageinflictor", SP_Props_DamageInflictor},
	{"props_locker_tall",   SP_Props_Locker_Tall},
	{"props_desklamp",      SP_Props_Desklamp},
	{"props_flamebarrel",   SP_Props_Flamebarrel},
	{"props_crate_64",      SP_crate_64},
	{"props_flippy_table",  SP_Props_Flipping_Table},
	{"props_crate_32",      SP_crate_32},
	{"props_crate_32x64",   SP_Props_Crate32x64},
	{"props_58x112tablew",  SP_Props_58x112tablew},
	{"props_castlebed",     SP_props_castlebed},
	{"props_radioSEVEN",    SP_Props_RadioSEVEN},
	{"props_snowGenerator", SP_props_snowGenerator},
	{"props_FireColumn",    SP_propsFireColumn},
	{"props_decoration",    SP_props_decoration},
	{"props_decorBRUSH",    SP_props_decorBRUSH},
	{"props_statue",        SP_props_statue},
	{"props_statueBRUSH",   SP_props_statueBRUSH},
	{"props_skyportal",     SP_skyportal},
	{"props_footlocker",    SP_props_footlocker},
	{"props_flamethrower",  SP_props_flamethrower},
	{"props_decoration_scale",SP_props_decor_Scale},

	{"truck_cam", SP_truck_cam},

	{"screen_fade", SP_screen_fade},
	{"camera_reset_player", SP_camera_reset_player},
	{"camera_cam",SP_camera_cam},
	
	{"dlight",   SP_dlight},
	{"corona",   SP_corona},

	{"test_gas", SP_gas},

	{"trigger_objective_info", SP_trigger_objective_info},

	{"script_model_med", SP_script_model_med},
	{"script_mover",     SP_script_mover},

	{0, 0}
};

/*
===============
G_CallSpawn

Finds the spawn function for the entity and calls it,
returning false if not found
===============
*/
bool G_CallSpawn( GameEntity *ent )
{
	if ( !ent->classname ) {
		Com_Printf( "G_CallSpawn: nullptr classname\n" );
		return false;
	}

	// check item spawn functions
	for (gitem_t *item = bg_itemlist + 1 ; item->classname ; item++ ) {
		if ( !strcmp( item->classname, ent->classname ) ) {
			G_SpawnItem( ent, item );
			return true;
		}
	}

	// check normal spawn functions
	for (spawn_t* s = spawns ; s->name ; s++ ) {
		if ( !strcmp( s->name, ent->classname ) ) {
			// found it
			s->spawn( ent );

			// RF, entity scripting
			if ( ent->shared.s.number >= MAX_CLIENTS && ent->scriptName ) {
				G_Script_ScriptParse( ent );
				G_Script_ScriptEvent( ent, "spawn", "" );
			}

			return true;
		}
	}
	Com_Printf( "%s doesn't have a spawn function\n", ent->classname );
	return false;
}

/*
=============
G_NewString

Builds a copy of the string, translating \n to real linefeeds
so message texts can be multi-line
=============
*/
char *G_NewString( const char *string )
{
	size_t l = strlen( string ) + 1;

	char* newb = (char *)G_Alloc( l );

	char* new_p = newb;

	// turn \n into a real linefeed
	for (int i = 0 ; i < l ; i++ ) {
		if ( string[i] == '\\' && i < l - 1 ) {
			i++;
			if ( string[i] == 'n' ) {
				*new_p++ = '\n';
			} else {
				*new_p++ = '\\';
			}
		} else {
			*new_p++ = string[i];
		}
	}

	return newb;
}

/*
===============
G_ParseField

Takes a key/value pair and sets the binary values
in a gentity
===============
*/
void G_ParseField( const char *key, const char *value, GameEntity *ent )
{
	for (gentity_field_t *f = fields; f->name; f++ ) {
		if ( !Q_stricmp( f->name, key ) ) {
			// found it
			uint8_t* b = (uint8_t *)ent;

			switch ( f->type ) {
			case F_LSTRING:
				*( char ** )( b + f->ofs ) = G_NewString( value );
				break;
			case F_VECTOR:
				{
					vec3_t vec;
					sscanf( value, "%f %f %f", &vec[0], &vec[1], &vec[2] );
					( ( float * )( b + f->ofs ) )[0] = vec[0];
					( ( float * )( b + f->ofs ) )[1] = vec[1];
					( ( float * )( b + f->ofs ) )[2] = vec[2];
				}
				break;
			case F_INT:
				*( int * )( b + f->ofs ) = atoi( value );
				break;
			case F_FLOAT:
				*( float * )( b + f->ofs ) = atof( value );
				break;
			case F_ANGLEHACK:
				{
					float v = atof( value );
					( ( float * )( b + f->ofs ) )[0] = 0;
					( ( float * )( b + f->ofs ) )[1] = v;
					( ( float * )( b + f->ofs ) )[2] = 0;
				}
				break;
			case F_IGNORE:
				break;
			default:
				Com_Printf( "Unknown type %d\n", f->type );
			}
			return;
		}
	}
}

/*
===================
G_SpawnGEntityFromSpawnVars

Spawn an entity and fill in all of the level fields from
level.spawnVars[], then call the class specfic spawn function
===================
*/
void G_SpawnGEntityFromSpawnVars()
{
	// get the next free entity
	GameEntity* ent = G_Spawn();

	for (int i = 0 ; i < level.numSpawnVars ; i++ ) {
		G_ParseField( level.spawnVars[i][0], level.spawnVars[i][1], ent );
	}

	// move editor origin to pos
	VectorCopy( ent->shared.s.origin, ent->shared.s.pos.trBase );
	VectorCopy( ent->shared.s.origin, ent->shared.r.currentOrigin );

	// if we didn't get a classname, don't bother spawning anything
	if ( !G_CallSpawn( ent ) ) {
		G_FreeEntity( ent );
	}
}


/*
====================
G_AddSpawnVarToken
====================
*/
char *G_AddSpawnVarToken( const char *string )
{
	size_t l = strlen( string );
	if ( level.numSpawnVarChars + l + 1 > MAX_SPAWN_VARS_CHARS ) {
		Com_Error( ERR_DROP, "G_AddSpawnVarToken: MAX_SPAWN_VARS" );
        return nullptr; // keep the linter happy, ERR_DROP does not return
	}

	char* dest = level.spawnVarChars + level.numSpawnVarChars;
	memcpy( dest, string, l + 1 );

	level.numSpawnVarChars += l + 1;

	return dest;
}

bool GetEntityToken( char *buffer, int bufferSize )
{
	const char  *s = COM_Parse( (const char**)&sv.entityParsePoint );
	Q_strncpyz( buffer, s, bufferSize );
	if ( !sv.entityParsePoint && !s[0] ) {
		return false;
	} else {
		return true;
	}
}

/*
====================
G_ParseSpawnVars

Parses a brace bounded set of key / value pairs out of the
level's entity strings into level.spawnVars[]

This does not actually spawn an entity.
====================
*/
bool G_ParseSpawnVars()
{
	char keyname[MAX_TOKEN_CHARS];
	char com_token[MAX_TOKEN_CHARS];

	level.numSpawnVars = 0;
	level.numSpawnVarChars = 0;

	// parse the opening brace
	if ( !GetEntityToken( com_token, sizeof( com_token ) ) ) {
		// end of spawn string
		return false;
	}
	if ( com_token[0] != '{' ) {
		Com_Error( ERR_DROP, "G_ParseSpawnVars: found %s when expecting {",com_token );
        return false; // keep the linter happy, ERR_DROP does not return
	}

	// go through all the key / value pairs
	while ( 1 ) {
		// parse key
		if ( !GetEntityToken( keyname, sizeof( keyname ) ) ) {
			Com_Error( ERR_DROP, "G_ParseSpawnVars: EOF without closing brace" );
            return false; // keep the linter happy, ERR_DROP does not return
		}

		if ( keyname[0] == '}' ) {
			break;
		}

		// parse value
		if ( !GetEntityToken( com_token, sizeof( com_token ) ) ) {
			Com_Error( ERR_DROP, "G_ParseSpawnVars: EOF without closing brace" );
            return false; // keep the linter happy, ERR_DROP does not return
		}

		if ( com_token[0] == '}' ) {
			Com_Error( ERR_DROP, "G_ParseSpawnVars: closing brace without data" );
            return false; // keep the linter happy, ERR_DROP does not return
		}
		if ( level.numSpawnVars == MAX_SPAWN_VARS ) {
			Com_Error( ERR_DROP, "G_ParseSpawnVars: MAX_SPAWN_VARS" );
            return false; // keep the linter happy, ERR_DROP does not return
		}
		level.spawnVars[ level.numSpawnVars ][0] = G_AddSpawnVarToken( keyname );
		level.spawnVars[ level.numSpawnVars ][1] = G_AddSpawnVarToken( com_token );
		level.numSpawnVars++;
	}

	return true;
}



/*QUAKED worldspawn (0 0 0) ? sun_cameraflare

Every map should have exactly one worldspawn.
"music"     Music wav file
"gravity"   800 is default gravity
"message" Text to print during connection process
"ambient"  Ambient light value (must use '_color')
"_color"    Ambient light color (must be used with 'ambient')
"sun"        Shader to use for 'sun' image
*/
void SP_worldspawn()
{
	const char    *s;

	G_SpawnString( "classname", "", &s );
	if ( Q_stricmp( s, "worldspawn" ) ) {
		Com_Error( ERR_DROP, "SP_worldspawn: The first entity isn't 'worldspawn'" );
        return; // keep the linter happy, ERR_DROP does not return
	}

	// make some data visible to connecting client
	SV_SetConfigstring( CS_GAME_VERSION, GAME_VERSION );

	SV_SetConfigstring( CS_LEVEL_START_TIME, va( "%i", level.startTime ) );

	G_SpawnString( "music", "", &s );
	SV_SetConfigstring( CS_MUSIC, s );

	G_SpawnString( "message", "", &s );
	SV_SetConfigstring( CS_MESSAGE, s );              // map specific message

	SV_SetConfigstring( CS_MOTD, g_motd.string );     // message of the day

	G_SpawnString( "gravity", "800", &s );
	Cvar_Set( "g_gravity", s );

	// (SA) FIXME: todo: sun shader set for worldspawn

	g_entities[ENTITYNUM_WORLD].shared.s.number = ENTITYNUM_WORLD;
	g_entities[ENTITYNUM_WORLD].classname = "worldspawn";

	// see if we want a warmup time
	SV_SetConfigstring( CS_WARMUP, "" );
	if ( g_restarted.integer ) {
		Cvar_Set( "g_restarted", "0" );
		level.warmupTime = 0;
	}

}


/*
==============
G_SpawnEntitiesFromString

Parses textual entity definitions out of an entstring and spawns gentities.
==============
*/
void G_SpawnEntitiesFromString()
{
	// allow calls to G_Spawn*()
	level.spawning = true;
	level.numSpawnVars = 0;

	// the worldspawn is not an actual entity, but it still
	// has a "spawn" function to perform any global setup
	// needed by a level (setting configstrings or cvars, etc)
	if ( !G_ParseSpawnVars() ) {
		Com_Error( ERR_DROP, "SpawnEntities: no entities" );
        return; // keep the linter happy, ERR_DROP does not return
	}
	SP_worldspawn();

	// parse ents
	while ( G_ParseSpawnVars() ) {
		G_SpawnGEntityFromSpawnVars();
	}

	level.spawning = false;            // any future calls to G_Spawn*() will be errors
}

