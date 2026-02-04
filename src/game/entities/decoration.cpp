#include "../gameEntity.h"
#include "../g_local.h"             // for spawn stuff
#include "../../server/server.h"    // SV_LinkEntity, SV_UnlinkEntity

/*QUAKED props_decoration (.6 .7 .7) (-8 -8 0) (8 8 16) STARTINVIS DEBRIS ANIMATE KEEPBLOCK TOUCHACTIVATE LOOPING STARTON
"model2" will specify the model to load

"noise"  the looping sound entity is to make

"type" type of debris ("glass", "wood", "metal", "ceramic", "rubble") default is "wood"
"count" how much debris ei. default 4 pieces

you will need to specify the bounding box for the entity
"high"  default is 4
"wide"	default is 4

"frames"	how many frames of animation to play
"loop" when the animation is done start again on this frame
"startonframe" on what frame do you want to start the animation
*/

#define DECOR_STARTINVIS 1
#define DECOR_DEBRIS 2
#define DECOR_ANIMATE 4
#define DECOR_KEEPBLOCK 8
#define DECOR_TOUCHACTIVATE 16
#define DECOR_LOOPING 32
#define DECOR_STARTON 64

void props_decoration_animate( GameEntity *ent )
{
	ent->shared.s.frame++;
	ent->shared.s.eType = ET_GENERAL;

	if ( ent->shared.s.frame > ent->count2 ) {
		if ( ent->spawnflags & DECOR_LOOPING || ent->spawnflags & DECOR_STARTON ) {
			ent->shared.s.frame = ent->props_frame_state;

			if ( !( ent->spawnflags & DECOR_STARTON ) ) {
				ent->takedamage = false;
			}
		} else {
			ent->shared.s.frame = ent->count2;
			ent->takedamage = false;

			return;
		}
	}

	ent->nextthink = level.time + 50;
}

void props_decoration_death( GameEntity *ent, GameEntity *inflictor, GameEntity *attacker, int damage, int mod )
{
	if ( !( ent->spawnflags & DECOR_KEEPBLOCK ) ) {
		ent->clipmask   = 0;
		ent->shared.r.contents = 0;
		ent->shared.s.eType = ET_GENERAL;
		SV_LinkEntity( &ent->shared );
	}

	ent->takedamage = false;

	G_UseTargets( ent, nullptr );

	if ( ent->spawnflags & DECOR_DEBRIS ) {
		Spawn_Shard( ent, inflictor, ent->count, ent->key );
	}

	if ( ent->spawnflags & DECOR_ANIMATE ) {
		ent->nextthink = level.time + 50;
		ent->think = props_decoration_animate;
		return;
	}

	G_FreeEntity( ent );

}

void Use_props_decoration( GameEntity *ent, GameEntity *self, GameEntity *activator )
{
	if ( ent->spawnflags & DECOR_STARTINVIS ) {
		SV_LinkEntity( &ent->shared );
		ent->spawnflags &= ~DECOR_STARTINVIS;
	} else if ( ent->spawnflags & DECOR_ANIMATE )     {
		ent->nextthink = level.time + 50;
		ent->think = props_decoration_animate;
	} else {
		SV_UnlinkEntity( &ent->shared );
		ent->spawnflags |= DECOR_STARTINVIS;
	}

}

void props_touch( GameEntity *self, GameEntity *other, trace_t *trace )
{
	if ( self->spawnflags & DECOR_TOUCHACTIVATE ) {
		props_decoration_death( self, other, other, 9999, MOD_CRUSH );
	}
}

void SP_props_decoration( GameEntity *ent )
{
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

	const char        *loop;

	const char        *startonframe;

	if ( G_SpawnString( "startonframe", "0", &startonframe ) ) {
		ent->shared.s.frame = atoi( startonframe );
	}

	if ( ent->model2 ) {
		ent->shared.s.modelindex = G_ModelIndex( ent->model2 );
	}

	if ( G_SpawnString( "noise", "100", &sound ) ) {
		ent->shared.s.loopSound = G_SoundIndex( sound );
	}

	if ( ( ent->spawnflags & DECOR_LOOPING ) && G_SpawnString( "loop", "100", &loop ) ) {
		ent->props_frame_state = atoi( loop );
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

	if ( ent->health ) {
		ent->isProp = true;
		ent->takedamage = true;
		ent->die = props_decoration_death;

		G_SpawnString( "type", "wood", &type );
		if ( !Q_stricmp( type,"wood" ) ) {
			ent->key = 1;
		} else if ( !Q_stricmp( type,"glass" ) ) {
			ent->key = 0;
		} else if ( !Q_stricmp( type,"metal" ) )  {
			ent->key = 2;
		} else if ( !Q_stricmp( type,"ceramic" ) ) {
			ent->key = 3;
		} else if ( !Q_stricmp( type, "rubble" ) ) {
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

		if ( Q_stricmp( ent->classname, "props_decorBRUSH" ) ) {
			VectorSet( ent->shared.r.mins, -width, -width, 0 );
			VectorSet( ent->shared.r.maxs, width, width, height );
		}

		ent->clipmask   = CONTENTS_SOLID;
		ent->shared.r.contents = CONTENTS_SOLID;
		ent->shared.s.eType = ET_MOVER;

		G_SpawnString( "frames", "0", &frames );
		num_frames = atof( frames );

		ent->count2 = num_frames;

		if ( ent->targetname ) {
			ent->use = Use_props_decoration;
		}

		ent->touch = props_touch;

	} else if ( !( ent->health ) && ent->spawnflags & DECOR_ANIMATE )       {
		G_SpawnString( "frames", "0", &frames );
		num_frames = atof( frames );

		ent->count2 = num_frames;
		ent->use = Use_props_decoration;
	}

	if ( ent->spawnflags & DECOR_STARTON ) {
		ent->nextthink = level.time + 50;
		ent->think = props_decoration_animate;
	}


	G_SetOrigin( ent, ent->shared.s.origin );
	G_SetAngle( ent, ent->shared.s.angles );

	if ( !( ent->spawnflags & DECOR_STARTINVIS ) ) {
		SV_LinkEntity( &ent->shared );
	} else {
		ent->use = Use_props_decoration;
	}

}

/*QUAKED props_decorBRUSH (.6 .7 .7) ? STARTINVIS DEBRIS ANIMATE KEEPBLOCK - LOOPING STARTON
ANIMATE animate on death
STARTON playanimation on death
must have an origin brush

"model2" will specify the model to load

"noise"  the looping sound entity is to make

"type" type of debris ("glass", "wood", "metal", "ceramic", "rubble") default is "wood"
"count" how much debris ei. default 4 pieces

"frames"	how many frames of animation to play
"loop" when the animation is done start again on this frame
"startonframe" on what frame do you want to start the animation
*/

void SP_props_decorBRUSH( GameEntity *self )
{
	SV_SetBrushModel( &self->shared, self->model );

	SP_props_decoration( self );

	if ( self->model2 ) {
		self->shared.s.modelindex2 = G_ModelIndex( self->model2 );
	}

}


/*QUAKED props_decoration_scale (.6 .7 .7) (-8 -8 0) (8 8 16) STARTINVIS DEBRIS ANIMATE KEEPBLOCK TOUCHACTIVATE LOOPING STARTON

"modelscale" - Scale multiplier (defaults to 1.0 and scales uniformly)
"modelscale_vec" - Set scale per-axis.  Overrides "modelscale", so if you have both the "modelscale" is ignored

"model2" will specify the model to load

"noise"  the looping sound entity is to make

"type" type of debris ("glass", "wood", "metal", "ceramic", "rubble") default is "wood"
"count" how much debris ei. default 4 pieces

you will need to specify the bounding box for the entity
"high"  default is 4
"wide"	default is 4

"frames"	how many frames of animation to play
"loop" when the animation is done start again on this frame
"startonframe" on what frame do you want to start the animation
*/

void SP_props_decor_Scale( GameEntity *ent )
{
	float scale[3] = {1,1,1};
	vec3_t scalevec;

	SP_props_decoration( ent );

	ent->shared.s.eType        = ET_GAMEMODEL;

	// look for general scaling
	if ( G_SpawnFloat( "modelscale", "1", &scale[0] ) ) {
		scale[2] = scale[1] = scale[0];
	}

	// look for axis specific scaling
	if ( G_SpawnVector( "modelscale_vec", "1 1 1", &scalevec[0] ) ) {
		VectorCopy( scalevec, scale );
	}

	// scale is stored in 'angles2'
	VectorCopy( scale, ent->shared.s.angles2 );

	SV_LinkEntity( &ent->shared );
}
