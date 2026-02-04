#include "../gameEntity.h"
#include "../g_local.h"             // for spawn stuff
#include "../../server/server.h"    // SV_LinkEntity, SV_UnlinkEntity

extern const char *hintStrings[];

void G_BlockThink( GameEntity *ent ) {
	if ( ent->shared.r.linked ) {
		G_SetAASBlockingEntity( ent, true );
	} else {
		G_SetAASBlockingEntity( ent, false );
	}
}

void ThrowDebris( GameEntity *self, char *modelname, float speed, vec3_t origin ) {
	// probably use le->leType = LE_FRAGMENT like brass and gibs
}

/*
	nuke the original entity
*/
void ClearExplosive( GameEntity *self ) {
	// RF, AAS areas are now free
	if ( !( self->spawnflags & 16 ) ) {
		G_SetAASBlockingEntity( self, false );
	}

	self->die   = nullptr;
	self->pain  = nullptr;
	self->touch = nullptr;
	self->use   = nullptr;
	self->nextthink = level.time + FRAMETIME;
	self->think = G_FreeEntity;


	G_FreeEntity( self );
}



/*
==============
func_explosive_explode
	NOTE: the 'damage' passed in is ignored completely
==============
*/
void func_explosive_explode( GameEntity *self, GameEntity *inflictor, GameEntity *attacker, int damage, int mod ) {
	vec3_t origin;
	vec3_t size;
	vec3_t dir = {0, 0, 1};
	GameEntity   *tent = 0;
	int timeToDeath;            //----(SA)	added

	self->takedamage = false;          // don't allow anything try to hurt me now that i'm exploding

	self->touch = nullptr; //----(SA)	added

	if ( self->wait >= 0 ) {

		self->think = ClearExplosive;

		timeToDeath = (int)( self->wait * 1000.0f ) + FRAMETIME;

		self->nextthink = level.time + timeToDeath; // delay removal until the animation has played out and leave as long as the user requested
		self->shared.s.time = timeToDeath < 3000 ? timeToDeath : self->nextthink - 3000;   // 3 sec fade at end, unless the life time is less than 2 seconds, then fade from now to death
		self->shared.s.time2 = self->nextthink;
	}

	VectorSubtract( self->shared.r.absmax, self->shared.r.absmin, size );
	VectorScale( size, 0.5, size );
	VectorAdd( self->shared.r.absmin, size, origin );

	self->shared.s.frame = 1;  // play death animation

	VectorCopy( origin, self->shared.s.origin2 );    // (SA) changed so i can have the model remain for a bit, but still have the explosion at the 'center'

	G_UseTargets( self, attacker );

	self->shared.s.density = self->count;      // pass the "mass" to the client
	self->shared.s.weapon = self->duration;    // pass the "force lowgrav" to client
	self->shared.s.effect3Time = self->key;            // pass the type to the client ("glass", "wood", "metal", "gibs", "brick", "stone", "fabric", 0, 1, 2, 3, 4, 5, 6)

	if ( self->damage ) {
		G_RadiusDamage( origin, self, self->damage, self->damage + 40, self, MOD_EXPLOSIVE );
	}

	// find target, aim at that
	if ( self->target ) {

		// since the explosive might need to fire the target rather than
		// aim at it, only aim at 'info_notnull' ents
		while ( 1 )
		{
			tent = G_Find( tent, FOFS( targetname ), self->target );
			if ( !tent ) {
				break;
			}

			if ( !Q_stricmp( tent->classname, "info_notnull" ) ) {
				break;  // found an info_notnull
			}
		}

		if ( tent ) {
			VectorSubtract( tent->shared.s.pos.trBase, origin, dir );
			VectorNormalize( dir );
		}
	}

	// if a valid target entity was not found, check for a specified 'angle' for the explosion direction
	if ( !tent && !self->model2 ) {    // if there's a 'model2', the dir is for that, not the explosion
		if ( self->shared.s.angles[1] ) {
			// up
			if ( self->shared.s.angles[1] == -1 ) {
				// it's 'up' by default
			}
			// down
			else if ( self->shared.s.angles[1] == -2 ) {
				dir[2] = -1;
			}
			// yawed
			else
			{
				RotatePointAroundVector( dir, dir, tv( 1, 0, 0 ), self->shared.s.angles[1] );
			}
		}
	}

	G_AddEvent( self, EV_EXPLODE, DirToByte( dir ) );

}

void func_explosive_touch( GameEntity *self, GameEntity *other, trace_t *trace ) {
	func_explosive_explode( self, self, other, self->damage, 0 );
}


void func_explosive_use( GameEntity *self, GameEntity *other, GameEntity *activator ) {
	func_explosive_explode( self, self, other, self->damage, 0 );
}

void func_explosive_alert( GameEntity *self ) {
	func_explosive_explode( self, self, self, self->damage, 0 );
}

void func_explosive_spawn( GameEntity *self, GameEntity *other, GameEntity *activator )
{
	SV_LinkEntity( &self->shared );
	self->use = func_explosive_use;
	// turn the brush to visible

	// RF, AAS areas are now occupied
	if ( !( self->spawnflags & 16 ) ) {
		G_SetAASBlockingEntity( self, true );
	}
}


void InitExplosive( GameEntity *ent )
{
	// if the "model2" key is set, use a seperate model
	// for drawing, but clip against the brushes
	if ( ent->model2 ) {
		int numLiving, numDead;

		ent->shared.s.modelindex2 = G_ModelIndex( ent->model2 );

		// (assume start frame of 'living' is frame '0')
		G_SpawnInt( "numLivingFrames", "0", &numLiving );
		G_SpawnInt( "numDeadFrames", "0", &numDead );

		// these are used later for the explosion,
		// so we can use them now for the animation info
		ent->shared.s.effect3Time = numLiving;
		ent->shared.s.density = numDead;
	}
	ent->shared.s.frame = 0;

	// pick it up if the level designer uses "damage" instead of "dmg"
    const char        *damage;
	if ( G_SpawnString( "damage", "0", &damage ) ) {
		ent->damage = atoi( damage );
	}

	ent->shared.s.eType = ET_EXPLOSIVE;
	SV_LinkEntity( &ent->shared );

	if ( !( ent->spawnflags & 16 ) ) {
		ent->think = G_BlockThink;
		ent->nextthink = level.time + FRAMETIME;
	}
}


/*QUAKED func_explosive (0 .5 .8) ? START_INVIS TOUCHABLE USESHADER LOWGRAV NOBLOCKAAS EXPLO DYNOMITE
EXPLO only explosives can damage it rockets grendades etc
DYNOMITE only can be damaged by DY-NO-MITE!
Any brush that you want to explode or break apart.  If you want an explosion, set dmg and it will do a radius explosion of that amount at the center of the bursh.
TOUCHABLE means automatic use on player contact.
USESHADER will apply the shader used on the brush model to the debris.
LOWGRAV specifies that the debris will /always/ fall slowly
"model2" optional md3 to draw over the solid clip brush
"numLivingFrames" - how many frames to loop while the ent is 'alive' (model should have two animations: living->dead. living always starts at frame '0')
"numDeadFrames" - how many frames to play when the ent 'dies' ('nnnFrames' parameters must go together)
"wait" - how long (in seconds) to leave the model after it's 'dead'.  '-1' leaves forever.
NOTE: if you use model2, you must have an origin brush in the explosive with the center of the origin at the origin of the model
"item" - when it explodes, pop this item out with the debirs (use QUAKED name. ex: "item_health_small")
"dmg" - how much radius damage should be done, defaults to 0
"health" - defaults to 100.  If health is set to '0' the brush will not be shootable.
"targetname" - if set, no touch field will be spawned and a remote button or trigger field triggers the explosion.
"type" - type of debris ("glass", "wood", "metal", "gibs", "brick", "rock", "fabric") default is "wood"
"mass" - defaults to 75.  This determines how much debris is emitted when it explodes.  You get one large chunk per 100 of mass (up to 8) and one small chunk per 25 of mass (up to 16).  So 800 gives the most.
"noise" - sound to play when triggered.  The explosive will default to a sound that matches it's 'type'.  Use the sound name "nosound" (case in-sensitive) if you want it silent.
"leaktype" - leaks particles of this type
"leakpressure" - force the particles come out with.  '0' would cause them to fall straight down
"leaktime" - how long it leaks before it quits
"leakcount" - how many holes in the entity before it no longer leaks.
the default sounds are:
  "wood"	- "sound/world/boardbreak.wav"
  "glass"	- "sound/world/glassbreak.wav"
  "metal"	- "sound/world/metalbreak.wav"
  "gibs"	- "sound/player/gibsplit1.wav"
  "brick"	- "sound/world/brickfall.wav"
  "stone"	- "sound/world/stonefall.wav"
  "fabric"	- "sound/world/metalbreak.wav"	// (SA) temp
'leaktypes':
1:oil
2:water
3:steam
4:wine
5:smoke
6:electrical
*/
/*
"fxdensity" size of explosion 1 - 100 (default is 10)
*/
void SP_func_explosive( GameEntity *ent ) {
	int health, mass, dam, i;
	char buffer[MAX_QPATH];
	const char    *s;
	const char    *type;
	const char    *cursorhint;

	SV_SetBrushModel( &ent->shared, ent->model );
	InitExplosive( ent );

	if ( ent->spawnflags & 1 ) {  // start invis
		ent->use = func_explosive_spawn;
		SV_UnlinkEntity( &ent->shared );
	} else if ( ent->targetname )     {
		ent->use = func_explosive_use;
		ent->AIScript_AlertEntity = func_explosive_alert;
	}


	if ( ent->spawnflags & 2 ) {  // touchable
		ent->touch = func_explosive_touch;
	} else {
		ent->touch = nullptr;
	}

	if ( ( ent->spawnflags & 4 ) && ent->model && strlen( ent->model ) ) {   // use shader
		ent->shared.s.eFlags |= EF_INHERITSHADER;
	}

	if ( ent->spawnflags & 8 ) {  // force lowgravity
		ent->duration = 1;
	}

	G_SpawnInt( "health", "100", &health );
	ent->health = health;

	G_SpawnInt( "dmg", "0", &dam );
	ent->damage = dam;

	if ( ent->health ) {
		ent->takedamage = true;
	}

	if ( G_SpawnInt( "mass", "75", &mass ) ) {
		ent->count = mass;
	} else {
		ent->count = 75;
	}

	if ( G_SpawnString( "type", "wood", &type ) ) {
		if ( !Q_stricmp( type,"wood" ) ) {
			ent->key = 0;
		} else if ( !Q_stricmp( type,"glass" ) ) {
			ent->key = 1;
		} else if ( !Q_stricmp( type,"metal" ) ) {
			ent->key = 2;
		} else if ( !Q_stricmp( type,"gibs" ) )  {
			ent->key = 3;
		} else if ( !Q_stricmp( type,"brick" ) )  {
			ent->key = 4;
		} else if ( !Q_stricmp( type,"rock" ) )  {
			ent->key = 5;
		} else if ( !Q_stricmp( type,"fabric" ) ) {
			ent->key = 6;
		}
	} else {
		ent->key = 0;
	}

	if ( G_SpawnString( "noise", "NOSOUND", &s ) ) {
		if ( Q_stricmp( s, "nosound" ) ) {
			Q_strncpyz( buffer, s, sizeof( buffer ) );
			ent->shared.s.dl_intensity = G_SoundIndex( buffer );
		}
	} else {
		switch ( ent->key ) {
		case 0:     // "wood"
			ent->shared.s.dl_intensity = G_SoundIndex( "sound/world/boardbreak.wav" );
			break;
		case 1:     // "glass"
			ent->shared.s.dl_intensity = G_SoundIndex( "sound/world/glassbreak.wav" );
			break;
		case 2:     // "metal"
			ent->shared.s.dl_intensity = G_SoundIndex( "sound/world/metalbreak.wav" );
			break;
		case 3:     // "gibs"
			ent->shared.s.dl_intensity = G_SoundIndex( "sound/player/gibsplit1.wav" );
			break;
		case 4:     // "brick"
			ent->shared.s.dl_intensity = G_SoundIndex( "sound/world/brickfall.wav" );
			break;
		case 5:     // "stone"
			ent->shared.s.dl_intensity = G_SoundIndex( "sound/world/stonefall.wav" );
			break;

		default:
			break;
		}
	}

	ent->shared.s.dmgFlags = 0;

	if ( G_SpawnString( "cursorhint", "0", &cursorhint ) ) {

		for ( i = 1; i < HINT_NUM_HINTS; i++ ) {  // skip "HINT_NONE"
			if ( !Q_strcasecmp( cursorhint, hintStrings[i] ) ) {
				ent->shared.s.dmgFlags = i;
				break;
			}
		}
	}

	ent->die = func_explosive_explode;
}
