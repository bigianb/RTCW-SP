#include "dlight.h"
#include "../gameEntity.h"
#include "../g_local.h"             // for spawn stuff
#include "../../server/server.h"    // SV_LinkEntity, SV_UnlinkEntity

const char* predef_lightstyles[] = {
	"mmnmmommommnonmmonqnmmo",
	"abcdefghijklmnopqrstuvwxyzyxwvutsrqponmlkjihgfedcba",
	"mmmmmaaaaammmmmaaaaaabcdefgabcdefg",
	"ma",
	"jklmnopqrstuvwxyzyxwvutsrqponmlkj",
	"nmonqnmomnmomomono",
	"mmmaaaabcdefgmmmmaaaammmaamm",
	"aaaaaaaazzzzzzzz",
	"mmamammmmammamamaaamammma",
	"abcdefghijklmnopqrrqponmlkjihgfedcba",
	"mmnommomhkmmomnonmmonqnmmo",
	"kmamaamakmmmaakmamakmakmmmma",
	"kmmmakakmmaaamammamkmamakmmmma",
	"mmnnoonnmmmmmmmmmnmmmmnonmmmmmmm",
	"mmmmnonmmmmnmmmmmnonmmmmmnmmmmmmm",
	"zzzzzzzzaaaaaaaa",
	"zzzzzzzzaaaaaaaaaaaaaaaa",
	"aaaaaaaazzzzzzzzaaaaaaaa",
	"aaaaaaaaaaaaaaaazzzzzzzz"
};


/*
==============
dlight_finish_spawning
	All the dlights should call this on the same frame, thereby
	being synched, starting	their sequences all at the same time.
==============
*/
void dlight_finish_spawning( GameEntity *ent ) {
	G_FindConfigstringIndex( va( "%i %s %i %i %i", ent->shared.s.number, ent->dl_stylestring, ent->health, ent->soundLoop, ent->dl_atten ), CS_DLIGHTS, MAX_DLIGHT_CONFIGSTRINGS, true );
}

static int dlightstarttime = 0;


/*QUAKED dlight (0 1 0) (-12 -12 -12) (12 12 12) FORCEACTIVE STARTOFF ONETIME
"style": value is an int from 1-19 that contains a pre-defined 'flicker' string.
"stylestring": set your own 'flicker' string.  (ex. "klmnmlk"). NOTE: this should be all lowercase
Stylestring characters run at 10 cps in the game. (meaning the alphabet, at 24 characters, would take 2.4 seconds to cycle)
"offset": change the initial index in a style string.  So val of 3 in the above example would start this light at 'N'.  (used to get dlights using the same style out of sync).
"atten": offset from the alpha values of the stylestring.  stylestring of "ddeeffzz" with an atten of -1 would result in "ccddeeyy"
Use color picker to set color or key "color".  values are 0.0-1.0 for each color (rgb).
FORCEACTIVE	- toggle makes sure this light stays alive in a map even if the user has r_dynamiclight set to 0.
STARTOFF	- means the dlight doesn't spawn in until ent is triggered
ONETIME		- when the dlight is triggered, it will play through it's cycle once, then shut down until triggered again
"shader" name of shader to apply
"sound" sound to loop every cycle (this actually just plays the sound at the beginning of each cycle)

styles:
1 - "mmnmmommommnonmmonqnmmo"
2 - "abcdefghijklmnopqrstuvwxyzyxwvutsrqponmlkjihgfedcba"
3 - "mmmmmaaaaammmmmaaaaaabcdefgabcdefg"
4 - "ma"
5 - "jklmnopqrstuvwxyzyxwvutsrqponmlkj"
6 - "nmonqnmomnmomomono"
7 - "mmmaaaabcdefgmmmmaaaammmaamm"
8 - "aaaaaaaazzzzzzzz"
9 - "mmamammmmammamamaaamammma"
10 - "abcdefghijklmnopqrrqponmlkjihgfedcba"
11 - "mmnommomhkmmomnonmmonqnmmo"
12 - "kmamaamakmmmaakmamakmakmmmma"
13 - "kmmmakakmmaaamammamkmamakmmmma"
14 - "mmnnoonnmmmmmmmmmnmmmmnonmmmmmmm"
15 - "mmmmnonmmmmnmmmmmnonmmmmmnmmmmmmm"
16 - "zzzzzzzzaaaaaaaa"
17 - "zzzzzzzzaaaaaaaaaaaaaaaa"
18 - "aaaaaaaazzzzzzzzaaaaaaaa"
19 - "aaaaaaaaaaaaaaaazzzzzzzz"
*/


/*
==============
shutoff_dlight
	the dlight knew when it was triggered to unlink after going through it's cycle once
==============
*/
void shutoff_dlight( GameEntity *ent ) {
	if ( !( ent->shared.r.linked ) ) {
		return;
	}

	SV_UnlinkEntity( &ent->shared );
	ent->think = 0;
	ent->nextthink = 0;
}


void use_dlight( GameEntity *ent, GameEntity *other, GameEntity *activator ) {
	if ( ent->shared.r.linked ) {
		SV_UnlinkEntity( &ent->shared );
	} else
	{
		ent->active = 0;
		SV_LinkEntity( &ent->shared );

		if ( ent->spawnflags & 4 ) {   // ONETIME
			ent->think = shutoff_dlight;
			ent->nextthink = level.time + (  strlen( ent->dl_stylestring )  * 100 ) - 100;
		}
	}
}



/*
==============
SP_dlight
	ent->dl_stylestring contains the lightstyle string
	ent->health tracks current index into style string
	ent->count tracks length of style string
==============
*/
void SP_dlight( GameEntity *ent )
{
	const char    *snd;
	const char *shader;
	int i;
	int offset, style, atten;

	G_SpawnInt( "offset", "0", &offset );              // starting index into the stylestring
	G_SpawnInt( "style", "0", &style );                // predefined stylestring
	G_SpawnString( "sound", "", &snd );                 
	G_SpawnInt( "atten", "0", &atten );                    
	G_SpawnString( "shader", "", &shader );             // name of shader to use for this dlight image

	if ( G_SpawnString( "sound", "0", &snd ) ) {
		ent->soundLoop = G_SoundIndex( snd );
	}

	if ( ent->dl_stylestring && strlen( ent->dl_stylestring ) ) {    // if they're specified in a string, use em
	} else if ( style )       {
		style = std::max( 1, style );                                  // clamp to predefined range
		style = std::min( 19, style );
		ent->dl_stylestring = predef_lightstyles[style - 1];    // these are input as 1-20
	} else {
		ent->dl_stylestring = "mmmaaa";                          // default to a strobe to call attention to this not being set
	}

	ent->count      = strlen( ent->dl_stylestring );

	ent->dl_atten = atten;

	// make the initial offset a valid index into the stylestring
	offset = offset % ( ent->count );

	ent->health     = offset;                   // set the offset into the string

	ent->think      = dlight_finish_spawning;
	if ( !dlightstarttime ) {                      // sync up all the dlights
		dlightstarttime = level.time + 100;
	}
	ent->nextthink  = dlightstarttime;

	if ( ent->dl_color[0] <= 0 &&                // if it's black or has no color assigned, make it white
		 ent->dl_color[1] <= 0 &&
		 ent->dl_color[2] <= 0 ) {
		ent->dl_color[0] = ent->dl_color[1] = ent->dl_color[2] = 1;
	}

	ent->dl_color[0] = ent->dl_color[0] * 255;  // range 0-255 now so the client doesn't have to on every update
	ent->dl_color[1] = ent->dl_color[1] * 255;
	ent->dl_color[2] = ent->dl_color[2] * 255;

	i = (int)( ent->dl_stylestring[offset] ) - (int)'a';
	i = i * ( 1000.0f / 24.0f );

	ent->shared.s.constantLight =  (int)ent->dl_color[0] | ( (int)ent->dl_color[1] << 8 ) | ( (int)ent->dl_color[2] << 16 ) | ( i / 4 << 24 );

	ent->use = use_dlight;

	if ( !( ent->spawnflags & 2 ) ) {
		SV_LinkEntity( &ent->shared );
	}

}

