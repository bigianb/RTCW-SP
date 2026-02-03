#include "../gameEntity.h"
#include "../g_local.h"             // for spawn stuff
#include "../../server/server.h"    // SV_LinkEntity, SV_UnlinkEntity



/*QUAKED corona (0 1 0) (-4 -4 -4) (4 4 4) START_OFF
Use color picker to set color or key "color".  values are 0.0-1.0 for each color (rgb).
"scale" will designate a multiplier to the default size.  (so 2.0 is 2xdefault size, 0.5 is half)
*/

/*
==============
use_corona
	so level designers can toggle them on/off
==============
*/
void use_corona( GameEntity *ent, GameEntity *other, GameEntity *activator )
{
	if ( ent->shared.r.linked ) {
		SV_UnlinkEntity( &ent->shared );
	} else {
		ent->active = 0;
		SV_LinkEntity( &ent->shared );
	}
}


void SP_corona( GameEntity *ent )
{
	ent->shared.s.eType        = ET_CORONA;

	if ( ent->dl_color[0] <= 0 &&                // if it's black or has no color assigned
		 ent->dl_color[1] <= 0 &&
		 ent->dl_color[2] <= 0 ) {
		ent->dl_color[0] = ent->dl_color[1] = ent->dl_color[2] = 1; // set white

	}
	ent->dl_color[0] = ent->dl_color[0] * 255;
	ent->dl_color[1] = ent->dl_color[1] * 255;
	ent->dl_color[2] = ent->dl_color[2] * 255;

	ent->shared.s.dl_intensity = (int)ent->dl_color[0] | ( (int)ent->dl_color[1] << 8 ) | ( (int)ent->dl_color[2] << 16 );

    float scale;
    G_SpawnFloat( "scale", "1", &scale );
	ent->shared.s.density = (int)( scale * 255 );

	ent->use = use_corona;

	if ( !( ent->spawnflags & 1 ) ) {
		SV_LinkEntity( &ent->shared );
	}
}

