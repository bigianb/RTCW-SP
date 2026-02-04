#include "../gameEntity.h"
#include "../g_local.h"             // for spawn stuff
#include "../../server/server.h"    // SV_LinkEntity, SV_UnlinkEntity

void DropToFloor( GameEntity *ent );
void moveit( GameEntity *ent, float yaw, float dist );

void touch_props_box_32( GameEntity *self, GameEntity *other, trace_t *trace )
{
	if ( other->shared.r.currentOrigin[2] > ( self->shared.r.currentOrigin[2] + 10 + 15 ) ) {
		return;
	}

	float ratio = 2.5;
	vec3_t v;
	VectorSubtract( self->shared.r.currentOrigin, other->shared.r.currentOrigin, v );
	moveit( self, vectoyaw( v ), ( 20 * ratio * FRAMETIME ) * .001 );
}

/*QUAKED props_box_32 (1 0 0) (-16 -16 -16) (16 16 16)

*/
void SP_props_box_32( GameEntity *self )
{
	self->shared.s.modelindex = G_ModelIndex( "models/mapobjects/boxes/box32.md3" );

	self->clipmask   = CONTENTS_SOLID;
	self->shared.r.contents = CONTENTS_SOLID;

	VectorSet( self->shared.r.mins, -16, -16, -16 );
	VectorSet( self->shared.r.maxs, 16, 16, 16 );

	self->touch = touch_props_box_32;

	SV_LinkEntity( &self->shared );

	self->think = DropToFloor;
	self->nextthink = level.time + FRAMETIME;
}

void touch_props_box_48( GameEntity *self, GameEntity *other, trace_t *trace )
{
	if ( other->shared.r.currentOrigin[2] > ( self->shared.r.currentOrigin[2] + 10 + 23 ) ) {
		return;
	}

	float ratio = 2.0;
	vec3_t v;
	VectorSubtract( self->shared.r.currentOrigin, other->shared.r.currentOrigin, v );
	moveit( self, vectoyaw( v ), ( 20 * ratio * FRAMETIME ) * .001 );
}

/*QUAKED props_box_48 (1 0 0) (-24 -24 -24) (24 24 24)

*/
void SP_props_box_48( GameEntity *self )
{
	self->shared.s.modelindex = G_ModelIndex( "models/mapobjects/boxes/box48.md3" );

	self->clipmask   = CONTENTS_SOLID;
	self->shared.r.contents = CONTENTS_SOLID;

	VectorSet( self->shared.r.mins, -24, -24, -24 );
	VectorSet( self->shared.r.maxs, 24, 24, 24 );

	self->touch = touch_props_box_48;

	SV_LinkEntity( &self->shared );

	self->think = DropToFloor;
	self->nextthink = level.time + FRAMETIME;
}

void touch_props_box_64( GameEntity *self, GameEntity *other, trace_t *trace )
{
	if ( other->shared.r.currentOrigin[2] > ( self->shared.r.currentOrigin[2] + 10 + 31 ) ) {
		return;
	}

	float ratio = 1.5;
	vec3_t v;
	VectorSubtract( self->shared.r.currentOrigin, other->shared.r.currentOrigin, v );
	moveit( self, vectoyaw( v ), ( 20 * ratio * FRAMETIME ) * .001 );
}

/*QUAKED props_box_64 (1 0 0) (-32 -32 -32) (32 32 32)

*/
void SP_props_box_64( GameEntity *self )
{
	self->shared.s.modelindex = G_ModelIndex( "models/mapobjects/boxes/box64.md3" );

	self->clipmask   = CONTENTS_SOLID;
	self->shared.r.contents = CONTENTS_SOLID;

	VectorSet( self->shared.r.mins, -32, -32, -32 );
	VectorSet( self->shared.r.maxs, 32, 32, 32 );

	self->touch = touch_props_box_64;

	SV_LinkEntity( &self->shared );

	self->think = DropToFloor;
	self->nextthink = level.time + FRAMETIME;
}
