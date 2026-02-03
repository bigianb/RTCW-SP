#include "../gameEntity.h"
#include "../g_local.h"             // for spawn stuff
#include "../../server/server.h"    // SV_LinkEntity, SV_UnlinkEntity


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
