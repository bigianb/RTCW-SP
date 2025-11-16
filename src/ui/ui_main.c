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

#include "ui_local.h"
#include "../cgame/cg_local.h"
#include "../client/client.h"
#include "../qcommon/qcommon.h"


uiInfo_t uiInfo;

static int gamecodetoui[] = {4,2,3,0,5,1,6};
static int uitogamecode[] = {4,6,2,3,1,5,7};

static uiMenuCommand_t menutype = UIMENU_NONE;

// externs
extern displayContextDef_t *DC;

static int  UI_SavegamesQsortCompare( const void *arg1, const void *arg2 );

void Text_PaintCenter( float x, float y, int font, float scale, vec4_t color, const char *text, float adjust );


vmCvar_t ui_new;
vmCvar_t ui_debug;
vmCvar_t ui_initialized;
vmCvar_t ui_WolfFirstRun;

void UI_Init( void );
void UI_Shutdown( void );
void UI_KeyEvent( int key, qboolean down );
void UI_MouseEvent( int dx, int dy );

qboolean UI_IsFullscreen( void );


void AssetCache()
{
	uiInfo.uiDC.Assets.gradientBar = RE_RegisterShaderNoMip( ASSET_GRADIENTBAR );
	uiInfo.uiDC.Assets.fxBasePic = RE_RegisterShaderNoMip( ART_FX_BASE );
	uiInfo.uiDC.Assets.fxPic[0] = RE_RegisterShaderNoMip( ART_FX_RED );
	uiInfo.uiDC.Assets.fxPic[1] = RE_RegisterShaderNoMip( ART_FX_YELLOW );
	uiInfo.uiDC.Assets.fxPic[2] = RE_RegisterShaderNoMip( ART_FX_GREEN );
	uiInfo.uiDC.Assets.fxPic[3] = RE_RegisterShaderNoMip( ART_FX_TEAL );
	uiInfo.uiDC.Assets.fxPic[4] = RE_RegisterShaderNoMip( ART_FX_BLUE );
	uiInfo.uiDC.Assets.fxPic[5] = RE_RegisterShaderNoMip( ART_FX_CYAN );
	uiInfo.uiDC.Assets.fxPic[6] = RE_RegisterShaderNoMip( ART_FX_WHITE );
	uiInfo.uiDC.Assets.scrollBar = RE_RegisterShaderNoMip( ASSET_SCROLLBAR );
	uiInfo.uiDC.Assets.scrollBarArrowDown = RE_RegisterShaderNoMip( ASSET_SCROLLBAR_ARROWDOWN );
	uiInfo.uiDC.Assets.scrollBarArrowUp = RE_RegisterShaderNoMip( ASSET_SCROLLBAR_ARROWUP );
	uiInfo.uiDC.Assets.scrollBarArrowLeft = RE_RegisterShaderNoMip( ASSET_SCROLLBAR_ARROWLEFT );
	uiInfo.uiDC.Assets.scrollBarArrowRight = RE_RegisterShaderNoMip( ASSET_SCROLLBAR_ARROWRIGHT );
	uiInfo.uiDC.Assets.scrollBarThumb = RE_RegisterShaderNoMip( ASSET_SCROLL_THUMB );
	uiInfo.uiDC.Assets.sliderBar = RE_RegisterShaderNoMip( ASSET_SLIDER_BAR );
	uiInfo.uiDC.Assets.sliderThumb = RE_RegisterShaderNoMip( ASSET_SLIDER_THUMB );

	for (int n = 0; n < NUM_CROSSHAIRS; n++ ) {
		uiInfo.uiDC.Assets.crosshairShader[n] = RE_RegisterShaderNoMip( va( "gfx/2d/crosshair%c", 'a' + n ) );
	}

}

void UI_DrawSides( float x, float y, float w, float h, float size )
{
	UI_AdjustFrom640( &x, &y, &w, &h );
	size *= uiInfo.uiDC.xscale;
	trap_R_DrawStretchPic( x, y, size, h, 0, 0, 0, 0, uiInfo.uiDC.whiteShader );
	trap_R_DrawStretchPic( x + w - size, y, size, h, 0, 0, 0, 0, uiInfo.uiDC.whiteShader );
}

void UI_DrawTopBottom( float x, float y, float w, float h, float size )
{
	UI_AdjustFrom640( &x, &y, &w, &h );
	size *= uiInfo.uiDC.yscale;
	trap_R_DrawStretchPic( x, y, w, size, 0, 0, 0, 0, uiInfo.uiDC.whiteShader );
	trap_R_DrawStretchPic( x, y + h - size, w, size, 0, 0, 0, 0, uiInfo.uiDC.whiteShader );
}
/*
================
UI_DrawRect

Coordinates are 640*480 virtual values
=================
*/
void UI_DrawRect( float x, float y, float width, float height, float size, const float *color )
{
	RE_SetColor( color );

	UI_DrawTopBottom( x, y, width, height, size );
	UI_DrawSides( x, y, width, height, size );

	RE_SetColor( NULL );
}


static
fontInfo_t *getTextFont(int font, float scale)
{
    fontInfo_t *fnt = &uiInfo.uiDC.Assets.textFont;
    if ( font == UI_FONT_DEFAULT ) {
        if ( scale <= ui_smallFont.value ) {
            fnt = &uiInfo.uiDC.Assets.smallFont;
        } else if ( scale > ui_bigFont.value ) {
            fnt = &uiInfo.uiDC.Assets.bigFont;
        }
    } else if ( font == UI_FONT_BIG ) {
        fnt = &uiInfo.uiDC.Assets.bigFont;
    } else if ( font == UI_FONT_SMALL ) {
        fnt = &uiInfo.uiDC.Assets.smallFont;
    } else if ( font == UI_FONT_HANDWRITING ) {
        fnt = &uiInfo.uiDC.Assets.handwritingFont;
    }
    return fnt;
}

int Text_Width( const char *text, int font, float scale, int limit )
{
    fontInfo_t *fnt = getTextFont(font, scale);

	float useScale = scale * fnt->glyphScale;
	float out = 0;
	if ( text ) {
		size_t len = strnlen( text, 1024);
		if ( limit > 0 && len > limit ) {
			len = limit;
		}
		size_t count = 0;
        const char *s = text;
		while ( s && *s && count < len ) {
			if ( Q_IsColorString( s ) ) {
				s += 2;
				continue;
			} else {
                glyphInfo_t *glyph = &fnt->glyphs[(unsigned char)*s];
				out += glyph->xSkip;
				s++;
				count++;
			}
		}
	}
	return out * useScale;
}

int Text_Height( const char *text, int font, float scale, int limit )
{
    fontInfo_t *fnt = getTextFont(font, scale);

	float useScale = scale * fnt->glyphScale;
	float max = 0;
	if ( text ) {
        size_t len = strnlen( text, 1024);
		if ( limit > 0 && len > limit ) {
			len = limit;
		}
		int count = 0;
        const char *s = text;
		while ( s && *s && count < len ) {
			if ( Q_IsColorString( s ) ) {
				s += 2;
				continue;
			} else {
                glyphInfo_t *glyph = &fnt->glyphs[*s];
				if ( max < glyph->height ) {
					max = glyph->height;
				}
				s++;
				count++;
			}
		}
	}
	return max * useScale;
}

void Text_PaintChar( float x, float y, float scale, glyphInfo_t *glyph )
{
    float width = glyph->imageWidth;
    float height = glyph->imageHeight;
    float w = width * scale;
    float h = height * scale;
    UI_AdjustFrom640( &x, &y, &w, &h );
    trap_R_DrawStretchPic( x, y, w, h, glyph->s, glyph->t, glyph->s2, glyph->t2, glyph->glyph);
}

void Text_Paint( float x, float y, int font, float scale, vec4_t color, const char *text, float adjust, int limit, int style )
{
    if (!text){
        return;
    }
    
    fontInfo_t *fnt = getTextFont(font, scale);

	float useScale = scale * fnt->glyphScale;

    const char *s = text;
    RE_SetColor( color );
    vec4_t newColor;
    memcpy( &newColor[0], &color[0], sizeof( vec4_t ) );
    size_t len = strnlen( text, 1024);
    if ( limit > 0 && len > limit ) {
        len = limit;
    }
    int count = 0;
    while ( s && *s && count < len ) {
        
        if ( Q_IsColorString( s ) ) {
            memcpy( newColor, g_color_table[ColorIndex( *( s + 1 ) )], sizeof( newColor ) );
            newColor[3] = color[3];
            RE_SetColor( newColor );
            s += 2;
        } else {
            glyphInfo_t *glyph = &fnt->glyphs[*s];
            float yadj = useScale * glyph->top;
            if ( style == ITEM_TEXTSTYLE_SHADOWED || style == ITEM_TEXTSTYLE_SHADOWEDMORE ) {
                int ofs = style == ITEM_TEXTSTYLE_SHADOWED ? 1 : 2;
                colorBlack[3] = newColor[3];
                RE_SetColor( colorBlack );
                Text_PaintChar( x + ofs, y - yadj + ofs, useScale, glyph );
                RE_SetColor( newColor );
                colorBlack[3] = 1.0;
            }
            Text_PaintChar( x, y - yadj, useScale, glyph );

            x += ( glyph->xSkip * useScale ) + adjust;
            s++;
            count++;
        }
    }
}

void Text_PaintWithCursor( float x, float y, int font, float scale, vec4_t color, const char *text, int cursorPos, char cursor, int limit, int style )
{
    if (!text){
        return;
    }
    fontInfo_t *fnt = getTextFont(font, scale);
	float useScale = scale * fnt->glyphScale;

    const char *s = text;
    RE_SetColor( color );
    vec4_t newColor;
    memcpy( &newColor[0], &color[0], sizeof( vec4_t ) );
    size_t len = strnlen( text, 1024);
    if ( limit > 0 && len > limit ) {
        len = limit;
    }
    int count = 0;
    glyphInfo_t *cursorGlyph = &fnt->glyphs[(unsigned char)cursor];
    while ( s && *s && count < len ) {
        glyphInfo_t *glyph = &fnt->glyphs[*s];
        if ( Q_IsColorString( s ) ) {
            memcpy( newColor, g_color_table[ColorIndex( *( s + 1 ) )], sizeof( newColor ) );
            newColor[3] = color[3];
            RE_SetColor( newColor );
            s += 2;
        } else {
            float yadj = useScale * glyph->top;
            if ( style == ITEM_TEXTSTYLE_SHADOWED || style == ITEM_TEXTSTYLE_SHADOWEDMORE ) {
                int ofs = style == ITEM_TEXTSTYLE_SHADOWED ? 1 : 2;
                colorBlack[3] = newColor[3];
                RE_SetColor( colorBlack );
                Text_PaintChar( x + ofs, y - yadj + ofs, useScale, glyph );
                colorBlack[3] = 1.0;
                RE_SetColor( newColor );
            }
            Text_PaintChar( x, y - yadj, useScale, glyph );
            
            if ( count == cursorPos && !( ( uiInfo.uiDC.realTime / BLINK_DIVISOR ) & 1 ) ) {
                yadj = useScale * cursorGlyph->top;
                Text_PaintChar( x, y - yadj, useScale, cursorGlyph );
            }

            x += ( glyph->xSkip * useScale );
            s++;
            count++;
        }
    }
    
    // need to paint cursor at end of text
    if ( cursorPos == len && !( ( uiInfo.uiDC.realTime / BLINK_DIVISOR ) & 1 ) ) {
        float yadj = useScale * cursorGlyph->top;
        Text_PaintChar( x, y - yadj, useScale, cursorGlyph );
    }
	
}


void UI_DrawCenteredPic( qhandle_t image, int w, int h )
{
	int x = ( SCREEN_WIDTH - w ) / 2;
	int y = ( SCREEN_HEIGHT - h ) / 2;
	UI_DrawHandlePic( x, y, w, h, image );
}

/*
=================
UI_Shutdown
=================
*/
void UI_Shutdown( void ) {

}

char *defaultMenu = NULL;

static
char *GetMenuBuffer( const char *filename )
{
	fileHandle_t f;
	static char buf[MAX_MENUFILE];

	int len = FS_FOpenFileByMode( filename, &f, FS_READ );
	if ( !f ) {
        Com_Printf( S_COLOR_RED "menu file not found: %s, using default\n", filename  );
		return defaultMenu;
	}
	if ( len >= MAX_MENUFILE ) {
        Com_Printf( S_COLOR_RED "menu file too large: %s is %i, max allowed is %i", filename, len, MAX_MENUFILE  );
		FS_FCloseFile( f );
		return defaultMenu;
	}

	FS_Read( buf, len, f );
	buf[len] = 0;
	FS_FCloseFile( f );

	return buf;
}

qboolean Asset_Parse( int handle ) {
	pc_token_t token;
	const char *tempStr;

	if ( !PC_ReadTokenHandle( handle, &token ) ) {
		return qfalse;
	}
	if ( Q_stricmp( token.string, "{" ) != 0 ) {
		return qfalse;
	}

	while ( 1 ) {

		memset( &token, 0, sizeof( pc_token_t ) );

		if ( !PC_ReadTokenHandle( handle, &token ) ) {
			return qfalse;
		}

		if ( Q_stricmp( token.string, "}" ) == 0 ) {
			return qtrue;
		}

		// font
		if ( Q_stricmp( token.string, "font" ) == 0 ) {
			int pointSize;
			if ( !PC_String_Parse( handle, &tempStr ) || !PC_Int_Parse( handle,&pointSize ) ) {
				return qfalse;
			}
			trap_R_RegisterFont( tempStr, pointSize, &uiInfo.uiDC.Assets.textFont );
			uiInfo.uiDC.Assets.fontRegistered = qtrue;
			continue;
		}

		if ( Q_stricmp( token.string, "smallFont" ) == 0 ) {
			int pointSize;
			if ( !PC_String_Parse( handle, &tempStr ) || !PC_Int_Parse( handle,&pointSize ) ) {
				return qfalse;
			}
			trap_R_RegisterFont( tempStr, pointSize, &uiInfo.uiDC.Assets.smallFont );
			continue;
		}

		if ( Q_stricmp( token.string, "bigFont" ) == 0 ) {
			int pointSize;
			if ( !PC_String_Parse( handle, &tempStr ) || !PC_Int_Parse( handle,&pointSize ) ) {
				return qfalse;
			}
			trap_R_RegisterFont( tempStr, pointSize, &uiInfo.uiDC.Assets.bigFont );
			continue;
		}

		// handwriting
		if ( Q_stricmp( token.string, "handwritingFont" ) == 0 ) {
			int pointSize;
			if ( !PC_String_Parse( handle, &tempStr ) || !PC_Int_Parse( handle,&pointSize ) ) {
				return qfalse;
			}
			trap_R_RegisterFont( tempStr, pointSize, &uiInfo.uiDC.Assets.handwritingFont );
			continue;
		}


		// gradientbar
		if ( Q_stricmp( token.string, "gradientbar" ) == 0 ) {
			if ( !PC_String_Parse( handle, &tempStr ) ) {
				return qfalse;
			}
			uiInfo.uiDC.Assets.gradientBar = RE_RegisterShaderNoMip( tempStr );
			continue;
		}

		// enterMenuSound
		if ( Q_stricmp( token.string, "menuEnterSound" ) == 0 ) {
			if ( !PC_String_Parse( handle, &tempStr ) ) {
				return qfalse;
			}
			uiInfo.uiDC.Assets.menuEnterSound = S_RegisterSound( tempStr );
			continue;
		}

		// exitMenuSound
		if ( Q_stricmp( token.string, "menuExitSound" ) == 0 ) {
			if ( !PC_String_Parse( handle, &tempStr ) ) {
				return qfalse;
			}
			uiInfo.uiDC.Assets.menuExitSound = S_RegisterSound( tempStr );
			continue;
		}

		// itemFocusSound
		if ( Q_stricmp( token.string, "itemFocusSound" ) == 0 ) {
			if ( !PC_String_Parse( handle, &tempStr ) ) {
				return qfalse;
			}
			uiInfo.uiDC.Assets.itemFocusSound = S_RegisterSound( tempStr );
			continue;
		}

		// menuBuzzSound
		if ( Q_stricmp( token.string, "menuBuzzSound" ) == 0 ) {
			if ( !PC_String_Parse( handle, &tempStr ) ) {
				return qfalse;
			}
			uiInfo.uiDC.Assets.menuBuzzSound = S_RegisterSound( tempStr );
			continue;
		}

		if ( Q_stricmp( token.string, "cursor" ) == 0 ) {
			if ( !PC_String_Parse( handle, &uiInfo.uiDC.Assets.cursorStr ) ) {
				return qfalse;
			}
			uiInfo.uiDC.Assets.cursor = RE_RegisterShaderNoMip( uiInfo.uiDC.Assets.cursorStr );
			continue;
		}

		if ( Q_stricmp( token.string, "fadeClamp" ) == 0 ) {
			if ( !PC_Float_Parse( handle, &uiInfo.uiDC.Assets.fadeClamp ) ) {
				return qfalse;
			}
			continue;
		}

		if ( Q_stricmp( token.string, "fadeCycle" ) == 0 ) {
			if ( !PC_Int_Parse( handle, &uiInfo.uiDC.Assets.fadeCycle ) ) {
				return qfalse;
			}
			continue;
		}

		if ( Q_stricmp( token.string, "fadeAmount" ) == 0 ) {
			if ( !PC_Float_Parse( handle, &uiInfo.uiDC.Assets.fadeAmount ) ) {
				return qfalse;
			}
			continue;
		}

		if ( Q_stricmp( token.string, "shadowX" ) == 0 ) {
			if ( !PC_Float_Parse( handle, &uiInfo.uiDC.Assets.shadowX ) ) {
				return qfalse;
			}
			continue;
		}

		if ( Q_stricmp( token.string, "shadowY" ) == 0 ) {
			if ( !PC_Float_Parse( handle, &uiInfo.uiDC.Assets.shadowY ) ) {
				return qfalse;
			}
			continue;
		}

		if ( Q_stricmp( token.string, "shadowColor" ) == 0 ) {
			if ( !PC_Color_Parse( handle, &uiInfo.uiDC.Assets.shadowColor ) ) {
				return qfalse;
			}
			uiInfo.uiDC.Assets.shadowFadeClamp = uiInfo.uiDC.Assets.shadowColor[3];
			continue;
		}

	}
	return qfalse;
}


void UI_ParseMenu( const char *menuFile, qboolean isHud  )
{
	Com_Printf( "Parsing menu file:%s\n", menuFile );

	int handle = trap_PC_LoadSource( menuFile );
	if ( !handle ) {
		return;
	}

	while ( 1 ) {
        pc_token_t token;
		memset( &token, 0, sizeof( pc_token_t ) );
		if ( !PC_ReadTokenHandle( handle, &token ) ) {
			break;
		}

		if ( token.string[0] == '}' ) {
			break;
		}

		if ( Q_stricmp( token.string, "assetGlobalDef" ) == 0 ) {
			if ( Asset_Parse( handle ) ) {
				continue;
			} else {
				break;
			}
		}

		if ( Q_stricmp( token.string, "menudef" ) == 0 ) {
			// start a new menu
			Menu_New( handle, isHud );
		}
	}
	trap_PC_FreeSource( handle );
}

qboolean Load_Menu( int handle, qboolean isHud )
{
	pc_token_t token;

	if ( !PC_ReadTokenHandle( handle, &token ) ) {
		return qfalse;
	}
	if ( token.string[0] != '{' ) {
		return qfalse;
	}

	while ( PC_ReadTokenHandle( handle, &token ) && token.string[0] != 0) {
		if ( token.string[0] == '}' ) {
			return qtrue;
		}
		UI_ParseMenu( token.string, isHud );
	}
	return qfalse;
}

void LoadMenus( const char *menuFile, qboolean reset, qboolean isHud )
{
	int start = Sys_Milliseconds();
	int handle = trap_PC_LoadSource( menuFile );
	if ( !handle ) {
        Com_Error( ERR_DROP, S_COLOR_YELLOW "menu file not found: %s, using default\n", menuFile );
		if ( !handle ) {
            Com_Error( ERR_DROP, S_COLOR_RED "default menu file not found: ui/menus.txt, unable to continue!\n", menuFile );
		}
	}

	ui_new.integer = 1;

	if ( reset ) {
		Menu_Reset(isHud);
	}

    pc_token_t token;
	while ( PC_ReadTokenHandle( handle, &token ) ) {
        
		if ( token.string[0] == 0 || token.string[0] == '}' ) {
			break;
		}

		if ( Q_stricmp( token.string, "loadmenu" ) == 0 ) {
			if ( !Load_Menu( handle, isHud ) ) {
				break;
			}
		}
	}

	Com_Printf( "UI menu load time = %d milli seconds\n", Sys_Milliseconds() - start );

	trap_PC_FreeSource( handle );
}

void UI_LoadMenus( const char *menuFile, qboolean reset ) {
	LoadMenus(menuFile, reset, qfalse);
}

/*
==============
UI_LoadTranslationStrings
==============
*/
#define MAX_BUFFER          20000
static void UI_LoadTranslationStrings( void )
{
	char filename[MAX_QPATH];
	fileHandle_t f;

	snprintf( filename, MAX_QPATH, "text/strings.txt" );
	int len = FS_FOpenFileByMode( filename, &f, FS_READ );
	if ( len <= 0 ) {
		Com_Printf( S_COLOR_RED "WARNING: string translation file (strings.txt not found in main/text)\n" );
		return;
	}
	if ( len >= MAX_BUFFER ) {
		Com_Error( ERR_DROP, "%s is too big, make it smaller (max = %i bytes)\n", filename, MAX_BUFFER );
        return;
	}

	// load the file into memory
    char buffer[MAX_BUFFER];
	FS_Read( buffer, len, f );
	buffer[len] = 0;
	FS_FCloseFile( f );
	// parse the list
	char* text = buffer;

	int numStrings = sizeof( translateStrings ) / sizeof( translateStrings[0] ) - 1;

	for (int i = 0; i < numStrings; i++ ) {
		char* token = COM_ParseExt( &text, qtrue );
		if ( !token[0] ) {
			break;
		}
		translateStrings[i].localname = malloc( strnlen( token, 1024 ) + 1 );
		strcpy( translateStrings[i].localname, token );
	}
}


void UI_Load() {
	char lastName[1024];
	menuDef_t *menu = Menu_GetFocused();
	char *menuSet = UI_Cvar_VariableString( "ui_menuFiles" );
	if ( menu && menu->window.name ) {
		strcpy( lastName, menu->window.name );
	}
	if ( menuSet == NULL || menuSet[0] == '\0' ) {
		menuSet = "ui/menus.txt";
	}

	String_Init();

	// load translation text
	UI_LoadTranslationStrings();

	UI_LoadMenus( menuSet, qtrue );
	Menus_CloseAll();
	Menus_ActivateByName( lastName );
}

//----(SA)	added
/*
==============
UI_DrawSavegameName
==============
*/
static void UI_DrawSavegameName( rectDef_t *rect, int font, float scale, vec4_t color, int textStyle ) {
	Text_PaintCenter( rect->x, rect->y, font, scale, color, ui_savegameName.string, textStyle );
}



/*
==============
UI_SavegameIndexFromName
	sorted index in feeder
==============
*/
static int UI_SavegameIndexFromName( const char *name )
{
	if ( name && *name ) {
		for (int i = 0; i < uiInfo.savegameCount; i++ ) {
			if ( Q_stricmp( name, uiInfo.savegameList[uiInfo.savegameStatus.displaySavegames[i]].savegameName ) == 0 ) {
				return i;
			}
		}
	}
	return 0;
}

/*
==============
UI_SavegameIndexFromName2
	return the index in t
==============
*/
static int UI_SavegameIndexFromName2( const char *name )
{
	int index = UI_SavegameIndexFromName( name );
	return uiInfo.savegameStatus.displaySavegames[index];
}

/*
==============
UI_DrawSaveGameShot
==============
*/
static void UI_DrawSaveGameShot( rectDef_t *rect, float scale, vec4_t color )
{
	qhandle_t image;

	RE_SetColor( color );

	if ( !strnlen( ui_savegameName.string, 256 ) || ui_savegameName.string[0] == '0' ) {
		image = RE_RegisterShaderNoMip( "menu/art/unknownmap" );
	} else {
		int i = UI_SavegameIndexFromName2( ui_savegameName.string );

		if ( uiInfo.savegameList[i].sshotImage == -1 ) {
			uiInfo.savegameList[i].sshotImage = RE_RegisterShaderNoMip( uiInfo.savegameList[i].imageName );
		}

		image = uiInfo.savegameList[i].sshotImage;
	}

	UI_DrawHandlePic( rect->x, rect->y, rect->w, rect->h, image );
	RE_SetColor( NULL );

}


static void UI_DrawPregameCinematic( rectDef_t *rect, float scale, vec4_t color ) {
	if ( uiInfo.previewMovie > -2 ) {
		uiInfo.previewMovie = trap_CIN_PlayCinematic( va( "%s.roq", "assault" ), 0, 0, 0, 0, ( CIN_loop | CIN_silent | CIN_system ) );
		if ( uiInfo.previewMovie >= 0 ) {
			trap_CIN_RunCinematic( uiInfo.previewMovie );
			CIN_SetExtents( uiInfo.previewMovie, rect->x, rect->y, rect->w, rect->h );
			trap_CIN_DrawCinematic( uiInfo.previewMovie );
		} else {
			uiInfo.previewMovie = -2;
		}
	}

}

static void UI_DrawPreviewCinematic( rectDef_t *rect, float scale, vec4_t color ) {
	if ( uiInfo.previewMovie > -2 ) {
		uiInfo.previewMovie = trap_CIN_PlayCinematic( va( "%s.roq", uiInfo.movieList[uiInfo.movieIndex] ), 0, 0, 0, 0, ( CIN_loop | CIN_silent ) );
		if ( uiInfo.previewMovie >= 0 ) {
			trap_CIN_RunCinematic( uiInfo.previewMovie );
			CIN_SetExtents( uiInfo.previewMovie, rect->x, rect->y, rect->w, rect->h );
			trap_CIN_DrawCinematic( uiInfo.previewMovie );
		} else {
			uiInfo.previewMovie = -2;
		}
	}

}


//----(SA)	added
/*
==============
UI_DrawMapLevelshot
	Draws the levelshot for the current map.
==============
*/
static void UI_DrawMapLevelshot( rectDef_t *rect ) {
	char levelname[64];
	qhandle_t levelshot = 0;

	Cvar_VariableStringBuffer( "mapname", levelname, sizeof( levelname ) );

	if ( levelname[0] != 0 ) {
		levelshot = RE_RegisterShaderNoMip( va( "levelshots/%s.tga", levelname ) );
	}

	if ( !levelshot ) {
		levelshot = RE_RegisterShaderNoMip( "menu/art/unknownmap" );
	}

	UI_DrawHandlePic( rect->x, rect->y, rect->w, rect->h, levelshot );
}

/*
==============
CG_HorizontalPercentBar
	Generic routine for pretty much all status indicators that show a fractional
	value to the palyer by virtue of how full a drawn box is.

flags:
	left		- 1
	center		- 2		// direction is 'right' by default and orientation is 'horizontal'
	vert		- 4
	nohudalpha	- 8		// don't adjust bar's alpha value by the cg_hudalpha value
	bg			- 16	// background contrast box (bg set with bgColor of 'NULL' means use default bg color (1,1,1,0.25)
	spacing		- 32	// some bars use different sorts of spacing when drawing both an inner and outer box

	lerp color	- 256	// use an average of the start and end colors to set the fill color
==============
*/


// TODO: these flags will be shared, but it was easier to work on stuff if I wasn't changing header files a lot
#define BAR_LEFT        0x0001
#define BAR_CENTER      0x0002
#define BAR_VERT        0x0004
#define BAR_NOHUDALPHA  0x0008
#define BAR_BG          0x0010
// different spacing modes for use w/ BAR_BG
#define BAR_BGSPACING_X0Y5  0x0020
#define BAR_BGSPACING_X0Y0  0x0040

#define BAR_LERP_COLOR  0x0100

#define BAR_BORDERSIZE 2

void UI_FilledBar( float x, float y, float w, float h, float *startColor, float *endColor, const float *bgColor, float frac, int flags ) {
	vec4_t backgroundcolor = {1, 1, 1, 0.25f}, colorAtPos;  // colorAtPos is the lerped color if necessary
	int indent = BAR_BORDERSIZE;

	if ( ( flags & BAR_BG ) && bgColor ) { // BAR_BG set, and color specified, use specified bg color
		Vector4Copy( bgColor, backgroundcolor );
	}

	// hud alpha
	if ( !( flags & BAR_NOHUDALPHA ) ) {
		startColor[3] *= ui_hudAlpha.value;
		if ( endColor ) {
			endColor[3] *= ui_hudAlpha.value;
		}

		backgroundcolor[3] *= ui_hudAlpha.value;
	}

	if ( flags & BAR_LERP_COLOR ) {
		Vector4Average( startColor, endColor, frac, colorAtPos );
	}

	// background
	if ( ( flags & BAR_BG ) ) {
		// draw background at full size and shrink the remaining box to fit inside with a border.  (alternate border may be specified by a BAR_BGSPACING_xx)
		UI_FillRect(   x,
					   y,
					   w,
					   h,
					   backgroundcolor );

		if ( flags & BAR_BGSPACING_X0Y0 ) {          // fill the whole box (no border)

		} else if ( flags & BAR_BGSPACING_X0Y5 ) {   // spacing created for weapon heat
			indent *= 3;
			y += indent;
			h -= ( 2 * indent );

		} else {                                // default spacing of 2 units on each side
			x += indent;
			y += indent;
			w -= ( 2 * indent );
			h -= ( 2 * indent );
		}
	}


	// adjust for horiz/vertical and draw the fractional box
	if ( flags & BAR_VERT ) {
		if ( flags & BAR_LEFT ) {    // TODO: remember to swap colors on the ends here
			y += ( h * ( 1 - frac ) );
		} else if ( flags & BAR_CENTER ) {
			y += ( h * ( 1 - frac ) / 2 );
		}

		if ( flags & BAR_LERP_COLOR ) {
			UI_FillRect( x, y, w, h * frac, colorAtPos );
		} else {
			UI_FillRect( x, y, w, h * frac, startColor );
		}

	} else {

		if ( flags & BAR_LEFT ) {    // TODO: remember to swap colors on the ends here
			x += ( w * ( 1 - frac ) );
		} else if ( flags & BAR_CENTER ) {
			x += ( w * ( 1 - frac ) / 2 );
		}

		if ( flags & BAR_LERP_COLOR ) {
			UI_FillRect( x, y, w * frac, h, colorAtPos );
		} else {
			UI_FillRect( x, y, w * frac, h, startColor );
		}
	}

}



/*
==============
UI_DrawLoadStatus
==============
*/
static void UI_DrawLoadStatus( rectDef_t *rect, vec4_t color, int align )
{
    
    int flags = 0;
    
    if ( align != HUD_HORIZONTAL ) {
        flags |= 4;   // BAR_VERT
        //		flags|=1;	// BAR_LEFT (left, when vertical means grow 'up')
    }
    
    flags |= 16;      // BAR_BG			- draw the filled contrast box
    
    char hunkBuf[MAX_QPATH];
    Cvar_VariableStringBuffer( "com_expectedhunkusage", hunkBuf, MAX_QPATH );
    int expectedHunk = atoi( hunkBuf );
    
    if ( expectedHunk > 0 ) {
        float percentDone = (float)( ui_hunkUsed.integer ) / (float)( expectedHunk );
        if ( percentDone > 0.97 ) { // never actually show 100%, since we are not in the game yet
            percentDone = 0.97;
        }
        
        UI_FilledBar( rect->x, rect->y, rect->w, rect->h, color, NULL, NULL, percentDone, flags );
    } else {
        Text_Paint( rect->x, rect->y, UI_FONT_DEFAULT, 0.2f, color, DC->getTranslatedString( "pleasewait" ), 0, 0, 0 );
    }
}

static int UI_OwnerDrawWidth( int ownerDraw, int font, float scale )
{
	const char *s = NULL;
	switch ( ownerDraw ) {

	case UI_SAVEGAMENAME:
		s = ui_savegameName.string;
		break;

	case UI_SAVEGAMEINFO:
        {
            int i = UI_SavegameIndexFromName2( ui_savegameName.string );
            s = uiInfo.savegameList[i].savegameInfoText;
        }
		break;

	case UI_KEYBINDSTATUS:
		if ( Display_KeyBindPending() ) {
			s = DC->getTranslatedString( "keywait" );
		} else {
			s = DC->getTranslatedString( "keychange" );
		}
		break;
	default:
		break;
	}

	if ( s ) {
		return Text_Width( s, font, scale, 0 );
	}
	return 0;
}


static void UI_DrawCrosshair( rectDef_t *rect, float scale, vec4_t color )
{
	int ch = ( uiInfo.currentCrosshair % NUM_CROSSHAIRS );

	if ( !ch ) {
		return;
	}

	RE_SetColor( color );
	UI_DrawHandlePic( rect->x, rect->y - rect->h, rect->w, rect->h, uiInfo.uiDC.Assets.crosshairShader[ch] );
	RE_SetColor( NULL );
}


static void UI_DrawKeyBindStatus( rectDef_t *rect, int font, float scale, vec4_t color, int textStyle )
{
	if ( Display_KeyBindPending() ) {
		Text_Paint( rect->x, rect->y, font, scale, color, DC->getTranslatedString( "keywait" ), 0, 0, textStyle );
	} else {
		Text_Paint( rect->x, rect->y, font, scale, color, DC->getTranslatedString( "keychange" ), 0, 0, textStyle );
	}
}

static void UI_DrawGLInfo( rectDef_t *rect, int font, float scale, vec4_t color, int textStyle )
{
	const char *lines[64];

	Text_Paint( rect->x + 2, rect->y, font, scale, color, va( "VENDOR: %s", uiInfo.uiDC.glconfig.vendor_string ), 0, 30, textStyle );
	Text_Paint( rect->x + 2, rect->y + 15, font, scale, color, va( "VERSION: %s: %s", uiInfo.uiDC.glconfig.version_string,uiInfo.uiDC.glconfig.renderer_string ), 0, 30, textStyle );
	Text_Paint( rect->x + 2, rect->y + 60, font, scale, color, va( "PIXELFORMAT: color(%d-bits) Z(%d-bits) stencil(%d-bits)", uiInfo.uiDC.glconfig.colorBits, uiInfo.uiDC.glconfig.depthBits, uiInfo.uiDC.glconfig.stencilBits ), 0, 30, textStyle );

	// build null terminated extension strings
    char buff[4096];
	Q_strncpyz( buff, uiInfo.uiDC.glconfig.extensions_string, 4096 );
	char* eptr = buff;
	int y = rect->y + 45;
	int numLines = 0;
	while ( y < rect->y + rect->h && *eptr )
	{
        while (*eptr == ' ' ){
            *eptr++ = '\0';
        }
		// track start of valid string
		if ( *eptr ) {
			lines[numLines++] = eptr;
		}

		while ( *eptr && *eptr != ' ' )
			eptr++;
	}

	int i = 0;
	while ( i < numLines ) {
		Text_Paint( rect->x + 2, y, font, scale, color, lines[i++], 0, 36, textStyle );
		if ( i < numLines ) {
			Text_Paint( rect->x + rect->w / 3.0f, y, font, scale, color, lines[i++], 0, 36, textStyle );
		}
		if ( i < numLines ) {
			Text_Paint( rect->x + ( 2.0f * ( rect->w / 3.0f ) ), y, font, scale, color, lines[i++], 0, 36, textStyle );
		}
		y += 10;
		if ( y > rect->y + rect->h - 11 ) {
			break;
		}
	}
}

void UI_OwnerDraw( float x, float y, float w, float h, float text_x, float text_y, int ownerDraw, int ownerDrawFlags, int align, float special, int font, float scale, vec4_t color, qhandle_t shader, int textStyle )
{
	rectDef_t rect;

	rect.x = x + text_x;
	rect.y = y + text_y;
	rect.w = w;
	rect.h = h;

	switch ( ownerDraw ) {

	case UI_SAVEGAME_SHOT:
		UI_DrawSaveGameShot( &rect, scale, color );
		break;
	case UI_STARTMAPCINEMATIC:
		UI_DrawPregameCinematic( &rect, scale, color );
		break;


	case UI_SAVEGAMENAME:
		UI_DrawSavegameName( &rect, font, scale, color, textStyle );
		break;
	case UI_SAVEGAMEINFO:
		break;
	case UI_LEVELSHOT:
		UI_DrawMapLevelshot( &rect );
		break;
	case UI_LOADSTATUSBAR:
		UI_DrawLoadStatus( &rect, color, align );
		break;

	case UI_CROSSHAIR:
		UI_DrawCrosshair( &rect, scale, color );
		break;
	case UI_GLINFO:
		UI_DrawGLInfo( &rect, font, scale, color, textStyle );
		break;
	case UI_KEYBINDSTATUS:
		UI_DrawKeyBindStatus( &rect, font, scale, color, textStyle );
		break;
	default:
        CG_OwnerDraw(x, y, w, h,  text_x,  text_y,  ownerDraw,  ownerDrawFlags,  align,  special,  font,  scale,  color,  shader,  textStyle);
		break;
	}
}

static qboolean UI_OwnerDrawVisible( int flags )
{
	return qtrue;
}

static qboolean UI_SavegameName_HandleKey( int flags, float *special, int key )
{
	if ( key == K_MOUSE1 || key == K_MOUSE2 || key == K_ENTER || key == K_KP_ENTER ) {
		int i = UI_SavegameIndexFromName( ui_savegameName.string );

		if ( key == K_MOUSE2 ) {
			i--;
		} else {
			i++;
		}
		if ( i >= uiInfo.savegameCount ) {
			i = 0;
		} else if ( i < 0 ) {
			i = uiInfo.savegameCount - 1;
		}

		// set feeder highlight
		Menu_SetFeederSelection( NULL, FEEDER_SAVEGAMES, i, NULL );

		Cvar_Set( "ui_savegameName", uiInfo.savegameList[uiInfo.savegameStatus.displaySavegames[i]].savegameName );
		Cvar_Set( "ui_savegameInfo", uiInfo.savegameList[uiInfo.savegameStatus.displaySavegames[i]].savegameInfoText );
		return qtrue;
	}
	return qfalse;
}


static qboolean UI_Crosshair_HandleKey( int flags, float *special, int key )
{
	if ( key == K_MOUSE1 || key == K_MOUSE2 || key == K_ENTER || key == K_KP_ENTER ) {
		if ( key == K_MOUSE2 ) {
			uiInfo.currentCrosshair--;
		} else {
			uiInfo.currentCrosshair++;
		}

		if ( uiInfo.currentCrosshair >= NUM_CROSSHAIRS ) {
			uiInfo.currentCrosshair = 0;
		} else if ( uiInfo.currentCrosshair < 0 ) {
			uiInfo.currentCrosshair = NUM_CROSSHAIRS - 1;
		}
		Cvar_Set( "cg_drawCrosshair", va( "%d", uiInfo.currentCrosshair ) );
		return qtrue;
	}
	return qfalse;
}


qboolean UI_OwnerDrawHandleKey( int ownerDraw, int flags, float *special, int key )
{
	switch ( ownerDraw ) {

	case UI_SAVEGAMENAME:
	case UI_SAVEGAMEINFO:
		return UI_SavegameName_HandleKey( flags, special, key );
	
	case UI_CROSSHAIR:
		UI_Crosshair_HandleKey( flags, special, key );
		break;
	
	default:
		break;
	}

	return qfalse;
}


static float UI_GetValue( int ownerDraw, int type ) {
	return 0;
}


/*
==============
UI_SavegamesQsortCompare
==============
*/
static int  UI_SavegamesQsortCompare( const void *arg1, const void *arg2 )
{
	int *ea = (int *)arg1;
	int *eb = (int *)arg2;

	if ( *ea == *eb ) {
		return 0;
	}

    savegameInfo *sg = &uiInfo.savegameList[*eb];
    savegameInfo *sg2 = &uiInfo.savegameList[*ea];

    int ret = 0;
	if ( uiInfo.savegameStatus.sortKey == SORT_SAVENAME ) {
		ret = Q_stricmp( &sg->savegameName[0], &sg2->savegameName[0] );

	} else if ( uiInfo.savegameStatus.sortKey == SORT_SAVETIME ) {

// (SA) better way to do this?  (i was adding up seconds, but that seems slower than a bunch of comparisons)
		int i = sg->tm.tm_year;
		int j = sg2->tm.tm_year;
		if ( i < j ) {
			ret = -1;
		} else if ( i > j ) {
			ret = 1;
		} else {
			i = sg->tm.tm_yday;
			j = sg2->tm.tm_yday;
			if ( i < j ) {
				ret = -1;
			} else if ( i > j ) {
				ret = 1;
			} else {
				i = sg->tm.tm_hour;
				j = sg2->tm.tm_hour;
				if ( i < j ) {
					ret = -1;
				} else if ( i > j ) {
					ret = 1;
				} else {
					i = sg->tm.tm_min;
					j = sg2->tm.tm_min;
					if ( i < j ) {
						ret = -1;
					} else if ( i > j ) {
						ret = 1;
					} else {
						i = sg->tm.tm_sec;
						j = sg2->tm.tm_sec;
						if ( i < j ) {
							ret = -1;
						} else if ( i > j ) {
							ret = 1;
						} else { ret = 0;}
					}
				}
			}
		}
	} else {
		ret = 0;
	}

	if ( uiInfo.savegameStatus.sortDir ) {
		return ret;
	} else {
		return -ret;
	}
}

static void UI_FeederSelection( float feederID, int index );
void UI_SavegameSort( int column, qboolean force )
{
	if ( !force ) {
		if ( uiInfo.savegameStatus.sortKey == column ) {
			return;
		}
	}
	uiInfo.savegameStatus.sortKey = column;

	if ( uiInfo.savegameCount ) {
		qsort( &uiInfo.savegameStatus.displaySavegames[0], uiInfo.savegameCount, sizeof( int ), UI_SavegamesQsortCompare );

		// re-select the one that was selected before sorting
		int cursel = UI_SavegameIndexFromName( ui_savegameName.string );
        UI_FeederSelection( FEEDER_SAVEGAMES, cursel );
		Menu_SetFeederSelection( NULL, FEEDER_SAVEGAMES, cursel, NULL );

		// and clear out the text entry
		Cvar_Set( "ui_savegame", "" );

	} else {
		Cvar_Set( "ui_savegameName", "" );
		Cvar_Set( "ui_savegameInfo", "(no savegames)" );
	}

}

/*
==============
UI_DelSavegame
==============
*/
static void UI_DelSavegame()
{
	int i = UI_SavegameIndexFromName2( ui_savegameName.string );

	int ret = FS_Delete( va( "save/%s.svg", uiInfo.savegameList[i].savegameFile ) );

	if ( ret ) {
		Com_Printf( "Deleted savegame: %s.svg\n", uiInfo.savegameList[i].savegameName );
	} else {
		Com_Printf( "Unable to delete savegame: %s.svg\n", uiInfo.savegameList[i].savegameName );
	}

	UI_SavegameSort( uiInfo.savegameStatus.sortKey, qtrue );  // re-sort
}


#define SAVE_INFOSTRING_LENGTH  256     // defined in g_save.c


/*
==============
UI_ParseSavegame
==============
*/

static char *monthStr[12] =
{
	"JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"
};

/*
==============
UI_ParseSavegame
==============
*/
void UI_ParseSavegame( int index )
{
    fileHandle_t f;
	FS_FOpenFileByMode( va( "save/%s.svg", uiInfo.savegameList[index].savegameFile ), &f, FS_READ );
	if ( !f ) {
		return;
	}

	// read the version
    int ver;
	FS_Read( &ver, sizeof( ver ), f );


	// 'info' if > 11
	// 'mission' if > 12
	// 'skill' if >13
	// 'time' if > 14

	// if the version is wrong, just set some defaults and get out
	if ( ver < 9 ) {  // don't try anything for really old savegames
		FS_FCloseFile( f );
		uiInfo.savegameList[index].mapName          = "unknownmap";
		uiInfo.savegameList[index].episode          = -1;
		uiInfo.savegameList[index].savegameInfoText = "Gametime: (unknown)\nHealth: (unknown)\n(old savegame)";

		uiInfo.savegameList[index].date = "temp_date";
		uiInfo.savegameList[index].time = "(old savegame)";

		memset( &uiInfo.savegameList[index].tm, 0, sizeof( qtime_t ) );
		uiInfo.savegameList[index].time = String_Alloc( va( "(old savegame ver: %d)", ver ) );
		return;
	}

	// read the mapname
    char mapname[MAX_QPATH];
	FS_Read( mapname, MAX_QPATH, f );
	uiInfo.savegameList[index].mapName = String_Alloc( &mapname[0] );

	// read the level time
    int i;
	FS_Read( &i, sizeof( i ), f );

	// read the totalPlayTime
	FS_Read( &i, sizeof( i ), f );

	// read the episode
	FS_Read( &i, sizeof( i ), f );
	uiInfo.savegameList[index].episode = i;

	if ( ver < 12 ) {
		FS_FCloseFile( f );
		uiInfo.savegameList[index].savegameInfoText = "Gametime: (unknown)\nHealth: (unknown)\n(old savegame)";
		uiInfo.savegameList[index].date = "temp_date";
		memset( &uiInfo.savegameList[index].tm, 0, sizeof( qtime_t ) );
		uiInfo.savegameList[index].time = String_Alloc( va( "(old savegame ver: %d)", ver ) );
		return;
	}

	// read the info string length
	FS_Read( &i, sizeof( i ), f );

	// read the info string
    char buf[SAVE_INFOSTRING_LENGTH];
	FS_Read( buf, i, f );
	buf[i] = '\0';        //DAJ made it a char
	uiInfo.savegameList[index].savegameInfoText = String_Alloc( buf );

	// time
	if ( ver > 14 ) {
        qtime_t* tm = &uiInfo.savegameList[index].tm;
		FS_Read( &tm->tm_sec, sizeof( tm->tm_sec ), f );          // secs after the min
		FS_Read( &tm->tm_min, sizeof( tm->tm_min ), f );          // mins after the hour
		FS_Read( &tm->tm_hour, sizeof( tm->tm_hour ), f );        // hrs since midnight
		FS_Read( &tm->tm_mday, sizeof( tm->tm_mday ), f );
		FS_Read( &tm->tm_mon, sizeof( tm->tm_mon ), f );
		FS_Read( &tm->tm_year, sizeof( tm->tm_year ), f );        // yrs from 1900
		FS_Read( &tm->tm_wday, sizeof( tm->tm_wday ), f );
		FS_Read( &tm->tm_yday, sizeof( tm->tm_yday ), f );        // days since jan1 (0-365)
		FS_Read( &tm->tm_isdst, sizeof( tm->tm_isdst ), f );
		uiInfo.savegameList[index].time = String_Alloc( va( "%s %i, %i   %02i:%02i", monthStr[tm->tm_mon], tm->tm_mday, 1900 + tm->tm_year, tm->tm_hour, tm->tm_min ) );
	} else {
		memset( &uiInfo.savegameList[index].tm, 0, sizeof( qtime_t ) );
		uiInfo.savegameList[index].time = String_Alloc( va( "(old save ver: %d)", ver ) );
	}

	FS_FCloseFile( f );
}

/*
==============
UI_LoadSavegames
==============
*/
static void UI_LoadSavegames( char *dir ) {
	char sglist[4096];

	if ( dir ) {
		uiInfo.savegameCount = FS_GetFileList( va( "save/%s", dir ), "svg", sglist, 4096 );
	} else {
		uiInfo.savegameCount = FS_GetFileList( "save", "svg", sglist, 4096 );
	}

	if ( uiInfo.savegameCount ) {
		if ( uiInfo.savegameCount > MAX_SAVEGAMES ) {
			uiInfo.savegameCount = MAX_SAVEGAMES;
		}
		char *sgname = sglist;
        int remaining = 4096;
		for (int i = 0; i < uiInfo.savegameCount; i++ ) {

            size_t len = strnlen( sgname, remaining );
            remaining -= (len + 1);
            
			if ( !Q_stricmp( sgname, "current.svg" ) ) {    // ignore some savegames that have special uses and shouldn't be loaded by the user directly
				i--;
				uiInfo.savegameCount -= 1;
				sgname += len + 1;
				continue;
			}

			if ( !Q_stricmp( sgname +  len - 4,".svg" ) ) {
				sgname[len - 4] = '\0';
			}

			if ( dir ) {
				uiInfo.savegameList[i].savegameFile = String_Alloc( va( "%s/%s", dir, sgname ) );
			} else {
				uiInfo.savegameList[i].savegameFile = String_Alloc( sgname );
			}

			uiInfo.savegameList[i].savegameName = String_Alloc( sgname );

			// get string into list for sorting too
			uiInfo.savegameStatus.displaySavegames[i] = i;

			// read savegame and get needed info
			UI_ParseSavegame( i );

			if ( uiInfo.savegameList[i].episode != -1 ) {
				uiInfo.savegameList[i].sshotImage = RE_RegisterShaderNoMip( va( "levelshots/episodeshots/e%d.tga", uiInfo.savegameList[i].episode + 1 ) );
			} else {
				uiInfo.savegameList[i].sshotImage = RE_RegisterShaderNoMip( "levelshots/episodeshots/e_unknown.tga" );
			}

			sgname += len + 1;
		}

		// sort it
		UI_SavegameSort( 0, qtrue );

		// set current selection
//		i = UI_SavegameIndexFromName(ui_savegameName.string);
//		Menu_SetFeederSelection(NULL, FEEDER_SAVEGAMES, i, NULL);
	}
}

/*
==============
UI_Update
==============
*/
static void UI_Update( const char *name ) {
	int val = Cvar_VariableValue( name );

	if ( Q_stricmp( name, "ui_SetName" ) == 0 ) {
		Cvar_Set( "name", UI_Cvar_VariableString( "ui_Name" ) );
	} else if ( Q_stricmp( name, "ui_setRate" ) == 0 ) {
		float rate = Cvar_VariableValue( "rate" );
		if ( rate >= 5000 ) {
			Cvar_Set( "cl_maxpackets", "30" );
			Cvar_Set( "cl_packetdup", "1" );
		} else if ( rate >= 4000 ) {
			Cvar_Set( "cl_maxpackets", "15" );
			Cvar_Set( "cl_packetdup", "2" );       // favor less prediction errors when there's packet loss
		} else {
			Cvar_Set( "cl_maxpackets", "15" );
			Cvar_Set( "cl_packetdup", "1" );       // favor lower bandwidth
		}
	} else if ( Q_stricmp( name, "ui_GetName" ) == 0 ) {
		Cvar_Set( "ui_Name", UI_Cvar_VariableString( "name" ) );
	} else if ( Q_stricmp( name, "r_colorbits" ) == 0 ) {
		switch ( val ) {
		case 0:
			Cvar_SetValue( "r_depthbits", 0 );
			Cvar_SetValue( "r_stencilbits", 0 );
			break;
		case 16:
			Cvar_SetValue( "r_depthbits", 16 );
			Cvar_SetValue( "r_stencilbits", 0 );
			break;
		case 32:
			Cvar_SetValue( "r_depthbits", 24 );
			break;
		}
	} else if ( Q_stricmp( name, "r_lodbias" ) == 0 ) {
		switch ( val ) {
		case 0:
			Cvar_SetValue( "r_subdivisions", 4 );
			break;
		case 1:
			Cvar_SetValue( "r_subdivisions", 12 );
			break;
		case 2:
			Cvar_SetValue( "r_subdivisions", 20 );
			break;
		}
	} else if ( Q_stricmp( name, "ui_glCustom" ) == 0 ) {
		switch ( val ) {
		case 0: // high quality
			Cvar_SetValue( "r_fullScreen", 0 );
			Cvar_SetValue( "r_subdivisions", 4 );
			Cvar_SetValue( "r_vertexlight", 0 );
			Cvar_SetValue( "r_lodbias", 0 );
			Cvar_SetValue( "r_colorbits", 32 );
			Cvar_SetValue( "r_depthbits", 24 );
			Cvar_SetValue( "r_picmip", 0 );
			Cvar_SetValue( "r_picmip2", 0 );
			Cvar_SetValue( "r_mode", 4 );
			Cvar_SetValue( "r_texturebits", 32 );
			Cvar_SetValue( "r_fastSky", 0 );
			Cvar_SetValue( "r_inGameVideo", 1 );
			Cvar_SetValue( "cg_shadows", 1 );
			Cvar_SetValue( "cg_brassTime", 2500 );
			Cvar_Set( "r_texturemode", "GL_LINEAR_MIPMAP_LINEAR" );
			break;
		case 1: // normal
			Cvar_SetValue( "r_fullScreen", 0 );
			Cvar_SetValue( "r_subdivisions", 12 );
			Cvar_SetValue( "r_vertexlight", 0 );
			Cvar_SetValue( "r_lodbias", 0 );
			Cvar_SetValue( "r_colorbits", 0 );
			Cvar_SetValue( "r_depthbits", 24 );
			Cvar_SetValue( "r_picmip", 0 );
			Cvar_SetValue( "r_picmip2", 1 );
			Cvar_SetValue( "r_mode", 3 );
			Cvar_SetValue( "r_texturebits", 0 );
			Cvar_SetValue( "r_fastSky", 0 );
			Cvar_SetValue( "r_inGameVideo", 1 );
			Cvar_SetValue( "cg_brassTime", 2500 );
			Cvar_Set( "r_texturemode", "GL_LINEAR_MIPMAP_NEAREST" ); //----(SA)	modified so wolf never sets trilinear automatically
			Cvar_SetValue( "cg_shadows", 0 );
			break;
		case 2: // fast
			Cvar_SetValue( "r_fullScreen", 0 );
			Cvar_SetValue( "r_subdivisions", 8 );
			Cvar_SetValue( "r_vertexlight", 0 );
			Cvar_SetValue( "r_lodbias", 1 );
			Cvar_SetValue( "r_colorbits", 0 );
			Cvar_SetValue( "r_depthbits", 0 );
			Cvar_SetValue( "r_picmip", 1 );
			Cvar_SetValue( "r_picmip2", 2 );
			Cvar_SetValue( "r_mode", 3 );
			Cvar_SetValue( "r_texturebits", 0 );
			Cvar_SetValue( "cg_shadows", 0 );
			Cvar_SetValue( "r_fastSky", 1 );
			Cvar_SetValue( "r_inGameVideo", 0 );
			Cvar_SetValue( "cg_brassTime", 0 );
			Cvar_Set( "r_texturemode", "GL_LINEAR_MIPMAP_NEAREST" );
			break;
		case 3: // fastest
			Cvar_SetValue( "r_fullScreen", 0 );
			Cvar_SetValue( "r_subdivisions", 20 );
			Cvar_SetValue( "r_vertexlight", 1 );
			Cvar_SetValue( "r_lodbias", 2 );
			Cvar_SetValue( "r_colorbits", 16 );
			Cvar_SetValue( "r_depthbits", 16 );
			Cvar_SetValue( "r_mode", 3 );
			Cvar_SetValue( "r_picmip", 2 );
			Cvar_SetValue( "r_picmip2", 3 );
			Cvar_SetValue( "r_texturebits", 16 );
			Cvar_SetValue( "cg_shadows", 0 );
			Cvar_SetValue( "cg_brassTime", 0 );
			Cvar_SetValue( "r_fastSky", 1 );
			Cvar_SetValue( "r_inGameVideo", 0 );
			Cvar_Set( "r_texturemode", "GL_LINEAR_MIPMAP_NEAREST" );
			break;

		case 999:   // 999 is reserved for having set default values ("recommended")
			break;
		}
	} else if ( Q_stricmp( name, "ui_mousePitch" ) == 0 ) {
		if ( val == 0 ) {
			Cvar_SetValue( "m_pitch", 0.022f );
		} else {
			Cvar_SetValue( "m_pitch", -0.022f );
		}

	} else if ( Q_stricmp( name, "ui_savegameListAutosave" ) == 0 ) {
		if ( val == 0 ) {
			UI_LoadSavegames( NULL );
		} else {

			UI_LoadSavegames( "autosave" );    // get from default directory 'main/save/autosave/*.svg'
		}
	}

}

static qboolean saveExists(const char* name)
{
    for (int i = 0; i < uiInfo.savegameCount; i++ ) {
        if ( Q_stricmp( name, uiInfo.savegameList[uiInfo.savegameStatus.displaySavegames[i]].savegameName ) == 0 ) {
            return qtrue;
        }
    }
    return qfalse;
}

static void scriptSavegame(qboolean promptOverwrite)
{
    char name[MAX_NAME_LENGTH];

    name[0] = '\0';
    Q_strncpyz( name, UI_Cvar_VariableString( "ui_savegame" ), MAX_NAME_LENGTH );

    if ( !strnlen( name, MAX_NAME_LENGTH) ) {
        Menus_OpenByName( "save_name_popmenu" );
    } else {
        const qboolean exists = saveExists(name);
        if (exists && promptOverwrite){
            Menus_OpenByName( "save_overwrite_popmenu" );
        } else {
            Cbuf_ExecuteText( EXEC_APPEND, va( "savegame %s\n", UI_Cvar_VariableString( "ui_savegame" ), MAX_NAME_LENGTH ) );
            Menus_CloseAll();
        }
    }
}

static void scriptResetDefaults()
{
    Cbuf_ExecuteText( EXEC_NOW, "cvar_restart\n" );
    Cbuf_ExecuteText( EXEC_NOW, "exec default.cfg\n" );
    Cbuf_ExecuteText( EXEC_NOW, "exec language.cfg\n" );
    Cbuf_ExecuteText( EXEC_NOW, "setRecommended\n" );
    Controls_SetDefaults();
    Cvar_Set( "com_introPlayed", "1" );
    Cvar_Set( "com_recommendedSet", "1" );
    Cbuf_ExecuteText( EXEC_APPEND, "vid_restart\n" );
}

/*
==============
UI_RunMenuScript
==============
*/

static void UI_RunMenuScript( char **args ) {
	const char *name, *name2;
	char buff[1024];

	if ( String_Parse( args, &name ) ) {
		if ( Q_stricmp( name, "resetDefaults" ) == 0 ) {
            scriptResetDefaults();
		} else if ( Q_stricmp( name, "saveControls" ) == 0 ) {
			Controls_SetConfig( qtrue );
		} else if ( Q_stricmp( name, "loadControls" ) == 0 ) {
			Controls_GetConfig();
		} else if ( Q_stricmp( name, "clearError" ) == 0 ) {
			Cvar_Set( "com_errorMessage", "" );
		} else if ( Q_stricmp( name, "LoadSaveGames" ) == 0 ) {  // get the list
			UI_LoadSavegames( NULL );
		} else if ( Q_stricmp( name, "Loadgame" ) == 0 ) {
			int i = UI_SavegameIndexFromName2( ui_savegameName.string );
			Cbuf_ExecuteText( EXEC_APPEND, va( "loadgame %s\n", uiInfo.savegameList[i].savegameFile ) );
		} else if ( Q_stricmp( name, "Savegame" ) == 0 ) {
            // save.  throw dialog box if file exists
            scriptSavegame(qtrue);
		} else if ( Q_stricmp( name, "Savegame2" ) == 0 ) {
            scriptSavegame(qfalse); // save with no confirm for overwrite
		} else if ( Q_stricmp( name, "DelSavegame" ) == 0 ) {
			int i = UI_SavegameIndexFromName2( ui_savegameName.string );
			UI_DelSavegame();
		} else if ( Q_stricmp( name, "SavegameSort" ) == 0 ) {
			int sortColumn;
			if ( Int_Parse( args, &sortColumn ) ) {
				// if same column we're already sorting on then flip the direction
				if ( sortColumn == uiInfo.savegameStatus.sortKey ) {
					uiInfo.savegameStatus.sortDir = !uiInfo.savegameStatus.sortDir;
				}
				// make sure we sort again
				UI_SavegameSort( sortColumn, qtrue );
			}
		} else if ( Q_stricmp( name, "playerstart" ) == 0 ) {
			Cbuf_ExecuteText( EXEC_APPEND, "fade 0 0 0 0 3\n" );    // fade screen up
			Cvar_Set( "g_playerstart", "1" );                 // set cvar which will trigger "playerstart" in script
			Menus_CloseAll();
		} else if ( Q_stricmp( name, "Quit" ) == 0 ) {
			Cvar_Set( "ui_singlePlayerActive", "0" );
			Cbuf_ExecuteText( EXEC_NOW, "quit" );
		} else if ( Q_stricmp( name, "Controls" ) == 0 ) {
			Cvar_Set( "cl_paused", "1" );
			trap_Key_SetCatcher( KEYCATCH_UI );
			Menus_CloseAll();
			Menus_ActivateByName( "setup_menu2" );
		
		
		} else if ( Q_stricmp( name, "closeingame" ) == 0 ) {
			trap_Key_SetCatcher( trap_Key_GetCatcher() & ~KEYCATCH_UI );
			Key_ClearStates();
			Cvar_Set( "cl_paused", "0" );
			Menus_CloseAll();
		} else if ( Q_stricmp( name, "glCustom" ) == 0 ) {
			Cvar_Set( "ui_glCustom", "4" );
		} else if ( Q_stricmp( name, "update" ) == 0 ) {
			if ( String_Parse( args, &name2 ) ) {
				UI_Update( name2 );
			}
		} else if ( Q_stricmp( name, "setrecommended" ) == 0 ) {
			Cbuf_ExecuteText( EXEC_APPEND, "setRecommended 1\n" );
		} else {
			Com_Printf( "unknown UI script %s\n", name );
		}
	}
}

// NERVE - SMF
static void UI_FeederAddItem( float feederID, const char *name, int index ) {

}
// -NERVE - SMF


/*
==============
UI_FileText
==============
*/
static const char *UI_FileText( char *fileName )
{

	fileHandle_t f;
	static char buf[MAX_MENUDEFFILE];

	int len = FS_FOpenFileByMode( fileName, &f, FS_READ );
	if ( !f ) {
		return NULL;
	}

	FS_Read( buf, len, f );
	buf[len] = 0;
	FS_FCloseFile( f );
	return &buf[0];
}

//----(SA)	added
/*
==============
UI_translateString
==============
*/
static const char *UI_translateString( const char *inString )
{
	int numStrings = sizeof( translateStrings ) / sizeof( translateStrings[0] ) - 1;

	for (int i = 0; i < numStrings; i++ ) {
		if ( !translateStrings[i].name || !strnlen( translateStrings[i].name, 1024 ) ) {
			return inString;
		}

		if ( !strcmp( inString, translateStrings[i].name ) ) {
			if ( translateStrings[i].localname && strnlen( translateStrings[i].localname, 1024 ) ) {
				return translateStrings[i].localname;
			}
			break;
		}
	}

	return inString;
}
//----(SA)	end

static qhandle_t UI_FeederItemImage( float feederID, int index ) {
    if ( feederID == FEEDER_SAVEGAMES ) {
        if ( index >= 0 && index < uiInfo.savegameCount ) {
            if ( uiInfo.savegameList[uiInfo.savegameStatus.displaySavegames[index]].sshotImage == -1 ) {
                uiInfo.savegameList[uiInfo.savegameStatus.displaySavegames[index]].sshotImage = RE_RegisterShaderNoMip( va( "levelshots/%s.tga",uiInfo.savegameList[uiInfo.savegameStatus.displaySavegames[index]].mapName ) );
            }
            return uiInfo.savegameList[uiInfo.savegameStatus.displaySavegames[index]].sshotImage;
        }
    }

    return 0;
}

static void UI_FeederSelection( float feederID, int index ) {
    static char info[MAX_STRING_CHARS];
   
    if ( feederID == FEEDER_CINEMATICS ) {
        uiInfo.movieIndex = index;
        if ( uiInfo.previewMovie >= 0 ) {
            trap_CIN_StopCinematic( uiInfo.previewMovie );
        }
        uiInfo.previewMovie = -1;
    } else if ( feederID == FEEDER_SAVEGAMES ) {
        if ( index >= 0 && index < uiInfo.savegameCount ) {
            
            // the text entry box
            Cvar_Set( "ui_savegame", uiInfo.savegameList[uiInfo.savegameStatus.displaySavegames[index]].savegameName );
            
            Cvar_Set( "ui_savegameName", uiInfo.savegameList[uiInfo.savegameStatus.displaySavegames[index]].savegameName );
            Cvar_Set( "ui_savegameInfo", uiInfo.savegameList[uiInfo.savegameStatus.displaySavegames[index]].savegameInfoText );
        }
    }
}

static int UI_FeederCount( float feederID ) {
    if ( feederID == FEEDER_SAVEGAMES ) {
        return uiInfo.savegameCount;
    }
    return 0;
}
/*
==============
UI_FeederItemText
==============
*/
static const char *UI_FeederItemText( float feederID, int index, int column, qhandle_t *handle )
{
	*handle = -1;
	if ( feederID == FEEDER_CINEMATICS ) {
		if ( index >= 0 && index < uiInfo.movieCount ) {
			return uiInfo.movieList[index];
		}
	} else if ( feederID == FEEDER_SAVEGAMES ) {
		if ( index >= 0 && index < uiInfo.savegameCount ) {
			switch ( column ) {
			case SORT_SAVENAME:
				return uiInfo.savegameList[uiInfo.savegameStatus.displaySavegames[index]].savegameName;
				break;
			case SORT_SAVETIME:
				return uiInfo.savegameList[uiInfo.savegameStatus.displaySavegames[index]].time;
				break;

			}
		}
	}
	
	return "";
}


static void UI_Pause( qboolean b )
{
	if ( b ) {
		// pause the game and set the ui keycatcher
		Cvar_Set( "cl_paused", "1" );
		trap_Key_SetCatcher( KEYCATCH_UI );
	} else {
		// unpause the game and clear the ui keycatcher
		trap_Key_SetCatcher( trap_Key_GetCatcher() & ~KEYCATCH_UI );
		Key_ClearStates();
		Cvar_Set( "cl_paused", "0" );
	}
}


static int UI_PlayCinematic( const char *name, float x, float y, float w, float h ) {
	return trap_CIN_PlayCinematic( name, x, y, w, h, ( CIN_loop | CIN_silent ) );
}

static void UI_StopCinematic( int handle ) {
	if ( handle >= 0 ) {
		trap_CIN_StopCinematic( handle );
	}
}

static void UI_DrawCinematic( int handle, float x, float y, float w, float h ) {
	CIN_SetExtents( handle, x, y, w, h );
	trap_CIN_DrawCinematic( handle );
}

static void UI_RunCinematicFrame( int handle ) {
	trap_CIN_RunCinematic( handle );
}


/*
=================
UI_KeyEvent
=================
*/
void UI_KeyEvent( int key, qboolean down )
{
	if ( Menu_Count() > 0 ) {
		menuDef_t *menu = Menu_GetFocused();
		if ( menu ) {
			if ( key == K_ESCAPE && down && !Menus_AnyFullScreenVisible() ) {
				Menus_CloseAll();
			} else {
				Menu_HandleKey( menu, key, down );
			}
		} else {
			trap_Key_SetCatcher( trap_Key_GetCatcher() & ~KEYCATCH_UI );
			Key_ClearStates();
			Cvar_Set( "cl_paused", "0" );
		}
	}
}

/*
=================
UI_MouseEvent
=================
*/
void UI_MouseEvent( int dx, int dy )
{
	// update mouse screen position
	uiInfo.uiDC.cursorx += dx;
	if ( uiInfo.uiDC.cursorx < 0 ) {
		uiInfo.uiDC.cursorx = 0;
	} else if ( uiInfo.uiDC.cursorx > SCREEN_WIDTH ) {
		uiInfo.uiDC.cursorx = SCREEN_WIDTH;
	}

	uiInfo.uiDC.cursory += dy;
	if ( uiInfo.uiDC.cursory < 0 ) {
		uiInfo.uiDC.cursory = 0;
	} else if ( uiInfo.uiDC.cursory > SCREEN_HEIGHT ) {
		uiInfo.uiDC.cursory = SCREEN_HEIGHT;
	}

	if ( Menu_Count() > 0 ) {
		Display_MouseMove( NULL, uiInfo.uiDC.cursorx, uiInfo.uiDC.cursory );
	}

}

void UI_LoadNonIngame()
{
	const char *menuSet = UI_Cvar_VariableString( "ui_menuFiles" );
	if ( menuSet == NULL || menuSet[0] == '\0' ) {
		menuSet = "ui/menus.txt";
	}
	UI_LoadMenus( menuSet, qfalse );
	uiInfo.inGameLoad = qfalse;
}

/*
==============
UI_GetActiveMenu
==============
*/
uiMenuCommand_t UI_GetActiveMenu( void ) {
	return menutype;
}

void UI_SetActiveMenu( uiMenuCommand_t menu )
{
	// this should be the ONLY way the menu system is brought up
	// enusure minumum menu data is cached
	if ( Menu_Count() > 0 ) {
		vec3_t v;
		v[0] = v[1] = v[2] = 0;

		if ( menu == UIMENU_BRIEFING && menutype == menu ) { // don't let briefing be set multiple times
			return;
		}

		menutype = menu;    //----(SA)	added

		switch ( menu ) {
		case UIMENU_NONE:
			trap_Key_SetCatcher( trap_Key_GetCatcher() & ~KEYCATCH_UI );
			Key_ClearStates();
			Cvar_Set( "cl_paused", "0" );
			Menus_CloseAll();

			return;
		case UIMENU_MAIN:
			trap_Key_SetCatcher( KEYCATCH_UI );
			if ( uiInfo.inGameLoad ) {
				UI_LoadNonIngame();
			}
			Menus_CloseAll();
			Menus_ActivateByName( "main" );
            char buf[256];
			Cvar_VariableStringBuffer( "com_errorMessage", buf, sizeof( buf ) );
			if ( strnlen( buf, 256 ) ) {
				Menus_ActivateByName( "error_popmenu" );
			}
			// ensure sound is there for the menu
			trap_S_FadeAllSound( 1.0f, 1000 );    // make sure sound fades up

			// ensure savegames are loadable
			Cvar_Set( "g_reloading", "0" );

			return;


		case UIMENU_ENDGAME:
			// ensure sound is there for the menu
			trap_S_FadeAllSound( 1.0f, 1000 );
			// ensure savegames are loadable
			Cvar_Set( "g_reloading", "0" );

			trap_Key_SetCatcher( KEYCATCH_UI );
			Menus_ActivateByName( "credit" );
			return;

		case UIMENU_POSTGAME:
			trap_Key_SetCatcher( KEYCATCH_UI );
			if ( uiInfo.inGameLoad ) {
				UI_LoadNonIngame();
			}
			Menus_CloseAll();
			Menus_ActivateByName( "endofgame" );
			return;

		case UIMENU_INGAME:
			Cvar_Set( "cl_paused", "1" );
			trap_Key_SetCatcher( KEYCATCH_UI );

			Menus_CloseAll();
			Menus_ActivateByName( "ingame" );
			return;

		case UIMENU_PREGAME:
			Cvar_Set( "cl_paused", "1" );
			trap_Key_SetCatcher( KEYCATCH_UI );
			Menus_CloseAll();
			Menus_ActivateByName( "pregame" );
			return;

		case UIMENU_NOTEBOOK:
			Cvar_Set( "cl_paused", "1" );
			trap_Key_SetCatcher( KEYCATCH_UI );
			Menus_CloseAll();
			Menus_ActivateByName( "notebook" );
			return;

		case UIMENU_BOOK1:
		case UIMENU_BOOK2:
		case UIMENU_BOOK3:
			trap_Key_SetCatcher( KEYCATCH_UI );
			Menus_CloseAll();
			Menus_ActivateByName( va( "hbook%d", ( menu - UIMENU_BOOK1 ) + 1 ) );
			return;

		case UIMENU_CLIPBOARD:
			Cvar_Set( "cl_paused", "1" );
			trap_Key_SetCatcher( KEYCATCH_UI );
			Menus_CloseAll();
			Menus_ActivateByName( "clipboard" );
			return;

		case UIMENU_BRIEFING:
			Menus_CloseAll();
			Menus_ActivateByName( "briefing" );
                return;

		case UIMENU_WM_QUICKMESSAGE:
			trap_Key_SetCatcher( KEYCATCH_UI );
			Menus_CloseAll();
			Menus_OpenByName( "wm_quickmessage" );
			return;

		default:
			break;
		}
	}
}

qboolean UI_IsFullscreen( void ) {
	return Menus_AnyFullScreenVisible();
}



static connstate_t lastConnState;
static char lastLoadingText[MAX_INFO_VALUE];

static void UI_ReadableSize( char *buf, int bufsize, int value ) {
	if ( value > 1024 * 1024 * 1024 ) { // gigs
		snprintf( buf, bufsize, "%d", value / ( 1024 * 1024 * 1024 ) );
		snprintf( buf + strnlen( buf, bufsize ), bufsize - strnlen( buf, bufsize ), ".%02d GB",
					 ( value % ( 1024 * 1024 * 1024 ) ) * 100 / ( 1024 * 1024 * 1024 ) );
	} else if ( value > 1024 * 1024 ) { // megs
		snprintf( buf, bufsize, "%d", value / ( 1024 * 1024 ) );
		snprintf( buf + strnlen( buf, bufsize ), bufsize - strnlen( buf, bufsize ), ".%02d MB",
					 ( value % ( 1024 * 1024 ) ) * 100 / ( 1024 * 1024 ) );
	} else if ( value > 1024 ) { // kilos
		snprintf( buf, bufsize, "%d KB", value / 1024 );
	} else { // bytes
		snprintf( buf, bufsize, "%d bytes", value );
	}
}

// Assumes time is in msec
static void UI_PrintTime( char *buf, int bufsize, int time ) {
	time /= 1000;  // change to seconds

	if ( time > 3600 ) { // in the hours range
		snprintf( buf, bufsize, "%d hr %d min", time / 3600, ( time % 3600 ) / 60 );
	} else if ( time > 60 ) { // mins
		snprintf( buf, bufsize, "%d min %d sec", time / 60, time % 60 );
	} else  { // secs
		snprintf( buf, bufsize, "%d sec", time );
	}
}

void Text_PaintCenter( float x, float y, int font, float scale, vec4_t color, const char *text, float adjust ) {
	int len = Text_Width( text, font, scale, 0 );
	Text_Paint( x - len / 2, y, font, scale, color, text, 0, 0, ITEM_TEXTSTYLE_SHADOWEDMORE );
}


/*
================
cvars
================
*/

typedef struct {
	vmCvar_t    *vmCvar;
	char        *cvarName;
	char        *defaultString;
	int cvarFlags;
} cvarTable_t;

vmCvar_t ui_ffa_fraglimit;
vmCvar_t ui_ffa_timelimit;

vmCvar_t ui_tourney_fraglimit;
vmCvar_t ui_tourney_timelimit;

vmCvar_t ui_team_fraglimit;
vmCvar_t ui_team_timelimit;
vmCvar_t ui_team_friendly;

vmCvar_t ui_ctf_capturelimit;
vmCvar_t ui_ctf_timelimit;
vmCvar_t ui_ctf_friendly;

vmCvar_t ui_arenasFile;
vmCvar_t ui_botsFile;
vmCvar_t ui_spScores1;
vmCvar_t ui_spScores2;
vmCvar_t ui_spScores3;
vmCvar_t ui_spScores4;
vmCvar_t ui_spScores5;
vmCvar_t ui_spAwards;
vmCvar_t ui_spVideos;
vmCvar_t ui_spSkill;

vmCvar_t ui_spSelection;
vmCvar_t ui_master;

vmCvar_t ui_brassTime;
vmCvar_t ui_drawCrosshair;
vmCvar_t ui_drawCrosshairNames;
vmCvar_t ui_drawCrosshairPickups;       //----(SA) added
vmCvar_t ui_useSuggestedWeapons;    //----(SA)	added
vmCvar_t ui_marks;
// JOSEPH 12-3-99
vmCvar_t ui_autoactivate;
vmCvar_t ui_emptyswitch;        //----(SA)	added
// END JOSEPH

vmCvar_t ui_server1;
vmCvar_t ui_server2;
vmCvar_t ui_server3;
vmCvar_t ui_server4;
vmCvar_t ui_server5;
vmCvar_t ui_server6;
vmCvar_t ui_server7;
vmCvar_t ui_server8;
vmCvar_t ui_server9;
vmCvar_t ui_server10;
vmCvar_t ui_server11;
vmCvar_t ui_server12;
vmCvar_t ui_server13;
vmCvar_t ui_server14;
vmCvar_t ui_server15;
vmCvar_t ui_server16;

vmCvar_t ui_cdkeychecked;
vmCvar_t ui_smallFont;
vmCvar_t ui_bigFont;

vmCvar_t ui_selectedPlayer;
vmCvar_t ui_selectedPlayerName;
vmCvar_t ui_netSource;
vmCvar_t ui_menuFiles;
vmCvar_t ui_gameType;
vmCvar_t ui_netGameType;
vmCvar_t ui_actualNetGameType;
vmCvar_t ui_joinGameType;
vmCvar_t ui_dedicated;

vmCvar_t ui_notebookCurrentPage;        //----(SA)	added
vmCvar_t ui_clipboardName;          // the name of the group for the current clipboard item //----(SA)	added
vmCvar_t ui_hudAlpha;
vmCvar_t ui_hunkUsed;       //----(SA)	added
vmCvar_t ui_cameraMode;     //----(SA)	added
vmCvar_t ui_savegameListAutosave;       //----(SA)	added
vmCvar_t ui_savegameName;

static cvarTable_t cvarTable[] = {
	{ &ui_ffa_fraglimit, "ui_ffa_fraglimit", "20", CVAR_ARCHIVE },
	{ &ui_ffa_timelimit, "ui_ffa_timelimit", "0", CVAR_ARCHIVE },

	{ &ui_tourney_fraglimit, "ui_tourney_fraglimit", "0", CVAR_ARCHIVE },
	{ &ui_tourney_timelimit, "ui_tourney_timelimit", "15", CVAR_ARCHIVE },

	{ &ui_team_fraglimit, "ui_team_fraglimit", "0", CVAR_ARCHIVE },
	{ &ui_team_timelimit, "ui_team_timelimit", "20", CVAR_ARCHIVE },
	{ &ui_team_friendly, "ui_team_friendly",  "1", CVAR_ARCHIVE },

	{ &ui_ctf_capturelimit, "ui_ctf_capturelimit", "8", CVAR_ARCHIVE },
	{ &ui_ctf_timelimit, "ui_ctf_timelimit", "30", CVAR_ARCHIVE },
	{ &ui_ctf_friendly, "ui_ctf_friendly",  "0", CVAR_ARCHIVE },

	{ &ui_arenasFile, "g_arenasFile", "", CVAR_INIT | CVAR_ROM },
	{ &ui_botsFile, "g_botsFile", "", CVAR_INIT | CVAR_ROM },
	{ &ui_spScores1, "g_spScores1", "", CVAR_ARCHIVE | CVAR_ROM },
	{ &ui_spScores2, "g_spScores2", "", CVAR_ARCHIVE | CVAR_ROM },
	{ &ui_spScores3, "g_spScores3", "", CVAR_ARCHIVE | CVAR_ROM },
	{ &ui_spScores4, "g_spScores4", "", CVAR_ARCHIVE | CVAR_ROM },
	{ &ui_spScores5, "g_spScores5", "", CVAR_ARCHIVE | CVAR_ROM },
	{ &ui_spAwards, "g_spAwards", "", CVAR_ARCHIVE | CVAR_ROM },
	{ &ui_spVideos, "g_spVideos", "", CVAR_ARCHIVE | CVAR_ROM },
	{ &ui_spSkill, "g_spSkill", "2", CVAR_ARCHIVE | CVAR_LATCH },

	{ &ui_spSelection, "ui_spSelection", "", CVAR_ROM },
	{ &ui_master, "ui_master", "0", CVAR_ARCHIVE },

	{ &ui_brassTime, "cg_brassTime", "1250", CVAR_ARCHIVE },
	{ &ui_drawCrosshair, "cg_drawCrosshair", "4", CVAR_ARCHIVE },
	{ &ui_drawCrosshairNames, "cg_drawCrosshairNames", "1", CVAR_ARCHIVE },
	{ &ui_drawCrosshairPickups, "cg_drawCrosshairPickups", "1", CVAR_ARCHIVE },   //----(SA) added
	{ &ui_marks, "cg_marktime", "20000", CVAR_ARCHIVE },
	{ &ui_autoactivate, "cg_autoactivate", "1", CVAR_ARCHIVE },
	{ &ui_useSuggestedWeapons, "cg_useSuggestedWeapons", "1", CVAR_ARCHIVE }, //----(SA)	added
	{ &ui_emptyswitch, "cg_emptyswitch", "0", CVAR_ARCHIVE }, //----(SA)	added
	{ &ui_server1, "server1", "", CVAR_ARCHIVE },
	{ &ui_server2, "server2", "", CVAR_ARCHIVE },
	{ &ui_server3, "server3", "", CVAR_ARCHIVE },
	{ &ui_server4, "server4", "", CVAR_ARCHIVE },
	{ &ui_server5, "server5", "", CVAR_ARCHIVE },
	{ &ui_server6, "server6", "", CVAR_ARCHIVE },
	{ &ui_server7, "server7", "", CVAR_ARCHIVE },
	{ &ui_server8, "server8", "", CVAR_ARCHIVE },
	{ &ui_server9, "server9", "", CVAR_ARCHIVE },
	{ &ui_server10, "server10", "", CVAR_ARCHIVE },
	{ &ui_server11, "server11", "", CVAR_ARCHIVE },
	{ &ui_server12, "server12", "", CVAR_ARCHIVE },
	{ &ui_server13, "server13", "", CVAR_ARCHIVE },
	{ &ui_server14, "server14", "", CVAR_ARCHIVE },
	{ &ui_server15, "server15", "", CVAR_ARCHIVE },
	{ &ui_server16, "server16", "", CVAR_ARCHIVE },
	{ &ui_dedicated, "ui_dedicated", "0", CVAR_ARCHIVE },
	{ &ui_smallFont, "ui_smallFont", "0.25", CVAR_ARCHIVE},
	{ &ui_bigFont, "ui_bigFont", "0.4", CVAR_ARCHIVE},
	{ &ui_cdkeychecked, "ui_cdkeychecked", "0", CVAR_ROM },
	{ &ui_selectedPlayer, "cg_selectedPlayer", "0", CVAR_ARCHIVE},
	{ &ui_selectedPlayerName, "cg_selectedPlayerName", "", CVAR_ARCHIVE},
	{ &ui_netSource, "ui_netSource", "0", CVAR_ARCHIVE },
	{ &ui_menuFiles, "ui_menuFiles", "ui/menus.txt", CVAR_ARCHIVE },

	{ &ui_joinGameType, "ui_joinGametype", "0", CVAR_ARCHIVE },
	{ &ui_netGameType, "ui_netGametype", "3", CVAR_ARCHIVE },
	{ &ui_actualNetGameType, "ui_actualNetGametype", "0", CVAR_ARCHIVE },
	{ &ui_notebookCurrentPage, "ui_notebookCurrentPage", "1", CVAR_ROM},
	{ &ui_clipboardName, "cg_clipboardName", "", CVAR_ROM },

	{ &ui_hudAlpha, "cg_hudAlpha", "0.8", CVAR_ARCHIVE },
	{ &ui_hunkUsed, "com_hunkused", "0", 0 },     //----(SA)	added
	{ &ui_cameraMode, "com_cameraMode", "0", 0},  //----(SA)	added

	{ &ui_savegameName, "ui_savegameName", "", CVAR_ROM}


};

static int cvarTableSize = sizeof( cvarTable ) / sizeof( cvarTable[0] );


/*
=================
UI_RegisterCvars
=================
*/
static
void UI_RegisterCvars( void ) {
	int i;
	cvarTable_t *cv;

	for ( i = 0, cv = cvarTable ; i < cvarTableSize ; i++, cv++ ) {
		Cvar_Register( cv->vmCvar, cv->cvarName, cv->defaultString, cv->cvarFlags );
	}
}

/*
=================
UI_UpdateCvars
=================
*/
static
void UI_UpdateCvars( void ) {
	int i;
	cvarTable_t *cv;

	for ( i = 0, cv = cvarTable ; i < cvarTableSize ; i++, cv++ ) {
		Cvar_Update( cv->vmCvar );
	}
}

int frameCount = 0;
int startTime;

#define UI_FPS_FRAMES   4
void UI_Refresh( int realtime ) {
    static int index;
    static int previousTimes[UI_FPS_FRAMES];

    //if ( !( trap_Key_GetCatcher() & KEYCATCH_UI ) ) {
    //    return;
    //}

    uiInfo.uiDC.frameTime = realtime - uiInfo.uiDC.realTime;
    uiInfo.uiDC.realTime = realtime;

    previousTimes[index % UI_FPS_FRAMES] = uiInfo.uiDC.frameTime;
    index++;
    if ( index > UI_FPS_FRAMES ) {
        int i, total;
        // average multiple frames together to smooth changes out a bit
        total = 0;
        for ( i = 0 ; i < UI_FPS_FRAMES ; i++ ) {
            total += previousTimes[i];
        }
        if ( !total ) {
            total = 1;
        }
        uiInfo.uiDC.FPS = 1000 * UI_FPS_FRAMES / total;
    }
    
    UI_UpdateCvars();

    if ( Menu_Count() > 0 ) {
        // paint all the menus
        Menu_PaintAll();
    }

    // draw cursor
    UI_SetColor( NULL );
    if ( Menu_Count() > 0 ) {
        uiMenuCommand_t mymenu = UI_GetActiveMenu();
        if ( mymenu != UIMENU_BRIEFING ) {
            UI_DrawHandlePic( uiInfo.uiDC.cursorx - 16, uiInfo.uiDC.cursory - 16, 32, 32, uiInfo.uiDC.Assets.cursor );
        }
    }
}

/*
=================
UI_Init
=================
*/
void UI_Init()
{
    UI_RegisterCvars();
    UI_InitMemory();

    // cache redundant calulations
    CL_GetGlconfig( &uiInfo.uiDC.glconfig );

    // for 640x480 virtualized screen
    uiInfo.uiDC.yscale = uiInfo.uiDC.glconfig.vidHeight * ( 1.0 / 480.0 );
    uiInfo.uiDC.xscale = uiInfo.uiDC.glconfig.vidWidth * ( 1.0 / 640.0 );
    if ( uiInfo.uiDC.glconfig.vidWidth * 480 > uiInfo.uiDC.glconfig.vidHeight * 640 ) {
        // wide screen
        uiInfo.uiDC.bias = 0.5 * ( uiInfo.uiDC.glconfig.vidWidth - ( uiInfo.uiDC.glconfig.vidHeight * ( 640.0 / 480.0 ) ) );
    } else {
        // no wide screen
        uiInfo.uiDC.bias = 0;
    }

    uiInfo.uiDC.registerShaderNoMip = &RE_RegisterShaderNoMip;
    uiInfo.uiDC.setColor = &UI_SetColor;
    uiInfo.uiDC.drawHandlePic = &UI_DrawHandlePic;
    uiInfo.uiDC.drawStretchPic = &trap_R_DrawStretchPic;

    uiInfo.uiDC.textWidth = &Text_Width;
    uiInfo.uiDC.textHeight = &Text_Height;

    uiInfo.uiDC.modelBounds = &trap_R_ModelBounds;
    uiInfo.uiDC.fillRect = &UI_FillRect;
    uiInfo.uiDC.drawRect = &UI_DrawRect;

    uiInfo.uiDC.drawTopBottom = &UI_DrawTopBottom;
    uiInfo.uiDC.clearScene = &trap_R_ClearScene;
    uiInfo.uiDC.addRefEntityToScene = &trap_R_AddRefEntityToScene;
    uiInfo.uiDC.renderScene = &trap_R_RenderScene;
    uiInfo.uiDC.registerFont = &trap_R_RegisterFont;
    uiInfo.uiDC.getValue = &UI_GetValue;
    uiInfo.uiDC.ownerDrawVisible = &UI_OwnerDrawVisible;
    uiInfo.uiDC.runScript = &UI_RunMenuScript;

    uiInfo.uiDC.setCVar = Cvar_Set;

    uiInfo.uiDC.startLocalSound = &S_StartLocalSound;
    uiInfo.uiDC.feederCount = &UI_FeederCount;
    uiInfo.uiDC.feederItemImage = &UI_FeederItemImage;
    uiInfo.uiDC.feederItemText = &UI_FeederItemText;
    uiInfo.uiDC.fileText = &UI_FileText;    //----(SA)

    uiInfo.uiDC.getTranslatedString = &UI_translateString;  //----(SA) added

    uiInfo.uiDC.feederSelection = &UI_FeederSelection;
    uiInfo.uiDC.feederAddItem = &UI_FeederAddItem;                  // NERVE - SMF


    uiInfo.uiDC.executeText = &Cbuf_ExecuteText;
    uiInfo.uiDC.Error = &Com_Error;
    uiInfo.uiDC.Print = &Com_Printf;
    uiInfo.uiDC.Pause = &UI_Pause;
    uiInfo.uiDC.ownerDrawWidth = &UI_OwnerDrawWidth;
    uiInfo.uiDC.registerSound = &S_RegisterSound;
    uiInfo.uiDC.startBackgroundTrack = &trap_S_StartBackgroundTrack;
    uiInfo.uiDC.stopBackgroundTrack = &trap_S_StopBackgroundTrack;
    uiInfo.uiDC.playCinematic = &UI_PlayCinematic;
    uiInfo.uiDC.stopCinematic = &UI_StopCinematic;
    uiInfo.uiDC.drawCinematic = &UI_DrawCinematic;
    uiInfo.uiDC.runCinematicFrame = &UI_RunCinematicFrame;

    Init_Display( &uiInfo.uiDC );

    String_Init();

    // load translation text
    UI_LoadTranslationStrings();

    uiInfo.uiDC.whiteShader = RE_RegisterShaderNoMip( "white" );

    AssetCache();

    uiInfo.characterCount = 0;
    uiInfo.aliasCount = 0;

    const char *menuSet = UI_Cvar_VariableString( "ui_menuFiles" );
    if ( menuSet == NULL || menuSet[0] == '\0' ) {
        menuSet = "ui/menus.txt";
    }

    UI_LoadMenus( menuSet, qtrue );
    UI_LoadMenus( "ui/ingame.txt", qfalse );

    Menus_CloseAll();

    // sets defaults for ui temp cvars
    uiInfo.effectsColor = gamecodetoui[(int)Cvar_VariableValue( "color" ) - 1];
    uiInfo.currentCrosshair = (int)Cvar_VariableValue( "cg_drawCrosshair" );
    Cvar_Set( "ui_mousePitch", ( Cvar_VariableValue( "m_pitch" ) >= 0 ) ? "0" : "1" );

    uiInfo.serverStatus.currentServerCinematic = -1;
    uiInfo.previewMovie = -1;

    if ( Cvar_VariableValue( "ui_WolfFirstRun" ) == 0 ) {
        Cvar_Set( "s_volume", "0.8" );
        Cvar_Set( "s_musicvolume", "0.8" );
        Cvar_Set( "ui_WolfFirstRun", "1" );
    }

    Cvar_Register( NULL, "debug_protocol", "", 0 );
}
