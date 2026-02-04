#include "../gameEntity.h"
#include "../g_local.h"             // for spawn stuff
#include "../../server/server.h"    // SV_LinkEntity, SV_UnlinkEntity


void use_target_effect( GameEntity *self, GameEntity *other, GameEntity *activator )
{
	GameEntity* tent = G_TempEntity( self->shared.r.currentOrigin, EV_EFFECT );
	VectorCopy( self->shared.r.currentOrigin, tent->shared.s.origin );
	VectorCopy( self->shared.r.currentOrigin, tent->shared.s.origin2 );
	if ( self->spawnflags & 32 ) {
		tent->shared.s.dl_intensity = 1;   // low grav
	} else {
		tent->shared.s.dl_intensity = 0;
	}

	SV_SetConfigstring( CS_TARGETEFFECT, self->dl_shader );   // allow shader to be set from entity

	// this should match the values from func_explosive
	tent->shared.s.frame = self->key;      // pass the type to the client ("glass", "wood", "metal", "gibs", "brick", "stone", "fabric", 0, 1, 2, 3, 4, 5, 6)

	tent->shared.s.eventParm       = self->spawnflags;
	tent->shared.s.density         = self->health;

	tent->shared.s.effect3Time     = self->key;            // pass the type to the client ("glass", "wood", "metal", "gibs", "brick", "stone", "fabric", 0, 1, 2, 3, 4, 5, 6)

	if ( self->spawnflags & 128 ) {
		tent->shared.s.teamNum = 1;
	} else {
		tent->shared.s.teamNum = 0;
	}

	if ( self->damage ) {
		G_RadiusDamage( self->shared.s.pos.trBase, self, self->damage, self->damage, self, MOD_EXPLOSIVE );
	}

	G_UseTargets( self, other );
}


/*QUAKED target_effect (0 .5 .8) (-6 -6 -6) (6 6 6) TNT explode smoke rubble gore lowgrav debris player_damage
"mass" defaults to 15.  This determines how much debris is emitted when it explodes.  (number of pieces)
"dmg" defaults to 0.  damage radius blast when triggered
"type" - if 'rubble' is specified, this is the model type ("glass", "wood", "metal", "gibs", "brick", "rock", "fabric") default is "wood"
*/
void SP_target_effect( GameEntity *ent )
{
	ent->use = use_target_effect;

    int mass;
	if ( G_SpawnInt( "mass", "15", &mass ) ) {
		ent->health = mass;
	} else {
		ent->health = 15;
	}

	// this should match the values from func_explosive
    const char    *type;
	if ( G_SpawnString( "type", "wood", &type ) ) {
		if ( !Q_stricmp( type,"wood" ) ) {
			ent->key = 0;
		} else if ( !Q_stricmp( type,"glass" ) ) {
			ent->key = 1;
		} else if ( !Q_stricmp( type,"metal" ) ) {
			ent->key = 2;
		} else if ( !Q_stricmp( type,"gibs" ) ) {
			ent->key = 3;
		} else if ( !Q_stricmp( type,"brick" ) ){
			ent->key = 4;
		} else if ( !Q_stricmp( type,"rock" ) ) {
			ent->key = 5;
		} else if ( !Q_stricmp( type,"fabric" ) ) {
			ent->key = 6;
		}
	} else {
		ent->key = 5;   // default to 'rock'
	}

}


