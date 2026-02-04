#include "../gameEntity.h"
#include "../g_local.h"             // for spawn stuff
#include "../../server/server.h"    // SV_LinkEntity, SV_UnlinkEntity

/*QUAKED props_flippy_table (.8 .6 .2) ? - - X_AXIS Y_AXIS LEADER
this entity will need a leader and an origin brush
!!!!!!!!!!!!!!
just a reminder to put the origin brush in the proper location for the leader and the
slave so that the table will flip over correctly.
*/

#define FLIPPY_TABLE_X_AXIS 4
#define FLIPPY_TABLE_Y_AXIS 8
#define FLIPPY_TABLE_LEADER 16

void flippy_table_use( GameEntity *ent, GameEntity *other, GameEntity *activator )
{
	// it would be odd to flip a table if your standing on it
	if ( other && other->shared.s.groundEntityNum == ent->shared.s.number ) {
		// Com_Printf ("can't push table over while standing on it\n");
		return;
	}

	ent->use = nullptr;

	bool is_infront = infront( ent, other );

	if ( is_infront ) {
		// need to swap the team leader with the slave
		for (GameEntity   * slave = ent ; slave ; slave = slave->teamchain )
		{
			if ( slave == ent ) {
				continue;
			}

			slave->shared.s.pos.trType = ent->shared.s.pos.trType;
			slave->shared.s.pos.trTime = ent->shared.s.pos.trTime;
			slave->shared.s.pos.trDuration = ent->shared.s.pos.trDuration;
			VectorCopy( ent->shared.s.pos.trBase, slave->shared.s.pos.trBase );
			VectorCopy( ent->shared.s.pos.trDelta, slave->shared.s.pos.trDelta );

			slave->shared.s.apos.trType = ent->shared.s.apos.trType;
			slave->shared.s.apos.trTime = ent->shared.s.apos.trTime;
			slave->shared.s.apos.trDuration = ent->shared.s.apos.trDuration;
			VectorCopy( ent->shared.s.apos.trBase, slave->shared.s.apos.trBase );
			VectorCopy( ent->shared.s.apos.trDelta, slave->shared.s.apos.trDelta );

			slave->think = ent->think;
			slave->nextthink = ent->nextthink;

			VectorCopy( ent->pos1, slave->pos1 );
			VectorCopy( ent->pos2, slave->pos2 );

			slave->speed = ent->speed;

			slave->flags &= ~FL_TEAMSLAVE;
			// make it visible
			SV_LinkEntity( &slave->shared );

			Use_BinaryMover( slave, other, other );
		}

		SV_UnlinkEntity( &ent->shared );
	} else {
		Use_BinaryMover( ent, other, other );
	}

}

void flippy_table_animate( GameEntity *ent )
{
}

void props_flippy_table_die( GameEntity *ent, GameEntity *inflictor, GameEntity *attacker, int damage, int mod )
{
	ent->think = flippy_table_animate;
	ent->nextthink = level.time + FRAMETIME;

	ent->takedamage = false;

	G_UseTargets( ent, nullptr );
}

void props_flippy_blocked( GameEntity *ent, GameEntity *other )
{
	vec3_t velocity;
	vec3_t angles;
	vec3_t kvel;

	// just for now
	float angle = ent->shared.r.currentAngles[YAW];

	if ( other->client ) {
		// shoot the player off of it
		VectorCopy( ent->shared.s.apos.trBase, angles );
		angles[YAW] += angle;
		angles[PITCH] = 0;  // always forward

		AngleVectors( angles, velocity, nullptr, nullptr );
		VectorScale( velocity, 24, velocity );
		velocity[2] += 100 + crandom() * 50;

		VectorScale( velocity, 32, kvel );
		VectorAdd( other->client->ps.velocity, kvel, other->client->ps.velocity );
	} else if ( other->shared.s.eType == ET_ITEM )     {
		VectorCopy( ent->shared.s.apos.trBase, angles );
		angles[YAW] += angle;
		angles[PITCH] = 0;  // always forward

		AngleVectors( angles, velocity, nullptr, nullptr );
		VectorScale( velocity, 150, velocity );
		velocity[2] += 300 + crandom() * 50;

		VectorScale( velocity, 8, kvel );
		other->shared.s.pos.trType = TR_GRAVITY;
		other->shared.s.pos.trTime = level.time;
		VectorCopy( kvel, other->shared.s.pos.trDelta );

		other->shared.s.eFlags |= EF_BOUNCE;
	} else{
		// just delete it or destroy it
		G_TempEntity( other->shared.s.origin, EV_ITEM_POP );
		G_FreeEntity( other );
	}
}

void SP_Props_Flipping_Table( GameEntity *ent )
{
	if ( !ent->model ) {
		Com_Printf( S_COLOR_RED "props_Flipping_Table with nullptr model\n" );
		return;
	}

	SV_SetBrushModel( &ent->shared, ent->model );

	ent->speed = 500;
	ent->angle = 90;

	if ( !( ent->spawnflags & FLIPPY_TABLE_X_AXIS ) && !( ent->spawnflags & FLIPPY_TABLE_Y_AXIS ) ) {
		Com_Printf( "you forgot to select the X or Y Axis\n" );
	}

	VectorClear( ent->rotate );

	if( ent->spawnflags & FLIPPY_TABLE_X_AXIS ) {
		ent->rotate[2] = 1;
	} else if ( ent->spawnflags & FLIPPY_TABLE_Y_AXIS ) {
		ent->rotate[0] = 1;
	} else {
        ent->rotate[1] = 1;
    }

	ent->spawnflags |= 64; // stay open

	InitMoverRotate( ent );

	VectorCopy( ent->shared.s.origin, ent->shared.s.pos.trBase );
	VectorCopy( ent->shared.s.pos.trBase, ent->shared.r.currentOrigin );
	VectorCopy( ent->shared.s.apos.trBase, ent->shared.r.currentAngles );

	ent->blocked = props_flippy_blocked;

	if ( !ent->health ) {
		ent->health = 100;
	}

	ent->wait *= 1000;
	ent->use = flippy_table_use;

	SV_LinkEntity( &ent->shared );
}

