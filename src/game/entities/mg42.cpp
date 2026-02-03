#include "../gameEntity.h"
#include "../g_local.h"             // for spawn stuff
#include "../../server/server.h"    // SV_LinkEntity, SV_UnlinkEntity
#include "../idlib/math/Math.h"

static vec3_t forward, right, up;
static vec3_t muzzle;

static int snd_noammo;

void flakPuff( vec3_t origin, bool sky ) {
	GameEntity *tent;
	vec3_t point;

	VectorCopy( origin, point );
	if ( sky ) {
		VectorMA( point, -256, forward, point );
	}

	point[2] += 16; // raise puff off the ground some
	tent = G_TempEntity( point, EV_SMOKE );
	VectorCopy( point, tent->shared.s.origin );
	tent->shared.s.time = 2000;
	tent->shared.s.time2 = 1000;
	tent->shared.s.density = 0;
	tent->shared.s.angles2[0] = 16 + 8;
	tent->shared.s.angles2[1] = 48 + 24;
	tent->shared.s.angles2[2] = 10;
}

int muzzleflashmodel;

void mg42_muzzleflash( GameEntity *ent, vec3_t muzzlepos ) {  // cheezy, but lets me use this routine for finding the muzzle point for firing the actual bullet

	vec3_t forward;
	vec3_t point;
	GameEntity   *flash;

	flash = G_Spawn();

	if ( flash ) {
		VectorCopy( ent->shared.s.pos.trBase, flash->shared.s.origin );
		VectorCopy( ent->shared.s.apos.trBase, flash->shared.s.angles );
		G_SetAngle( flash, ent->shared.s.apos.trBase );
		G_SetOrigin( flash, ent->shared.s.pos.trBase );

		VectorCopy( flash->shared.s.origin, point );
		AngleVectors( flash->shared.s.angles, forward, nullptr, nullptr );
		VectorMA( point, 40, forward, point );

		if ( muzzlepos ) {
			VectorCopy( point, muzzlepos );
		}

		G_SetOrigin( flash, point );

		flash->shared.s.modelindex = muzzleflashmodel;

		flash->shared.r.contents = CONTENTS_TRIGGER;
		flash->shared.r.svFlags = SVF_USE_CURRENT_ORIGIN;
		flash->shared.s.eType = ET_GENERAL;

		flash->think = G_FreeEntity;
		flash->nextthink = level.time + 50;

		SV_LinkEntity( &flash->shared );
	}

}


//----(SA)	added 'activator' so the bits that used to expect 'ent' to be the gun still work
void Fire_Lead( GameEntity *ent, GameEntity *activator, float spread, int damage ) {
	trace_t tr;
	vec3_t end, lead_muzzle, mg42_muzzle;
	float r;
	float u;
	GameEntity       *tent;
	GameEntity       *traceEnt;

	//bool	isflak = false;

	// 'muzzle' is bogus for mg42.  adjust for it
	if ( !Q_stricmp( ent->classname, "misc_mg42" ) ) {
		mg42_muzzleflash( ent, mg42_muzzle );    // get current position for mg42 muzzle flash/bullet origin
		VectorCopy( mg42_muzzle, lead_muzzle );
	} else {
		VectorCopy( muzzle, lead_muzzle );
	}

	r = crandom() * spread;
	u = crandom() * spread;
	VectorMA( lead_muzzle, 8192, forward, end );
	VectorMA( end, r, right, end );
	VectorMA( end, u, up, end );

	SV_Trace( &tr, lead_muzzle, nullptr, nullptr, end, ent->shared.s.number, MASK_SHOT, false );

	AICast_ProcessBullet( activator, lead_muzzle, tr.endpos );

	if ( tr.surfaceFlags & SURF_NOIMPACT ) {
		if ( !Q_stricmp( ent->classname, "misc_flak" ) ) {
			if ( ent->count == 1 ) {
				G_AddEvent( ent, EV_FLAKGUN1, 0 );
			} else if ( ent->count == 2 ) {
				G_AddEvent( ent, EV_FLAKGUN2, 0 );
			} else if ( ent->count == 3 ) {
				G_AddEvent( ent, EV_FLAKGUN3, 0 );
			} else if ( ent->count == 4 ) {
				G_AddEvent( ent, EV_FLAKGUN4, 0 );
			}

			flakPuff( tr.endpos, true );
		} else {
//			mg42_muzzleflash (ent, 0);
			G_AddEvent( ent, EV_FIRE_WEAPON_MG42, 0 );
		}
		return;
	}

	traceEnt = &g_entities[ tr.entityNum ];

	// snap the endpos to integers, but nudged towards the line
	SnapVectorTowards( tr.endpos, lead_muzzle );

	// send bullet impact
	if ( traceEnt->takedamage && traceEnt->client ) {
		tent = G_TempEntity( tr.endpos, EV_BULLET_HIT_FLESH );
		tent->shared.s.eventParm = traceEnt->shared.s.number;
		tent->shared.s.otherEntityNum = ent->shared.s.number;

		if ( LogAccuracyHit( traceEnt, ent ) ) {
			ent->client->ps.persistant[PERS_ACCURACY_HITS]++;
		}

	} else {
		// Ridah, bullet impact should reflect off surface
		vec3_t reflect;
		float dot;

		if ( !Q_stricmp( ent->classname, "misc_mg42" ) ) {
			tent = G_TempEntity( tr.endpos, EV_BULLET_HIT_WALL );

			dot = DotProduct( forward, tr.plane.normal );
			VectorMA( forward, -2 * dot, tr.plane.normal, reflect );
			VectorNormalize( reflect );

			tent->shared.s.eventParm = DirToByte( reflect );
			tent->shared.s.otherEntityNum = ent->shared.s.number;
			tent->shared.s.otherEntityNum2 = activator->shared.s.number;  // (SA) store the user id, so the client can position the tracer
		}
		// done.
	}

	if ( traceEnt->takedamage ) {
		G_Damage( traceEnt, ent, ent, forward, tr.endpos,
				  damage, 0, MOD_MACHINEGUN );
	}

	if ( !Q_stricmp( ent->classname, "misc_mg42" ) ) {
		G_AddEvent( ent, EV_FIRE_WEAPON_MG42, 0 );
	} else if ( !Q_stricmp( ent->classname, "misc_flak" ) ) {
		if ( ent->count == 1 ) {
			G_AddEvent( ent, EV_FLAKGUN1, 0 );
		} else if ( ent->count == 2 ) {
			G_AddEvent( ent, EV_FLAKGUN2, 0 );
		} else if ( ent->count == 3 ) {
			G_AddEvent( ent, EV_FLAKGUN3, 0 );
		} else if ( ent->count == 4 ) {
			G_AddEvent( ent, EV_FLAKGUN4, 0 );
		}

		flakPuff( tr.endpos, false );
	}

}

float AngleDifference( float ang1, float ang2 );

void clamp_hweapontofirearc( GameEntity *self, GameEntity *other, vec3_t dang ) {

// NOTE: use this value, and THEN the cl_input.c scales to tweak the feel
#define MG42_YAWSPEED       300.0   // degrees per second
#define MG42_IDLEYAWSPEED   80.0    // degrees per second (while returning to base)

	int i;
	float diff, yawspeed;
	bool clamped;

	clamped = false;

	if ( other ) {
		VectorCopy( self->TargetAngles, dang );
		yawspeed = MG42_YAWSPEED;
	} else {    // go back to start position
		VectorCopy( self->shared.s.angles, dang );
		yawspeed = MG42_IDLEYAWSPEED;
	}

	if ( dang[0] < 0 && dang[0] < -( self->varc ) ) {
		clamped = true;
		dang[0] = -( self->varc );
	}

// NOTE to self this change has damaged visualy all mg42 behind sandbags
	if ( other && other->shared.r.svFlags & SVF_CASTAI ) {
		if ( self->spawnflags & 1 ) {
			if ( dang[0] > 0 && dang[0] > 20.0 ) {
				clamped = true;
				dang[0] = 20.0;
			}
		} else if ( dang[0] > 0 && dang[0] > 10.0 )     {
			clamped = true;
			dang[0] = 10.0;
		}
	} else
	{
		if ( self->spawnflags & 1 ) {
			if ( dang[0] > 0 && dang[0] > ( self->varc / 2 ) ) {
				clamped = true;
				dang[0] = self->varc / 2;
			}
		} else if ( dang[0] > 0 && dang[0] > ( self->varc / 2 ) )    {
			clamped = true;
			dang[0] = self->varc / 2;
		}
	}

//	Com_Printf ("dang[0] = %5.2f\n", dang[0]);

	if ( !Q_stricmp( self->classname, "misc_mg42" ) || !( self->active ) ) {
		diff = AngleDifference( dang[YAW], self->shared.s.angles[YAW] );
		if ( fabs( diff ) > self->harc ) {
			clamped = true;
			if ( diff > 0 ) {
				dang[YAW] = AngleMod( self->shared.s.angles[YAW] + self->harc );
			} else {
				dang[YAW] = AngleMod( self->shared.s.angles[YAW] - self->harc );
			}
		}

		// dang is now the ideal angles
		for ( i = 0; i < 3; i++ ) {
			BG_EvaluateTrajectory( &self->shared.s.apos, level.time, self->shared.r.currentAngles );
			diff = AngleDifference( dang[i], self->shared.r.currentAngles[i] );
			if ( fabs( diff ) > ( yawspeed * ( (float)FRAMETIME / 1000.0 ) ) ) {
				clamped = true;
				if ( diff > 0 ) {
					dang[i] = AngleMod( self->shared.r.currentAngles[i] + ( yawspeed * ( (float)FRAMETIME / 1000.0 ) ) );
				} else {
					dang[i] = AngleMod( self->shared.r.currentAngles[i] - ( yawspeed * ( (float)FRAMETIME / 1000.0 ) ) );
				}
			}
		}
	}

	if ( other && other->shared.r.svFlags & SVF_CASTAI ) {
		clamped = false;
	}

	// sanity check the angles again to make sure we don't go passed the harc whilst trying to get to the other side
	// diff = AngleDifference( dang[YAW], self->shared.s.angles[YAW] );
	diff = AngleDifference( self->shared.s.angles[YAW], dang[YAW] );
	// if (fabs(diff) > self->harc) {
	if ( fabs( diff ) > self->harc && other && other->shared.r.svFlags & SVF_CASTAI ) {
		clamped = true;

		if ( diff > 0 ) {
			dang[YAW] = AngleMod( self->shared.s.angles[YAW] + self->harc );
		} else {
			dang[YAW] = AngleMod( self->shared.s.angles[YAW] - self->harc );
		}

//		Com_Printf ("dang %5.2f ang %5.2f diff %5.2f\n", dang[YAW], self->shared.s.angles[YAW], diff);

	}
//	else
//		Com_Printf ("not clamped cang %5.2f\n", self->TargetAngles[YAW]);


	if ( other && clamped ) {
		// we only do this to keep the input angles close to the weapon, this doesn't actually
		// effect the view
		SetClientViewAngle( other, dang );

		//if they are an AI, they should dismount now
		if ( other->shared.r.svFlags & SVF_CASTAI ) {
			if ( !other->mg42ClampTime ) {
				other->mg42ClampTime = level.time;
			} else if ( other->mg42ClampTime < level.time - 750 ) {
				other->active = false;
			}
		}
	} else if ( other ) {
		other->mg42ClampTime = 0;
	}


	if ( g_mg42arc.integer ) {
		Com_Printf( "varc = %5.2f\n", dang[0] );
	}
}

// NOTE: this only effects the external view of the user, when using the mg42, the
// view position is set on the client-side to keep it firm behind the gun with
// interpolation
void clamp_playerbehindgun( GameEntity *self, GameEntity *other, vec3_t dang ) {
	vec3_t forward, right, up;
	vec3_t point;


	AngleVectors( self->shared.s.apos.trBase, forward, right, up );
	VectorMA( self->shared.r.currentOrigin, -36, forward, point );

	point[2] = other->shared.r.currentOrigin[2];
	SV_UnlinkEntity( &other->shared );
	VectorCopy( point, other->client->ps.origin );

	// save results of pmove
	BG_PlayerStateToEntityState( &other->client->ps, &other->shared.s, true );

	// use the precise origin for linking
	VectorCopy( other->client->ps.origin, other->shared.r.currentOrigin );

	SV_LinkEntity( &other->shared );
}

#define MG42_SPREAD 200
#define MG42_DAMAGE 18
#define MG42_DAMAGE_AI  9
#define FIREARC         120

#define FLAK_SPREAD 100
#define FLAK_DAMAGE 36

void mg42_touch( GameEntity *self, GameEntity *other, trace_t *trace ) {
	vec3_t dang;
	int i;

	if ( !self->active ) {
		return;
	}

	if ( other->active ) {
		for ( i = 0; i < 3; i++ )
			dang[i] = SHORT2ANGLE( other->client->pers.cmd.angles[i] );

		// the gun should go to our current angles next time it thinks
		VectorCopy( dang, self->TargetAngles );
		//VectorCopy( other->client->ps.viewangles, self->TargetAngles );

		// now tell the client to lock the view in the direction of the gun
		//if (other->shared.r.svFlags & SVF_CASTAI) {
		other->client->ps.viewlocked = 1;
		other->client->ps.viewlocked_entNum = self->shared.s.number;
		//}

		if ( self->shared.s.frame ) {
			other->client->ps.gunfx = 1;
		} else {
			other->client->ps.gunfx = 0;
		}

		// clamp the mg42 to fire arc
		VectorCopy( other->client->ps.viewangles, self->TargetAngles );

		clamp_hweapontofirearc( self, other, dang );

		// clamp player behind the gun
		clamp_playerbehindgun( self, other, dang );

		VectorCopy( dang, self->TargetAngles );
	}
}

void mg42_track( GameEntity *self, GameEntity *other ) {
	vec3_t dang;
	int i;
	bool validshot = false;
	bool is_flak = false;

	if ( !Q_stricmp( self->classname, "misc_flak" ) ) {
		is_flak = true;
	}

	if ( !self->active ) {
		return;
	}

	if ( other->active ) {
		if ( ( !( level.time % 100 ) ) && ( other->client ) && ( other->client->buttons & BUTTON_ATTACK ) ) {
			if ( self->shared.s.frame && !is_flak ) {
				// Com_Printf ("gun: destroyed = %d\n", self->shared.s.frame);
				G_AddEvent( self, EV_GENERAL_SOUND, snd_noammo );
				other->client->ps.gunfx = 1;
			} else
			{
				AngleVectors( self->shared.s.apos.trBase, forward, right, up );
				VectorCopy( self->shared.s.pos.trBase, muzzle );

				if ( !Q_stricmp( self->classname, "misc_mg42" ) ) {
					VectorMA( muzzle, 16, forward, muzzle );
					VectorMA( muzzle, 16, up, muzzle );
					validshot = true;
				} else if ( !Q_stricmp( self->classname, "misc_flak" ) )       {
					if ( self->delay < level.time ) {
						self->delay = level.time + 250;

						self->count++;

						if ( self->count > 4 ) {
							self->count = 1;
						}

						// guns 1 and 2 were switched
						if ( self->count == 2 ) {
							VectorMA( muzzle, 72, forward, muzzle );
							VectorMA( muzzle, 31, up, muzzle );
							VectorMA( muzzle, 22, right, muzzle );
						} else if ( self->count == 1 )     {
							VectorMA( muzzle, 72, forward, muzzle );
							VectorMA( muzzle, 31, up, muzzle );
							VectorMA( muzzle, -22, right, muzzle );
						} else if ( self->count == 3 )     {
							VectorMA( muzzle, 72, forward, muzzle );
							VectorMA( muzzle, 10, up, muzzle );
							VectorMA( muzzle, 22, right, muzzle );
						} else if ( self->count == 4 )     {
							VectorMA( muzzle, 72, forward, muzzle );
							VectorMA( muzzle, 10, up, muzzle );
							VectorMA( muzzle, -22, right, muzzle );
						}

						validshot = true;
						self->shared.s.frame++;
					}
				}
				// snap to integer coordinates for more efficient network bandwidth usage
				SnapVector( muzzle );

				if ( validshot ) {
					if ( !( other->shared.r.svFlags & SVF_CASTAI ) ) {
						if ( is_flak ) {
							Fire_Lead( self, other, FLAK_SPREAD, FLAK_DAMAGE );
						} else
						{
							Fire_Lead( self, other, MG42_SPREAD, MG42_DAMAGE );
						}
					} else
					{
						if ( self->damage ) {
							Fire_Lead( self, other, MG42_SPREAD / self->accuracy, self->damage );
						} else {
							Fire_Lead( self, other, MG42_SPREAD / self->accuracy, MG42_DAMAGE_AI );
						}

					}

					// play character anim
					BG_AnimScriptEvent( &other->client->ps, ANIM_ET_FIREWEAPON, false, true );

					other->client->ps.viewlocked = 2; // this enable screen jitter when firing
				}
			}
		} else {
			other->client->ps.viewlocked = 1;
		}

		// move to the position over the next frame
		VectorCopy( self->TargetAngles, dang );
		VectorSubtract( dang, self->shared.s.apos.trBase, self->shared.s.apos.trDelta );
		for ( i = 0; i < 3; i++ ) {
			self->shared.s.apos.trDelta[i] = AngleNormalize180( self->shared.s.apos.trDelta[i] );
		}
		VectorScale( self->shared.s.apos.trDelta, 1000 / 50, self->shared.s.apos.trDelta );
		self->shared.s.apos.trTime = level.time;
		self->shared.s.apos.trDuration = 50;
	}
}

#define GUN1_IDLE   0
#define GUN2_IDLE   4
#define GUN3_IDLE   8
#define GUN4_IDLE   12

#define GUN1_LASTFIRE   3
#define GUN2_LASTFIRE   7
#define GUN3_LASTFIRE   11
#define GUN4_LASTFIRE   15

void Flak_Animate( GameEntity *ent ) {
	//Com_Printf ("frame %i\n", ent->shared.s.frame);

	if ( ent->shared.s.frame == GUN1_IDLE
		 || ent->shared.s.frame == GUN2_IDLE
		 || ent->shared.s.frame == GUN3_IDLE
		 || ent->shared.s.frame == GUN4_IDLE ) {
		return;
	}

	if ( ent->count == 1 ) {
		if ( ent->shared.s.frame == GUN1_LASTFIRE ) {
			ent->shared.s.frame = GUN2_IDLE;
		} else if ( ent->shared.s.frame > GUN1_IDLE ) {
			ent->shared.s.frame++;
		}
	} else if ( ent->count == 2 )     {
		if ( ent->shared.s.frame == GUN2_LASTFIRE ) {
			ent->shared.s.frame = GUN3_IDLE;
		} else if ( ent->shared.s.frame > GUN2_IDLE ) {
			ent->shared.s.frame++;
		}
	} else if ( ent->count == 3 )     {
		if ( ent->shared.s.frame == GUN3_LASTFIRE ) {
			ent->shared.s.frame = GUN4_IDLE;
		} else if ( ent->shared.s.frame > GUN3_IDLE ) {
			ent->shared.s.frame++;
		}
	} else if ( ent->count == 4 )     {
		if ( ent->shared.s.frame == GUN4_LASTFIRE ) {
			ent->shared.s.frame = GUN1_IDLE;
		} else if ( ent->shared.s.frame > GUN4_IDLE ) {
			ent->shared.s.frame++;
		}
	}
}

#define USEMG42_DISTANCE 46
void mg42_think( GameEntity *self ) {
	vec3_t vec;
	GameEntity   *owner;
	int i;
	float len;
	float usedist;
	bool is_flak = false;

	if ( !Q_stricmp( self->classname, "misc_flak" ) ) {
		is_flak = true;
		Flak_Animate( self );
	}

	VectorClear( vec );

	owner = &g_entities[self->shared.r.ownerNum];

	// move to the current angles
	BG_EvaluateTrajectory( &self->shared.s.apos, level.time, self->shared.s.apos.trBase );

	if ( owner->client ) {
		VectorSubtract( self->shared.r.currentOrigin, owner->shared.r.currentOrigin, vec );
		len = VectorLength( vec );

		if ( owner->shared.r.svFlags & SVF_CASTAI ) {
			usedist = USEMG42_DISTANCE;
		} else {
			// kinda dumb since the player had to be close enough to activate it to get this
			// far and the start point for the difference is calculated differently each place anyway
			usedist = 999;  // always allow the touch by player

		}
		if ( len < usedist && ( owner->active == 1 ) && owner->health > 0 ) {
			self->active = true;
			if ( is_flak ) {
				owner->client->ps.persistant[PERS_HWEAPON_USE] = 2;
			} else {
				owner->client->ps.persistant[PERS_HWEAPON_USE] = 1;
			}
			mg42_track( self, owner );
			self->nextthink = level.time + 50;

			if ( !( owner->shared.r.svFlags & SVF_CASTAI ) ) {
				clamp_playerbehindgun( self, owner, vec3_origin );
			}

			return;
		}

	}

	// slowly rotate back to position
	//clamp_hweapontofirearc( self, nullptr, vec );
	// move to the position over the next frame
	VectorSubtract( self->shared.s.angles, self->shared.s.apos.trBase, self->shared.s.apos.trDelta );
	for ( i = 0; i < 3; i++ ) {
		self->shared.s.apos.trDelta[i] = AngleNormalize180( self->shared.s.apos.trDelta[i] );
	}
	VectorScale( self->shared.s.apos.trDelta, 400 / 50, self->shared.s.apos.trDelta );
	self->shared.s.apos.trTime = level.time;
	self->shared.s.apos.trDuration = 50;

	self->nextthink = level.time + 50;

	// only let them go when it's pointing forward
	if ( owner->client ) {
		if ( fabs( AngleNormalize180( self->shared.s.angles[YAW] - self->shared.s.apos.trBase[YAW] ) ) > 10 ) {
			BG_EvaluateTrajectory( &self->shared.s.apos, self->nextthink, owner->client->ps.viewangles );
			return; // still waiting
		}
	}

	self->active = false;

	if ( owner->client ) {
		owner->client->ps.eFlags &= ~EF_MG42_ACTIVE;        // whoops, missed this
		owner->client->ps.persistant[PERS_HWEAPON_USE] = 0;
		owner->client->ps.viewlocked = 0;   // let them look around
		owner->active = false;
		owner->client->ps.gunfx = 0;
	}

	self->shared.r.ownerNum = self->shared.s.number;


}

void mg42_die( GameEntity *self, GameEntity *inflictor, GameEntity *attacker, int damage, int mod ) {
	GameEntity   *gun;
	GameEntity   *owner;


	G_Sound( self, self->soundPos3 ); // death sound

	// self->chain not set if no tripod
	if ( self->chain ) {
		gun = self->chain;
	} else {
		gun = self;
	}
	
	owner = &g_entities[gun->shared.r.ownerNum];

	if ( gun && self->health <= 0 ) {
		gun->shared.s.frame = 2;
		gun->takedamage = false;

	}

	self->takedamage = false;

	if ( owner && owner->client ) {
		owner->client->ps.persistant[PERS_HWEAPON_USE] = 0;
		self->shared.r.ownerNum = self->shared.s.number;
		owner->client->ps.viewlocked = 0;   // let them look around
		owner->active = false;
		owner->client->ps.gunfx = 0;

		self->active = false;
		gun->active = false;
	}


	SV_LinkEntity( &self->shared );
}

void mg42_use( GameEntity *ent, GameEntity *other, GameEntity *activator )
{
	GameEntity* owner = &g_entities[ent->shared.r.ownerNum];

	if ( owner && owner->client ) {
		owner->client->ps.persistant[PERS_HWEAPON_USE] = 0;
		ent->shared.r.ownerNum = ent->shared.s.number;
		owner->client->ps.viewlocked = 0;   // let them look around
		owner->active = false;
		owner->client->ps.gunfx = 0;
	}

	SV_LinkEntity( &ent->shared );
}

void mg42_spawn( GameEntity *ent ) {
	GameEntity *base, *gun;
	vec3_t offset;

	ent->soundPos3 = G_SoundIndex( "sound/weapons/mg42/mg42_death.wav" );   // die sound

	//if (!(ent->spawnflags & 2)) // no tripod
	{
		base = G_Spawn();

		if ( !( ent->spawnflags & 2 ) ) { // no tripod
			base->clipmask = CONTENTS_SOLID;
			base->shared.r.contents = CONTENTS_SOLID;
			base->shared.r.svFlags = SVF_USE_CURRENT_ORIGIN;
			base->shared.s.eType = ET_GENERAL;


			base->shared.s.modelindex = G_ModelIndex( "models/mapobjects/weapons/mg42b.md3" );
		}

		VectorSet( base->shared.r.mins, -8, -8, -8 );
		VectorSet( base->shared.r.maxs, 8, 8, 48 );
		VectorCopy( ent->shared.s.origin, offset );
		offset[2] -= 24;
		G_SetOrigin( base, offset );
		base->shared.s.apos.trType = TR_STATIONARY;
		base->shared.s.apos.trTime = 0;
		base->shared.s.apos.trDuration = 0;
		base->shared.s.dmgFlags = HINT_MG42;   // identify this for cursorhints
		VectorCopy( ent->shared.s.angles, base->shared.s.angles );
		VectorCopy( base->shared.s.angles, base->shared.s.apos.trBase );
		VectorCopy( base->shared.s.angles, base->shared.s.apos.trDelta );
		base->health = ent->health;
		base->target = ent->target; //----(SA)	added so mounting mg42 can trigger targets
		base->takedamage = true;
		base->die = mg42_die;
		base->soundPos3 = ent->soundPos3;   //----(SA)
		base->activateArc = ent->activateArc;           //----(SA)	added
		SV_LinkEntity( &base->shared );
	}

	gun = G_Spawn();
	gun->classname = "misc_mg42";
	gun->clipmask = CONTENTS_SOLID;
	gun->shared.r.contents = CONTENTS_TRIGGER;
	gun->shared.r.svFlags = SVF_USE_CURRENT_ORIGIN;
	gun->shared.s.eType = ET_MG42;

	// DHM - Don't need to specify here, handled in G_CheckForCursorHints
	//gun->shared.s.dmgFlags = HINT_MG42;	// identify this for cursorhints

	gun->touch = mg42_touch;
	gun->shared.s.modelindex = G_ModelIndex( "models/mapobjects/weapons/mg42a.md3" );
	VectorCopy( ent->shared.s.origin, offset );
	offset[2] += 24;
	G_SetOrigin( gun, offset );
	VectorSet( gun->shared.r.mins, -24, -24, -8 );
	VectorSet( gun->shared.r.maxs, 24, 24, 48 );
	gun->shared.s.apos.trTime = 0;
	gun->shared.s.apos.trDuration = 0;
	VectorCopy( ent->shared.s.angles, gun->shared.s.angles );
	VectorCopy( gun->shared.s.angles, gun->shared.s.apos.trBase );
	VectorCopy( gun->shared.s.angles, gun->shared.s.apos.trDelta );

	VectorCopy( ent->shared.s.angles, gun->shared.s.angles2 );

	gun->think = mg42_think;
	gun->nextthink = level.time + FRAMETIME;
	gun->shared.s.number = gun - g_entities;
	gun->harc = ent->harc;
	gun->varc = ent->varc;
	gun->shared.s.apos.trType = TR_LINEAR_STOP;    // interpolate the angles
	gun->takedamage = true;
	gun->targetname = ent->targetname;      // need this for scripting
	gun->damage = ent->damage;
	gun->health = ent->health;  //----(SA)	added
	gun->accuracy = ent->accuracy;
	gun->target = ent->target;  //----(SA)	added so mounting mg42 can trigger targets
	gun->use = mg42_use;
	gun->die = mg42_die; // JPW NERVE we want it to be called for non-tripod machineguns too (for mp_beach etc)
	gun->soundPos3 = ent->soundPos3;    //----(SA)
	gun->activateArc = ent->activateArc;            //----(SA)	added

	if ( !( ent->spawnflags & 2 ) ) { // no tripod
		gun->mg42BaseEnt = base->shared.s.number;
	} else {
		gun->mg42BaseEnt = -1;
	}

	gun->spawnflags = ent->spawnflags;

	SV_LinkEntity( &gun->shared );

	if ( !( ent->spawnflags & 2 ) ) { // no tripod
		base->chain = gun;
	}

	G_FreeEntity( ent );


	muzzleflashmodel = G_ModelIndex( "models/weapons2/machinegun/mg42_flash.md3" );

}

/*QUAKED misc_mg42 (1 0 0) (-16 -16 -24) (16 16 24) HIGH NOTRIPOD
harc - horizonal fire arc. Default is 115
varc - vertical fire arc. Default is 45
grabarc - activatable arc behind the gun.  if not specified, it uses the old default grabbing dynamics
health - how much damage can it take. Default is 50
damage - determines how much the weapon will inflict if a non player uses it
accuracy - all guns are 100% accurate a value of 0.5 would make it 50%
*/
void SP_mg42( GameEntity *self ) {
	const char        *damage;
	const char        *accuracy;
	float grabarc;

	if ( !self->harc ) {
		self->harc = 115;
	} else
	{
		if ( self->harc < 45 ) {
			self->harc = 45;
		}
	}

	if ( !self->varc ) {
		self->varc = 90.0;
	}

	if ( !self->health ) {
		self->health = 100;
	}

	self->think = mg42_spawn;
	self->nextthink = level.time + FRAMETIME;

	snd_noammo = G_SoundIndex( "sound/weapons/noammo.wav" );

	G_SpawnFloat( "grabarc", "0", &grabarc );   // half arc, so actually activatable over 120 deg
	self->activateArc = grabarc;


	if ( G_SpawnString( "damage", "0", &damage ) ) {
		self->damage = atoi( damage );
	}

	G_SpawnString( "accuracy", "1.0", &accuracy );

	self->accuracy = atof( accuracy );

	if ( !self->accuracy ) {
		self->accuracy = 1;
	}
}



void flak_spawn( GameEntity *ent ) {
	GameEntity *gun;
	vec3_t offset;

	gun = G_Spawn();
	gun->classname = "misc_flak";
	gun->clipmask = CONTENTS_SOLID;
	gun->shared.r.contents = CONTENTS_TRIGGER;
	gun->shared.r.svFlags = SVF_USE_CURRENT_ORIGIN;
	gun->shared.s.eType = ET_GENERAL;
	gun->touch = mg42_touch;
	gun->shared.s.modelindex = G_ModelIndex( "models/mapobjects/weapons/flak_a.md3" );
	VectorCopy( ent->shared.s.origin, offset );
	G_SetOrigin( gun, offset );
	VectorSet( gun->shared.r.mins, -24, -24, -8 );
	VectorSet( gun->shared.r.maxs, 24, 24, 48 );
	gun->shared.s.apos.trTime = 0;
	gun->shared.s.apos.trDuration = 0;
	VectorCopy( ent->shared.s.angles, gun->shared.s.angles );
	VectorCopy( gun->shared.s.angles, gun->shared.s.apos.trBase );
	VectorCopy( gun->shared.s.angles, gun->shared.s.apos.trDelta );
	gun->think = mg42_think;
	gun->nextthink = level.time + FRAMETIME;
	gun->shared.s.number = gun - g_entities;
	gun->harc = ent->harc;
	gun->varc = ent->varc;
	gun->shared.s.apos.trType = TR_LINEAR_STOP;    // interpolate the angles
	gun->takedamage = true;
	gun->targetname = ent->targetname;      // need this for scripting
	gun->mg42BaseEnt = ent->shared.s.number;

	SV_LinkEntity( &gun->shared );

}

/*QUAKED misc_flak (1 0 0) (-32 -32 0) (32 32 100)
*/
void SP_misc_flak( GameEntity *self ) {

	if ( !self->harc ) {
		self->harc = 180;
	} else
	{
		if ( self->harc < 90 ) {
			self->harc = 115;
		}
	}

	if ( !self->varc ) {
		self->varc = 90.0;
	}

	if ( !self->health ) {
		self->health = 100;
	}

	self->think = flak_spawn;
	self->nextthink = level.time + FRAMETIME;

	snd_noammo = G_SoundIndex( "sound/weapons/noammo.wav" );
}
