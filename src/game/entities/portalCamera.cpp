#include "../gameEntity.h"
#include "../g_local.h"             // for spawn stuff
#include "../../server/server.h"    // SV_LinkEntity, SV_UnlinkEntity


void locateCamera( GameEntity *ent )
{
	GameEntity* owner = G_PickTarget( ent->target );
	if ( !owner ) {
		Com_Printf( "Couldn't find target for misc_partal_surface\n" );
		G_FreeEntity( ent );
		return;
	}
	ent->shared.r.ownerNum = owner->shared.s.number;

	// frame holds the rotate speed
	if ( owner->spawnflags & 1 ) {
		ent->shared.s.frame = 25;
	} else if ( owner->spawnflags & 2 ) {
		ent->shared.s.frame = 75;
	}

	// clientNum holds the rotate offset
	ent->shared.s.clientNum = owner->shared.s.clientNum;

	VectorCopy( owner->shared.s.origin, ent->shared.s.origin2 );

	// see if the portal_camera has a target
	GameEntity* target = G_PickTarget( owner->target );
    vec3_t dir;
	if ( target ) {
		VectorSubtract( target->shared.s.origin, owner->shared.s.origin, dir );
		VectorNormalize( dir );
	} else {
		G_SetMovedir( owner->shared.s.angles, dir );
	}

	ent->shared.s.eventParm = DirToByte( dir );
}

/*QUAKED misc_portal_surface (0 0 1) (-8 -8 -8) (8 8 8)
The portal surface nearest this entity will show a view from the targeted misc_portal_camera, or a mirror view if untargeted.
This must be within 64 world units of the surface!
*/
void SP_misc_portal_surface( GameEntity *ent )
{
	VectorClear( ent->shared.r.mins );
	VectorClear( ent->shared.r.maxs );
	SV_LinkEntity( &ent->shared );

	ent->shared.r.svFlags = SVF_PORTAL;
	ent->shared.s.eType = ET_PORTAL;

	if ( !ent->target ) {
		VectorCopy( ent->shared.s.origin, ent->shared.s.origin2 );
	} else {
		ent->think = locateCamera;
		ent->nextthink = level.time + 100;
	}
}

/*QUAKED misc_portal_camera (0 0 1) (-8 -8 -8) (8 8 8) slowrotate fastrotate
The target for a misc_portal_director.  You can set either angles or target another entity to determine the direction of view.
"roll" an angle modifier to orient the camera around the target vector;
*/
void SP_misc_portal_camera( GameEntity *ent )
{
	VectorClear( ent->shared.r.mins );
	VectorClear( ent->shared.r.maxs );
	SV_LinkEntity( &ent->shared );

    float roll;
	G_SpawnFloat( "roll", "0", &roll );

	ent->shared.s.clientNum = roll / 360.0 * 256;
}
