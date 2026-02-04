#include "../gameEntity.h"
#include "../g_local.h"             // for spawn stuff
#include "../../server/server.h"    // SV_LinkEntity, SV_UnlinkEntity


/*QUAKED props_statue (.6 .3 .2) (-8 -8 0) (8 8 128) HURT DEBRIS ANIMATE KEEPBLOCK
"model2" will specify the model to load

"noise"  the sound entity is to make

"type" type of debris ("glass", "wood", "metal", "ceramic", "rubble") default is "wood"
"count" how much debris ei. default 4 pieces

you will need to specify the bounding box for the entity
"high"  default is 4
"wide"	default is 4

"frames"	how many frames of animation to play
"delay"		how long of a delay before damage is inflicted ei. 0.5 sec or 2.7 sec

"damage"	amount of damage to be inflicted
*/

void props_statue_blocked( GameEntity *ent ) {
	trace_t trace;
	vec3_t start, end, mins, maxs;
	vec3_t forward;
	float dist;
	GameEntity   *traceEnt;
	float grav = 128;
	vec3_t kvel;

	if ( !Q_stricmp( ent->classname, "props_statueBRUSH" ) ) {
		return;
	}

	VectorCopy( ent->shared.s.origin, start );
	start[2] += 24;

	VectorSet( mins, ent->shared.r.mins[0], ent->shared.r.mins[1], -23 );
	VectorSet( maxs, ent->shared.r.maxs[0], ent->shared.r.maxs[1], 23 );

	AngleVectors( ent->shared.r.currentAngles, forward, nullptr, nullptr );

	VectorCopy( start, end );

	dist = ( ( ent->shared.r.maxs[2] + 16 ) / ent->count2 ) * ent->shared.s.frame;

	VectorMA( end, dist, forward, end );

	SV_Trace( &trace, start, mins, maxs, end, ent->shared.s.number, MASK_SHOT, false );

	if ( trace.surfaceFlags & SURF_NOIMPACT ) { // bogus test but just in case
		return;
	}

	traceEnt = &g_entities[ trace.entityNum ];

	if ( traceEnt->takedamage && traceEnt->client ) {
		G_Damage( traceEnt, ent, ent, nullptr, trace.endpos, ent->damage, 0, MOD_CRUSH );

		// TBD: push client back a bit
		VectorScale( forward, grav, kvel );
		VectorAdd( traceEnt->client->ps.velocity, kvel, traceEnt->client->ps.velocity );

		if ( !traceEnt->client->ps.pm_time ) {
			int t;

			t = grav * 2;
			if ( t < 50 ) {
				t = 50;
			}
			if ( t > 200 ) {
				t = 200;
			}
			traceEnt->client->ps.pm_time = t;
			traceEnt->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
		}

	} else {
		G_Damage( traceEnt, ent, ent, nullptr, trace.endpos, 9999, 0, MOD_CRUSH );
	}

}

void props_statue_animate( GameEntity *ent ) {

	bool takeashot = false;

	ent->shared.s.frame++;
	ent->shared.s.eType = ET_GENERAL;

	if ( ent->shared.s.frame > ent->count2 ) {
		ent->shared.s.frame = ent->count2;
		ent->takedamage = false;
	}

	if ( ( ( ent->delay * 1000 ) + ent->timestamp ) > level.time ) {
		ent->count = 0;
	} else if ( ent->count == 5 )     {
		takeashot = true;
		ent->count = 0;
	} else {
		ent->count++;
	}

	if ( takeashot ) {
		props_statue_blocked( ent );
	}

	if ( ent->shared.s.frame < ent->count2 ) {
		ent->nextthink = level.time + 50;
	}
}


void props_statue_death( GameEntity *ent, GameEntity *inflictor, GameEntity *attacker, int damage, int mod ) {

	ent->timestamp = level.time;

	G_AddEvent( ent, EV_GENERAL_SOUND, ent->noise_index );

	if ( !( ent->spawnflags & 8 ) ) {
		ent->clipmask   = 0;
		ent->shared.r.contents = 0;
		ent->shared.s.eType = ET_GENERAL;
		SV_LinkEntity( &ent->shared );
	}

	ent->takedamage = false;

	G_UseTargets( ent, nullptr );

	if ( ent->spawnflags & 2 ) {
		Spawn_Shard( ent, inflictor, ent->count, ent->key );
	}

	if ( ent->spawnflags & 4 ) {
		ent->nextthink = level.time + 50;
		ent->think = props_statue_animate;
		return;
	}

	G_FreeEntity( ent );

}

void props_statue_touch( GameEntity *self, GameEntity *other, trace_t *trace ) {
	props_statue_death( self, other, other, 9999, MOD_CRUSH );
}

void SP_props_statue( GameEntity *ent ) {
	float light;
	vec3_t color;
	bool lightSet, colorSet;
	const char        *sound;
	const char        *type;
	const char        *high;
	const char        *wide;
	const char        *frames;
	float height;
	float width;
	float num_frames;

	if ( ent->model2 ) {
		ent->shared.s.modelindex = G_ModelIndex( ent->model2 );
	}

	if ( G_SpawnString( "noise", "100", &sound ) ) {
		ent->noise_index = G_SoundIndex( sound );
	}

	// if the "color" or "light" keys are set, setup constantLight
	lightSet = G_SpawnFloat( "light", "100", &light );
	colorSet = G_SpawnVector( "color", "1 1 1", color );
	if ( lightSet || colorSet ) {
		int r, g, b, i;

		r = color[0] * 255;
		if ( r > 255 ) {
			r = 255;
		}
		g = color[1] * 255;
		if ( g > 255 ) {
			g = 255;
		}
		b = color[2] * 255;
		if ( b > 255 ) {
			b = 255;
		}
		i = light / 4;
		if ( i > 255 ) {
			i = 255;
		}
		ent->shared.s.constantLight = r | ( g << 8 ) | ( b << 16 ) | ( i << 24 );
	}

	ent->isProp = true;
	ent->takedamage = true;
	ent->die = props_statue_death;

	G_SpawnString( "type", "wood", &type );
	if ( !Q_stricmp( type,"wood" ) ) {
		ent->key = 1;
	} else if ( !Q_stricmp( type,"glass" ) )   {
		ent->key = 0;
	} else if ( !Q_stricmp( type,"metal" ) )                                                           {
		ent->key = 2;
	} else if ( !Q_stricmp( type,"ceramic" ) )                                                                                                                   {
		ent->key = 3;
	} else if ( !Q_stricmp( type, "rubble" ) )                                                                                                                                                                             {
		ent->key = 4;
	}

	G_SpawnString( "high", "0", &high );
	height = atof( high );
	if ( !height ) {
		height = 4;
	}

	G_SpawnString( "wide", "0", &wide );
	width = atof( wide );

	if ( !width ) {
		width = 4;
	}

	width /= 2;

	if ( Q_stricmp( ent->classname, "props_statueBRUSH" ) ) {
		VectorSet( ent->shared.r.mins, -width, -width, 0 );
		VectorSet( ent->shared.r.maxs, width, width, height );
	}

	ent->clipmask   = CONTENTS_SOLID;
	ent->shared.r.contents = CONTENTS_SOLID;
	ent->shared.s.eType = ET_MOVER;

	G_SpawnString( "frames", "0", &frames );
	num_frames = atof( frames );

	ent->count2 = num_frames;

	ent->touch = props_statue_touch;


	G_SetOrigin( ent, ent->shared.s.origin );
	G_SetAngle( ent, ent->shared.s.angles );

	if ( !ent->damage ) {
		ent->damage = 1;
	}

	SV_LinkEntity( &ent->shared );
}


/*QUAKED props_statueBRUSH (.6 .3 .2) ? HURT DEBRIS ANIMATE KEEPBLOCK
needs an origin brush

"model2" will specify the model to load

"noise"  the sound entity is to make

"type" type of debris ("glass", "wood", "metal", "ceramic", "rubble") default is "wood"
"count" how much debris ei. default 4 pieces

"frames"	how many frames of animation to play
"delay"		how long of a delay before damage is inflicted ei. 0.5 sec or 2.7 sec

THE damage has been disabled at the moment
"damage"	amount of damage to be inflicted

*/

void SP_props_statueBRUSH( GameEntity *self ) {

	SV_SetBrushModel( &self->shared, self->model );

	SP_props_statue( self );

	if ( self->model2 ) {
		self->shared.s.modelindex2 = G_ModelIndex( self->model2 );
	}

	if ( !( self->health ) ) {
		self->health = 6;
	}

}
