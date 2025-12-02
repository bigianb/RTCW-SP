/*
===========================================================================

Return to Castle Wolfenstein single player GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company. 

This file is part of the Return to Castle Wolfenstein single player GPL Source Code (RTCW SP Source Code).  

RTCW SP Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

RTCW SP Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with RTCW SP Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the RTCW SP Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the RTCW SP Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/



#include "cg_local.h"
#include "../ui/ui_shared.h"
#include "../ui/ui_local.h"
#include "../client/snd_public.h"
#include "../client/client.h"


static void CG_DrawPlayerArmorValue( rectDef_t *rect, int font, float scale, vec4_t color, qhandle_t shader, int textStyle ) {
	char num[16];
	int value;
	centity_t   *cent;
	playerState_t   *ps;

    if (cg.snap == NULL){
        return;
    }
    
	cent = &cg_entities[cg.snap->ps.clientNum];
	ps = &cg.snap->ps;

	value = ps->stats[STAT_ARMOR];


	if ( shader ) {
		RE_SetColor( color );
		CG_DrawPic( rect->x, rect->y, rect->w, rect->h, shader );
		RE_SetColor( NULL );
	} else {
		snprintf( num, sizeof( num ), "%i", value );
		value = Text_Width( num, font, scale, 0 );
		Text_Paint( rect->x + ( rect->w - value ) / 2, rect->y + rect->h, font, scale, color, num, 0, 0, textStyle );
	}
}

static int weapIconDrawSize( int weap ) {
	switch ( weap ) {

		// weapons to not draw
	case WP_KNIFE:
		return 0;

		// weapons with 'wide' icons
	case WP_THOMPSON:
	case WP_MP40:
	case WP_STEN:
	case WP_MAUSER:
	case WP_GARAND:
	case WP_VENOM:
	case WP_TESLA:
	case WP_PANZERFAUST:
	case WP_FLAMETHROWER:
	case WP_FG42:
	case WP_FG42SCOPE:
	case WP_SNOOPERSCOPE:
	case WP_SNIPERRIFLE:
		return 2;
	}

	return 1;
}


/*
==============
CG_DrawPlayerWeaponIcon
==============
*/
static void CG_DrawPlayerWeaponIcon( rectDef_t *rect, bool drawHighlighted, int align ) {
	int size;
	int realweap;                   // DHM - Nerve
	qhandle_t icon;
	float scale,halfScale;

	if ( !cg_drawIcons.integer ) {
		return;
	}

	realweap = cg.predictedPlayerState.weapon;

	size = weapIconDrawSize( realweap );

	if ( !size ) {
		return;
	}

	if ( drawHighlighted ) {
		icon = cg_weapons[ realweap ].weaponIcon[1];
	} else {
		icon = cg_weapons[ realweap ].weaponIcon[0];
	}



	// pulsing grenade icon to help the player 'count' in their head
	if ( cg.predictedPlayerState.grenadeTimeLeft ) {   // grenades and dynamite set this

		// these time differently
		if ( realweap == WP_DYNAMITE ) {
			if ( ( ( cg.grenLastTime ) % 1000 ) > ( ( cg.predictedPlayerState.grenadeTimeLeft ) % 1000 ) ) {
				S_StartLocalSound( cgs.media.grenadePulseSound4, CHAN_LOCAL_SOUND );
			}
		} else {
			if ( ( ( cg.grenLastTime ) % 1000 ) < ( ( cg.predictedPlayerState.grenadeTimeLeft ) % 1000 ) ) {
				switch ( cg.predictedPlayerState.grenadeTimeLeft / 1000 ) {
				case 3:
					S_StartLocalSound( cgs.media.grenadePulseSound4, CHAN_LOCAL_SOUND );
					break;
				case 2:
					S_StartLocalSound( cgs.media.grenadePulseSound3, CHAN_LOCAL_SOUND );
					break;
				case 1:
					S_StartLocalSound( cgs.media.grenadePulseSound2, CHAN_LOCAL_SOUND );
					break;
				case 0:
					S_StartLocalSound( cgs.media.grenadePulseSound1, CHAN_LOCAL_SOUND );
					break;
				}
			}
		}

		scale = (float)( ( cg.predictedPlayerState.grenadeTimeLeft ) % 1000 ) / 100.0f;
		halfScale = scale * 0.5f;

		cg.grenLastTime = cg.predictedPlayerState.grenadeTimeLeft;
	} else {
		scale = halfScale = 0;
	}


	if ( icon ) {
		float x, y, w, h;

		if ( size == 1 ) { // draw half width to match the icon asset

			// start at left
			x = rect->x - halfScale;
			y = rect->y - halfScale;
			w = rect->w / 2 + scale;
			h = rect->h + scale;

			switch ( align ) {
			case ITEM_ALIGN_CENTER:
				x += rect->w / 4;
				break;
			case ITEM_ALIGN_RIGHT:
				x += rect->w / 2;
				break;
			case ITEM_ALIGN_LEFT:
			default:
				break;
			}

		} else {
			x = rect->x - halfScale;
			y = rect->y - halfScale;
			w = rect->w + scale;
			h = rect->h + scale;
		}


		CG_DrawPic( x, y, w, h, icon );
	}
}


/*
==============
CG_DrawPlayerAmmoIcon
==============
*/
static void CG_DrawPlayerAmmoIcon( rectDef_t *rect, bool draw2D ) {
	centity_t   *cent;
	playerState_t   *ps;
	vec3_t angles;
	vec3_t origin;

    if (cg.snap == NULL){
        return;
    }
    
	cent = &cg_entities[cg.snap->ps.clientNum];
	ps = &cg.snap->ps;

	// TTimo: gcc: suggests () around && within ||
	if ( draw2D || ( !cg_draw3dIcons.integer && cg_drawIcons.integer ) ) {
		qhandle_t icon;
		icon = cg_weapons[ cg.predictedPlayerState.weapon ].ammoIcon;
		if ( icon ) {
			CG_DrawPic( rect->x, rect->y, rect->w, rect->h, icon );
		}
	} else if ( cg_draw3dIcons.integer ) {
		if ( cent->currentState.weapon && cg_weapons[ cent->currentState.weapon ].ammoModel ) {
			VectorClear( angles );
			origin[0] = 70;
			origin[1] = 0;
			origin[2] = 0;
			angles[YAW] = 90 + 20 * sin( cg.time / 1000.0 );
			CG_Draw3DModel( rect->x, rect->y, rect->w, rect->h, cg_weapons[ cent->currentState.weapon ].ammoModel, 0, origin, angles );
		}
	}
}

extern void CG_CheckForCursorHints( void );
#define CURSORHINT_SCALE    10

/*
==============
CG_DrawCursorHints

  cg_cursorHints.integer ==
	0:	no hints
	1:	sin size pulse
	2:	one way size pulse
	3:	alpha pulse
	4+:	static image

==============
*/
extern void CG_DrawExitStats( void );

static void CG_DrawCursorhint( rectDef_t *rect ) {
	float       *color;
	qhandle_t icon, icon2 = 0;
	float scale, halfscale;
	bool redbar = false;

	if ( !cg_cursorHints.integer ) {
		return;
	}

	CG_CheckForCursorHints();

	icon = cgs.media.hintShaders[cg.cursorHintIcon];

	switch ( cg.cursorHintIcon ) {
	case HINT_NONE:
	case HINT_FORCENONE:
		icon = 0;
		break;
	case HINT_BREAKABLE_DYNAMITE:
		icon = cgs.media.hintShaders[HINT_BREAKABLE];
		break;
	case HINT_CHAIR:
		// only show 'pickupable' if you're not armed, or are armed with a single handed weapon
		if ( cg.predictedPlayerState.weapon && !( WEAPS_ONE_HANDED & ( 1 << ( cg.predictedPlayerState.weapon ) ) ) ) { // (SA) this was backwards
			icon = cgs.media.hintShaders[HINT_NOACTIVATE];
		}
		break;

	case HINT_PLAYER:
		icon = cgs.media.hintShaders[HINT_NOACTIVATE];
		break;

	case HINT_PLYR_FRIEND:
		break;

	case HINT_NOEXIT_FAR:
		redbar = true;     // draw the status bar in red to show that you can't exit yet
	case HINT_EXIT_FAR:
		break;

	case HINT_NOEXIT:
		redbar = true;     // draw the status bar in red to show that you can't exit yet
	case HINT_EXIT:
		cg.exitStatsFade = 250;     // fade /up/ time
		if ( !cg.exitStatsTime ) {
			cg.exitStatsTime = cg.time;
		}
		CG_DrawExitStats();
		break;

	default:
		break;
	}

	if ( !icon ) {
		return;
	}


	// color
	color = CG_FadeColor( cg.cursorHintTime, cg.cursorHintFade );
	if ( !color ) {
		RE_SetColor( NULL );
		cg.exitStatsTime = 0;   // exit stats will fade up next time they're hit
		cg.cursorHintIcon = HINT_NONE;  // clear the hint
		return;
	}

	if ( cg_cursorHints.integer == 3 ) {
		color[3] *= 0.5 + 0.5 * sin( (float)cg.time / 150.0 );
	}


	// size
	if ( cg_cursorHints.integer >= 3 ) {   // no size pulsing
		scale = halfscale = 0;
	} else {
		if ( cg_cursorHints.integer == 2 ) {
			scale = (float)( ( cg.cursorHintTime ) % 1000 ) / 100.0f; // one way size pulse
		} else {
			scale = CURSORHINT_SCALE * ( 0.5 + 0.5 * sin( (float)cg.time / 150.0 ) ); // sin pulse

		}
		halfscale = scale * 0.5f;
	}

	// set color and draw the hint
	RE_SetColor( color );
	CG_DrawPic( rect->x - halfscale, rect->y - halfscale, rect->w + scale, rect->h + scale, icon );

	if ( icon2 ) {
		CG_DrawPic( rect->x - halfscale, rect->y - halfscale, rect->w + scale, rect->h + scale, icon2 );
	}

	RE_SetColor( NULL );

	// draw status bar under the cursor hint
	if ( cg.cursorHintValue && ( !( cg.time - cg.cursorHintTime ) ) ) {    // don't fade bar out w/ hint icon
		if ( redbar ) {
			Vector4Set( color, 1, 0, 0, 0.5f );
		} else {
			Vector4Set( color, 0, 0, 1, 0.5f );
		}
		CG_FilledBar( rect->x, rect->y + rect->h + 4, rect->w, 8, color, NULL, NULL, (float)cg.cursorHintValue / 255.0f, 0 );
	}

}



/*
==============
CG_DrawMessageIcon
==============
*/
//----(SA)	added

static void CG_DrawMessageIcon( rectDef_t *rect ) {
	int icon;

	if ( !cg_youGotMail.integer ) {
		return;
	}

	if ( cg_youGotMail.integer == 2 ) {
		icon = cgs.media.youGotObjectiveShader;
	} else {
		icon = cgs.media.youGotMailShader;
	}

	CG_DrawPic( rect->x, rect->y, rect->w, rect->h, icon );
}



/*
==============
CG_DrawPlayerAmmoValue
	0 - ammo
	1 - clip
==============
*/
static void CG_DrawPlayerAmmoValue( rectDef_t *rect, int font, float scale, vec4_t color, qhandle_t shader, int textStyle, int type ) {
	char num[16];
	int value, value2 = 0;
	centity_t   *cent;
	playerState_t   *ps;
	int weap;
	bool special = false;

    if (cg.snap == NULL){
        return;
    }
    
	cent = &cg_entities[cg.snap->ps.clientNum];
	ps = &cg.snap->ps;

	weap = cent->currentState.weapon;

	if ( !weap ) {
		return;
	}

	if ( ps->weaponstate == WEAPON_RELOADING && type != 0 ) {
		return;
	}

	switch ( weap ) {      // some weapons don't draw ammo count text
	case WP_KNIFE:
	case WP_CLASS_SPECIAL:              // DHM - Nerve
		return;

	case WP_AKIMBO:
		special = true;
		break;

	case WP_GRENADE_LAUNCHER:
	case WP_GRENADE_PINEAPPLE:
	case WP_DYNAMITE:
	case WP_TESLA:
	case WP_FLAMETHROWER:
		if ( type == 0 ) {  // don't draw reserve value, just clip (since these weapons have all their ammo in the clip)
			return;
		}
		break;

	default:
		break;
	}


	if ( type == 0 ) { // ammo
		value = cg.snap->ps.ammo[BG_FindAmmoForWeapon( (weapon_t)weap )];
	} else {        // clip
		value = ps->ammoclip[BG_FindClipForWeapon( (weapon_t)weap )];
		if ( special ) {
			value2 = value;
			if ( weapAlts[weap] ) {
				value = ps->ammoclip[weapAlts[weap]];
			}
		}
	}

	if ( value > -1 ) {
		if ( shader ) {
			RE_SetColor( color );
			CG_DrawPic( rect->x, rect->y, rect->w, rect->h, shader );
			RE_SetColor( NULL );
		} else {
			snprintf( num, sizeof( num ), "%i", value );
			value = Text_Width( num, font, scale, 0 );
			Text_Paint( rect->x + ( rect->w - value ) / 2, rect->y + rect->h, font, scale, color, num, 0, 0, textStyle );

//			if(special) {	// draw '0' for akimbo guns
			if ( value2 || ( special && type == 1 ) ) {
				snprintf( num, sizeof( num ), "%i /", value2 );
				value = Text_Width( num, font, scale, 0 );
				Text_Paint( -30 + rect->x + ( rect->w - value ) / 2, rect->y + rect->h, font, scale, color, num, 0, 0, textStyle );
			}
		}
	}
}

static void CG_DrawHoldableItem( rectDef_t *rect, int font, float scale, bool draw2D ) {
	int value;
	gitem_t *item;

	item    = BG_FindItemForHoldable( (holdable_t)cg.holdableSelect );

	if ( !item ) {
		return;
	}

	value   = cg.predictedPlayerState.holdable[cg.holdableSelect];

	if ( value ) {
//		CG_RegisterItemVisuals( value );
		CG_RegisterItemVisuals( item - bg_itemlist );

		if ( cg.holdableSelect == HI_WINE ) {
			if ( value > 3 ) {
				value = 3;  // 3 stages to icon, just draw full if beyond 'full'
			}
//			CG_DrawPic( rect->x, rect->y, rect->w, rect->h, cg_items[ value ].icons[2-(value-1)] );
			CG_DrawPic( rect->x, rect->y, rect->w, rect->h, cg_items[item - bg_itemlist].icons[2 - ( value - 1 )] );
		} else {
//			CG_DrawPic( rect->x, rect->y, rect->w, rect->h, cg_items[ value ].icons[0] );
			CG_DrawPic( rect->x, rect->y, rect->w, rect->h, cg_items[item - bg_itemlist].icons[0] );
		}
	}
}

static void CG_DrawPlayerHealth( rectDef_t *rect, int font, float scale, vec4_t color, qhandle_t shader, int textStyle ) {
	playerState_t   *ps;
	int value;
	char num[16];

    if (cg.snap == 0){
        return;
    }
    
	ps = &cg.snap->ps;

	value = ps->stats[STAT_HEALTH];

	if ( shader ) {
		RE_SetColor( color );
		CG_DrawPic( rect->x, rect->y, rect->w, rect->h, shader );
		RE_SetColor( NULL );
	} else {
		snprintf( num, sizeof( num ), "%i", value );
		value = Text_Width( num, font, scale, 0 );
		Text_Paint( rect->x + ( rect->w - value ) / 2, rect->y + rect->h, font, scale, color, num, 0, 0, textStyle );
	}
}


float CG_GetValue( int ownerDraw, int type ) {
	centity_t   *cent;
	clientInfo_t *ci;
	playerState_t   *ps;

	if (cg.snap == NULL){
		return 0.0f;
	}
	
	cent = &cg_entities[cg.snap->ps.clientNum];
	ps = &cg.snap->ps;

	switch ( ownerDraw ) {

	case CG_PLAYER_ARMOR_VALUE:
		return ps->stats[STAT_ARMOR];
		break;
	case CG_PLAYER_AMMO_VALUE:
		if ( cent->currentState.weapon ) {
			if ( type == RANGETYPE_RELATIVE ) {
				int weap = BG_FindAmmoForWeapon( (weapon_t)cent->currentState.weapon );
				return (float)ps->ammo[weap] / (float)ammoTable[weap].maxammo;
			} else {
				return ps->ammo[BG_FindAmmoForWeapon( (weapon_t)cent->currentState.weapon )];
			}
		}
		break;
	case CG_PLAYER_AMMOCLIP_VALUE:
		if ( cent->currentState.weapon ) {
			if ( type == RANGETYPE_RELATIVE ) {
				return (float)ps->ammoclip[BG_FindClipForWeapon( (weapon_t)cent->currentState.weapon )] / (float)ammoTable[cent->currentState.weapon].maxclip;
			} else {
				return ps->ammoclip[BG_FindClipForWeapon( (weapon_t)cent->currentState.weapon )];
			}
		}
		break;

	case CG_PLAYER_HEALTH:
		return ps->stats[STAT_HEALTH];
		break;
	case CG_PLAYER_WEAPON_STABILITY:
		return ps->aimSpreadScale;
		break;

#define BONUSTIME 60000.0f
#define SPRINTTIME 20000.0f

	case CG_STAMINA:
		if ( type == RANGETYPE_RELATIVE ) {
			return (float)cg.snap->ps.sprintTime / SPRINTTIME;
		} else {
			return cg.snap->ps.sprintTime;
		}
		break;
	default:
		break;
	}

	return -1;
}

bool CG_OwnerDrawVisible( int flags )
{
	if ( flags & CG_SHOW_NOT_V_BINOC ) {      // if looking through binocs
		if ( cg.zoomedBinoc ) {
			return false;
		}
	}

	if ( flags & ( CG_SHOW_NOT_V_BINOC | CG_SHOW_NOT_V_SNIPER | CG_SHOW_NOT_V_SNOOPER | CG_SHOW_NOT_V_FGSCOPE ) ) {
		// setting any of these does not necessarily disable drawing in regular view
		// CG_SHOW_NOT_V_CLEAR must also be set to hide for reg view
		if ( !( flags & CG_SHOW_NOT_V_CLEAR ) ) {
			return true;
		}
	}

	return false;
}


static void CG_Text_Paint_Limit( float *maxX, float x, float y, int font, float scale, vec4_t color, const char* text, float adjust, int limit ) {
	int len, count;
	vec4_t newColor;
	glyphInfo_t *glyph;
	if ( text ) {
		const char *s = text;
		float max = *maxX;
		float useScale;

		fontInfo_t *fnt = &uiInfo.uiDC.Assets.textFont;
		if ( font == UI_FONT_DEFAULT ) {
			if ( scale <= cg_smallFont.value ) {
				fnt = &uiInfo.uiDC.Assets.smallFont;
			} else if ( scale > cg_bigFont.value ) {
				fnt = &uiInfo.uiDC.Assets.bigFont;
			}
		} else if ( font == UI_FONT_BIG ) {
			fnt = &uiInfo.uiDC.Assets.bigFont;
		} else if ( font == UI_FONT_SMALL ) {
			fnt = &uiInfo.uiDC.Assets.smallFont;
		} else if ( font == UI_FONT_HANDWRITING ) {
			fnt = &uiInfo.uiDC.Assets.handwritingFont;
		}

		useScale = scale * fnt->glyphScale;
		RE_SetColor( color );
		len = strlen( text );
		if ( limit > 0 && len > limit ) {
			len = limit;
		}
		count = 0;
		while ( s && *s && count < len ) {
			glyph = &fnt->glyphs[*s];
			if ( Q_IsColorString( s ) ) {
				memcpy( newColor, g_color_table[ColorIndex( *( s + 1 ) )], sizeof( newColor ) );
				newColor[3] = color[3];
				RE_SetColor( newColor );
				s += 2;
				continue;
			} else {
				float yadj = useScale * glyph->top;
				if ( Text_Width( s, font, useScale, 1 ) + x > max ) {
					*maxX = 0;
					break;
				}
				Text_PaintChar( x, y - yadj, useScale, glyph );
				x += ( glyph->xSkip * useScale ) + adjust;
				*maxX = x;
				count++;
				s++;
			}
		}
		RE_SetColor( NULL );
	}
}


/*
==============
CG_DrawWeapStability
	draw a bar showing current stability level (0-255), max at current weapon/ability, and 'perfect' reference mark

	probably only drawn for scoped weapons
==============
*/
void CG_DrawWeapStability( rectDef_t *rect, vec4_t color, int align ) {
	vec4_t goodColor = {0, 1, 0, 0.5f}, badColor = {1, 0, 0, 0.5f};

	if ( !cg_drawSpreadScale.integer ) {
		return;
	}

	if ( cg_drawSpreadScale.integer == 1 && !( cg.weaponSelect == WP_SNOOPERSCOPE || cg.weaponSelect == WP_SNIPERRIFLE || cg.weaponSelect == WP_FG42SCOPE ) ) {
		// cg_drawSpreadScale of '1' means only draw for scoped weapons, '2' means draw all the time (for debugging)
		return;
	}

	if ( !( cg.snap->ps.aimSpreadScale ) ) {
		return;
	}

	CG_FilledBar( rect->x, rect->y, rect->w, rect->h, goodColor, badColor, NULL, (float)cg.snap->ps.aimSpreadScale / 255.0f, 2 | 4 | 256 ); // flags (BAR_CENTER|BAR_VERT|BAR_LERP_COLOR)
}


/*
==============
CG_DrawWeapHeat
==============
*/
void CG_DrawWeapHeat( rectDef_t *rect, int align ) {
	vec4_t color = {1, 0, 0, 0.2f}, color2 = {1, 0, 0, 0.5f};
	int flags = 0;

	if ( cg.snap == NULL || !( cg.snap->ps.curWeapHeat ) ) {
		return;
	}

	if ( align != HUD_HORIZONTAL ) {
		flags |= 4;   // BAR_VERT

	}
	flags |= 1;       // BAR_LEFT			- this is hardcoded now, but will be decided by the menu script
	flags |= 16;      // BAR_BG			- draw the filled contrast box
//	flags|=32;		// BAR_BGSPACING_X0Y5	- different style

	flags |= 256;     // BAR_COLOR_LERP
	CG_FilledBar( rect->x, rect->y, rect->w, rect->h, color, color2, NULL, (float)cg.snap->ps.curWeapHeat / 255.0f, flags );
}


/*
==============
CG_DrawFatigue
==============
*/

static void CG_DrawFatigue( rectDef_t *rect, vec4_t color, int align ) {
    if (cg.snap == NULL){
        return;
    }
    
	vec4_t colorBonus = {1, 1, 0, 0.45f};   // yellow (a little more solid for the 'bonus' stamina)
	float barFrac;  //, omBarFrac;
	int flags = 0;
	float chargeTime;       // DHM - Nerve

	barFrac = (float)cg.snap->ps.sprintTime / SPRINTTIME;
//	omBarFrac = 1.0f-barFrac;

	if ( align != HUD_HORIZONTAL ) {
		flags |= 4;   // BAR_VERT
		flags |= 1;   // BAR_LEFT (left, when vertical means grow 'up')
	}

	CG_FilledBar( rect->x, rect->y, rect->w, rect->h, color, NULL, NULL, (float)cg.snap->ps.sprintTime / SPRINTTIME, flags );

	// fill in the left side of the bar with the counter for the nofatigue powerup
	if ( cg.snap->ps.powerups[PW_NOFATIGUE] ) {
		CG_FilledBar( rect->x, rect->y, rect->w / 2, rect->h, colorBonus, NULL, NULL, cg.snap->ps.powerups[PW_NOFATIGUE] / BONUSTIME, flags );
	}

}


/*
==============
CG_OwnerDraw
==============
*/
void CG_OwnerDraw( float x, float y, float w, float h, float text_x, float text_y, int ownerDraw, int ownerDrawFlags, int align, float special, int font, float scale, vec4_t color, qhandle_t shader, int textStyle ) {
	rectDef_t rect;

	if ( cg_drawStatus.integer == 0 ) {
		return;
	}

	rect.x = x;
	rect.y = y;
	rect.w = w;
	rect.h = h;

	switch ( ownerDraw ) {
	case CG_PLAYER_WEAPON_ICON2D:
		CG_DrawPlayerWeaponIcon( &rect, (ownerDrawFlags & CG_SHOW_HIGHLIGHTED), align );
		break;
	case CG_PLAYER_ARMOR_VALUE:
		CG_DrawPlayerArmorValue( &rect, font, scale, color, shader, textStyle );
		break;
	case CG_PLAYER_AMMO_VALUE:
		CG_DrawPlayerAmmoValue( &rect, font, scale, color, shader, textStyle, 0 );
		break;
	case CG_CURSORHINT:
		CG_DrawCursorhint( &rect );
		break;
	case CG_NEWMESSAGE:
		CG_DrawMessageIcon( &rect );
		break;
	case CG_PLAYER_AMMOCLIP_VALUE:
		CG_DrawPlayerAmmoValue( &rect, font, scale, color, shader, textStyle, 1 );
		break;
	case CG_PLAYER_WEAPON_HEAT:
		CG_DrawWeapHeat( &rect, align );
		break;
	case CG_PLAYER_WEAPON_STABILITY:
		CG_DrawWeapStability( &rect, color, align );
		break;
	case CG_STAMINA:
		CG_DrawFatigue( &rect, color, align );
		break;
	case CG_PLAYER_HOLDABLE:
		CG_DrawHoldableItem( &rect, font, scale, (ownerDrawFlags & CG_SHOW_2DONLY)?true : false );
		break;
	case CG_PLAYER_HEALTH:
		CG_DrawPlayerHealth( &rect, font, scale, color, shader, textStyle );
		break;

	default:
		break;
	}
}

void CG_MouseEvent( int x, int y ) {
	int n;

	if ( cg.predictedPlayerState.pm_type == PM_NORMAL ) {
		Key_SetCatcher( 0 );
		return;
	}

	cgs.cursorX += x;
	if ( cgs.cursorX < 0 ) {
		cgs.cursorX = 0;
	} else if ( cgs.cursorX > 640 ) {
		cgs.cursorX = 640;
	}

	cgs.cursorY += y;
	if ( cgs.cursorY < 0 ) {
		cgs.cursorY = 0;
	} else if ( cgs.cursorY > 480 ) {
		cgs.cursorY = 480;
	}

	n = Display_CursorType( cgs.cursorX, cgs.cursorY );
	cgs.activeCursor = 0;
	if ( n == CURSOR_ARROW ) {
		cgs.activeCursor = cgs.media.selectCursor;
	} else if ( n == CURSOR_SIZER ) {
		cgs.activeCursor = cgs.media.sizeCursor;
	}

	if ( cgs.capturedItem ) {
		Display_MouseMove( cgs.capturedItem, x, y );
	} else {
		Display_MouseMove( NULL, cgs.cursorX, cgs.cursorY );
	}

}

void CG_KeyEvent( int key, bool down ) {

	if ( !down ) {
		return;
	}

	if ( cg.predictedPlayerState.pm_type == PM_NORMAL ) {
		Key_SetCatcher( 0 );
		return;
	}

	Display_HandleKey( key, down, cgs.cursorX, cgs.cursorY );

	if ( cgs.capturedItem ) {
		cgs.capturedItem = NULL;
	}   else {
		if ( key == K_MOUSE2 && down ) {
			cgs.capturedItem = Display_CaptureItem( cgs.cursorX, cgs.cursorY );
		}
	}
}

int CG_ClientNumFromName( const char *p ) {
	int i;
	for ( i = 0; i < cgs.maxclients; i++ ) {
		if ( cgs.clientinfo[i].infoValid && Q_stricmp( cgs.clientinfo[i].name, p ) == 0 ) {
			return i;
		}
	}
	return -1;
}

void CG_RunMenuScript( char **args ) {
}

