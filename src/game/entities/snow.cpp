#include "../gameEntity.h"
#include "../g_local.h"             // for spawn stuff
#include "../../server/server.h"    // SV_LinkEntity, SV_UnlinkEntity

/*QUAKED props_snowGenerator (3 2 7) ? TOGGLE_ON ALWAYS_ON
entity brush need to be targeted to an info notnull this
will determine the direction the snow particles will travel.

speed
gravity
turb

count is the number of snowflurries 3 to 5 would be a good number

duration is how long the effect will last 1 is 1 second
*/

#define SNOWGEN_TOGGLE_ON 1
#define SNOWGEN_ALWAYS_ON 2

void props_snowGenerator_think( GameEntity *ent )
{
	if ( !( ent->spawnflags & SNOWGEN_TOGGLE_ON ) ) {
		return;
	}

	float high = ent->shared.r.maxs[2] - ent->shared.r.mins[2];
	float wide = ent->shared.r.maxs[1] - ent->shared.r.mins[1];
	float deep = ent->shared.r.maxs[0] - ent->shared.r.mins[0];

	for (int i = 0; i < ent->count; i++ ) {
		vec3_t point;
		VectorCopy( ent->pos1, point );

		// we need to randomize to the extent of the brush
		point[0] += crandom() * ( deep * 0.5 );
		point[1] += crandom() * ( wide * 0.5 );
		point[2] += crandom() * ( high * 0.5 );

		GameEntity* tent = G_TempEntity( point, EV_SNOWFLURRY );
		VectorCopy( point, tent->shared.s.origin );
		VectorCopy( ent->movedir, tent->shared.s.angles );
		tent->shared.s.time = 2000; // life time
		tent->shared.s.time2 = 1000; // alpha fade start
	}

	if ( ent->spawnflags & SNOWGEN_ALWAYS_ON ) {
		ent->nextthink = level.time + FRAMETIME;
	} else if ( ent->wait < level.time ) {
		ent->nextthink = level.time + FRAMETIME;
	}
}

void props_snowGenerator_use( GameEntity *ent, GameEntity *other, GameEntity *activator )
{
	if ( !( ent->spawnflags & SNOWGEN_TOGGLE_ON ) ) {
		ent->spawnflags |= SNOWGEN_TOGGLE_ON;
		ent->think = props_snowGenerator_think;
		ent->nextthink = level.time + FRAMETIME;
		ent->wait = level.time + ent->duration;
	} else {
		ent->spawnflags &= ~SNOWGEN_TOGGLE_ON;
	}
}

void SP_props_snowGenerator( GameEntity *ent )
{
	vec3_t center;

	SV_SetBrushModel( &ent->shared, ent->model );

	VectorAdd( ent->shared.r.absmin, ent->shared.r.absmax, center );
	VectorScale( center, 0.5, center );

	VectorCopy( center, ent->pos1 );

	if ( !ent->target ) {
		Com_Printf( "snowGenerator at loc %s does not have a target\n", vtos( center ) );
		return;
	}

	GameEntity* target = G_Find( target, FOFS( targetname ), ent->target );
	if ( !target ) {
		Com_Printf( "error snowGenerator at loc %s does cant find target %s\n", vtos( center ), ent->target );
		return;
	}

	VectorSubtract( target->shared.s.origin, ent->shared.s.origin, ent->movedir );
	VectorNormalize( ent->movedir );
	
	ent->shared.r.contents = CONTENTS_TRIGGER;
	ent->shared.r.svFlags = SVF_NOCLIENT;

	if ( ent->spawnflags & SNOWGEN_TOGGLE_ON || ent->spawnflags & SNOWGEN_ALWAYS_ON ) {
		ent->think = props_snowGenerator_think;
		ent->nextthink = level.time + FRAMETIME;

		if ( ent->spawnflags & SNOWGEN_ALWAYS_ON ) {
			ent->spawnflags |= SNOWGEN_TOGGLE_ON;
		}
	}

	ent->use = props_snowGenerator_use;

	if ( !( ent->delay ) ) {
		ent->delay = 100;
	} else {
		ent->delay *= 100;
	}

	if ( !( ent->count ) ) {
		ent->count = 32;
	}

	if ( !( ent->duration ) ) {
		ent->duration = 1;
	}

	ent->duration *= 1000;

	SV_LinkEntity( &ent->shared );
}

/*QUAKED misc_snow256 (1 0 0) (-256 -256 -16) (256 256 16) TURBULENT
health = density defaults to 32
*/
/*QUAKED misc_snow128 (1 0 0) (-128 -128 -16) (128 128 16) TURBULENT
health = density defaults to 32
*/
/*QUAKED misc_snow64 (1 0 0) (-64 -64 -16) (64 64 16) TURBULENT
health = density defaults to 32
*/
/*QUAKED misc_snow32 (1 0 0) (-32 -32 -16) (32 32 16) TURBULENT
health = density defaults to 32
*/

/*QUAKED misc_bubbles8 (1 0 0) (-8 -8 0) (8 8 16) TURBULENT
health = density defaults to 32
*/
/*QUAKED misc_bubbles16 (1 0 0) (-16 -16 0) (16 16 16) TURBULENT
health = density defaults to 32
*/
/*QUAKED misc_bubbles32 (1 0 0) (-32 -32 0) (32 32 16) TURBULENT
health = density defaults to 32
*/
/*QUAKED misc_bubbles64 (1 0 0) (-64 -64 0) (64 64 64) TURBULENT
health = density defaults to 32
*/


void snowInPVS( GameEntity *ent )
{
	int oldactive = ent->active;

	ent->nextthink = level.time + FRAMETIME;

	GameEntity* player = AICast_FindEntityForName( "player" );
    if (!player){
        return;
    }

    bool inPVS = SV_inPVS( player->shared.r.currentOrigin, ent->shared.r.currentOrigin );

    if ( inPVS ) {
        ent->active = true;
    } else {
        ent->active = false;
    }

	// there hasn't been a change so bail
	if ( oldactive == ent->active ) {
		return;
	}

    GameEntity *tent = G_TempEntity( player->shared.r.currentOrigin, ent->active ? EV_SNOW_ON : EV_SNOW_OFF );
	tent->shared.s.frame = ent->shared.s.number;
	SV_LinkEntity( &ent->shared );
}

void snow_think( GameEntity *ent )
{	
    vec3_t dest;
	VectorCopy( ent->shared.s.origin, dest );

	if ( ent->spawnflags & 2 ) { // bubble
		dest[2] += 8192;
	} else {
		dest[2] -= 8192;
	}

    trace_t tr;
	SV_Trace( &tr, ent->shared.s.origin, nullptr, nullptr, dest, ent->shared.s.number, MASK_SHOT, false );

    int turb;
	if ( ent->spawnflags & 1 ) {
		turb = 1;
	} else {
		turb = 0;
	}

	if ( !Q_stricmp( ent->classname, "misc_snow256" ) ) {
		G_FindConfigstringIndex( va( "%i %.2f %.2f %.2f %.2f %.2f %.2f %i %i %i", PARTICLE_SNOW256, ent->shared.s.origin[0], ent->shared.s.origin[1], ent->shared.s.origin[2], tr.endpos[0], tr.endpos[1], tr.endpos[2], ent->health, turb, ent->shared.s.number ), CS_PARTICLES, MAX_PARTICLES_AREAS, true );
	} else if ( !Q_stricmp( ent->classname, "misc_snow128" ) )       {
		G_FindConfigstringIndex( va( "%i %.2f %.2f %.2f %.2f %.2f %.2f %i %i %i", PARTICLE_SNOW128, ent->shared.s.origin[0], ent->shared.s.origin[1], ent->shared.s.origin[2], tr.endpos[0], tr.endpos[1], tr.endpos[2], ent->health, turb, ent->shared.s.number ), CS_PARTICLES, MAX_PARTICLES_AREAS, true );
	} else if ( !Q_stricmp( ent->classname, "misc_snow64" ) )       {
		G_FindConfigstringIndex( va( "%i %.2f %.2f %.2f %.2f %.2f %.2f %i %i %i", PARTICLE_SNOW64, ent->shared.s.origin[0], ent->shared.s.origin[1], ent->shared.s.origin[2], tr.endpos[0], tr.endpos[1], tr.endpos[2], ent->health, turb, ent->shared.s.number ), CS_PARTICLES, MAX_PARTICLES_AREAS, true );
	} else if ( !Q_stricmp( ent->classname, "misc_snow32" ) )       {
		G_FindConfigstringIndex( va( "%i %.2f %.2f %.2f %.2f %.2f %.2f %i %i %i", PARTICLE_SNOW32, ent->shared.s.origin[0], ent->shared.s.origin[1], ent->shared.s.origin[2], tr.endpos[0], tr.endpos[1], tr.endpos[2], ent->health, turb, ent->shared.s.number ), CS_PARTICLES, MAX_PARTICLES_AREAS, true );
	} else if ( !Q_stricmp( ent->classname, "misc_bubbles8" ) )       {
		G_FindConfigstringIndex( va( "%i %.2f %.2f %.2f %.2f %.2f %.2f %i %i %i", PARTICLE_BUBBLE8, ent->shared.s.origin[0], ent->shared.s.origin[1], ent->shared.s.origin[2], tr.endpos[0], tr.endpos[1], tr.endpos[2], ent->health, turb, ent->shared.s.number ), CS_PARTICLES, MAX_PARTICLES_AREAS, true );
	} else if ( !Q_stricmp( ent->classname, "misc_bubbles16" ) )       {
		G_FindConfigstringIndex( va( "%i %.2f %.2f %.2f %.2f %.2f %.2f %i %i %i", PARTICLE_BUBBLE16, ent->shared.s.origin[0], ent->shared.s.origin[1], ent->shared.s.origin[2], tr.endpos[0], tr.endpos[1], tr.endpos[2], ent->health, turb, ent->shared.s.number ), CS_PARTICLES, MAX_PARTICLES_AREAS, true );
	} else if ( !Q_stricmp( ent->classname, "misc_bubbles32" ) )       {
		G_FindConfigstringIndex( va( "%i %.2f %.2f %.2f %.2f %.2f %.2f %i %i %i", PARTICLE_BUBBLE32, ent->shared.s.origin[0], ent->shared.s.origin[1], ent->shared.s.origin[2], tr.endpos[0], tr.endpos[1], tr.endpos[2], ent->health, turb, ent->shared.s.number ), CS_PARTICLES, MAX_PARTICLES_AREAS, true );
	} else if ( !Q_stricmp( ent->classname, "misc_bubbles64" ) )       {
		G_FindConfigstringIndex( va( "%i %.2f %.2f %.2f %.2f %.2f %.2f %i %i %i", PARTICLE_BUBBLE64, ent->shared.s.origin[0], ent->shared.s.origin[1], ent->shared.s.origin[2], tr.endpos[0], tr.endpos[1], tr.endpos[2], ent->health, turb, ent->shared.s.number ), CS_PARTICLES, MAX_PARTICLES_AREAS, true );
	}

	ent->think = snowInPVS;
	ent->nextthink = level.time + FRAMETIME;

}

void SP_Snow( GameEntity *ent )
{
	ent->think = snow_think;
	ent->nextthink = level.time + FRAMETIME;

	G_SetOrigin( ent, ent->shared.s.origin );

	ent->shared.r.svFlags = SVF_USE_CURRENT_ORIGIN;
	ent->shared.s.eType = ET_GENERAL;

	SV_LinkEntity( &ent->shared );

	if ( !ent->health ) {
		ent->health = 32;
	}

	ent->active = true;
}


void SP_Bubbles( GameEntity *ent )
{
	ent->think = snow_think;
	ent->nextthink = level.time + FRAMETIME;

	G_SetOrigin( ent, ent->shared.s.origin );

	ent->shared.r.svFlags = SVF_USE_CURRENT_ORIGIN;
	ent->shared.s.eType = ET_GENERAL;

	SV_LinkEntity( &ent->shared );

	if ( !ent->health ) {
		ent->health = 32;
	}

	ent->active = true;

	ent->spawnflags |= 2;
}
