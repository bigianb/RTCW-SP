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

#pragma once

#include "../game/q_shared.h"
#include "../cgame/tr_types.h"
#include "../cgame/cg_local.h"
#include "ui_public.h"
#include "keycodes.h"
#include "../game/bg_public.h"
#include "ui_shared.h"

extern vmCvar_t ui_ffa_fraglimit;
extern vmCvar_t ui_ffa_timelimit;

extern vmCvar_t ui_tourney_fraglimit;
extern vmCvar_t ui_tourney_timelimit;

extern vmCvar_t ui_team_fraglimit;
extern vmCvar_t ui_team_timelimit;
extern vmCvar_t ui_team_friendly;

extern vmCvar_t ui_ctf_capturelimit;
extern vmCvar_t ui_ctf_timelimit;
extern vmCvar_t ui_ctf_friendly;

extern vmCvar_t ui_arenasFile;
extern vmCvar_t ui_botsFile;
extern vmCvar_t ui_spScores1;
extern vmCvar_t ui_spScores2;
extern vmCvar_t ui_spScores3;
extern vmCvar_t ui_spScores4;
extern vmCvar_t ui_spScores5;
extern vmCvar_t ui_spAwards;
extern vmCvar_t ui_spVideos;
extern vmCvar_t ui_spSkill;

extern vmCvar_t ui_spSelection;
extern vmCvar_t ui_master;

extern vmCvar_t ui_brassTime;
extern vmCvar_t ui_drawCrosshair;
extern vmCvar_t ui_drawCrosshairNames;
extern vmCvar_t ui_drawCrosshairPickups;    //----(SA) added
extern vmCvar_t ui_useSuggestedWeapons;     //----(SA)	added
extern vmCvar_t ui_marks;
// JOSEPH 12-3-99
extern vmCvar_t ui_autoactivate;
extern vmCvar_t ui_emptyswitch;
// END JOSEPH

extern vmCvar_t ui_server1;
extern vmCvar_t ui_server2;
extern vmCvar_t ui_server3;
extern vmCvar_t ui_server4;
extern vmCvar_t ui_server5;
extern vmCvar_t ui_server6;
extern vmCvar_t ui_server7;
extern vmCvar_t ui_server8;
extern vmCvar_t ui_server9;
extern vmCvar_t ui_server10;
extern vmCvar_t ui_server11;
extern vmCvar_t ui_server12;
extern vmCvar_t ui_server13;
extern vmCvar_t ui_server14;
extern vmCvar_t ui_server15;
extern vmCvar_t ui_server16;

extern vmCvar_t ui_smallFont;
extern vmCvar_t ui_bigFont;
extern vmCvar_t ui_cdkey;
extern vmCvar_t ui_cdkeychecked;
extern vmCvar_t ui_selectedPlayer;
extern vmCvar_t ui_selectedPlayerName;
extern vmCvar_t ui_netSource;
extern vmCvar_t ui_menuFiles;
extern vmCvar_t ui_gameType;
extern vmCvar_t ui_netGameType;
extern vmCvar_t ui_actualNetGameType;
extern vmCvar_t ui_joinGameType;
extern vmCvar_t ui_dedicated;
extern vmCvar_t ui_notebookCurrentPage;
extern vmCvar_t ui_clipboardName;
extern vmCvar_t ui_hudAlpha;
extern vmCvar_t ui_hunkUsed;    //----(SA)	added
extern vmCvar_t ui_cameraMode;  //----(SA)	added
extern vmCvar_t ui_savegameListAutosave;    //----(SA)	added
extern vmCvar_t ui_savegameName;    //----(SA)	added

// NERVE - SMF - multiplayer cvars
extern vmCvar_t ui_serverFilterType;
extern vmCvar_t ui_currentNetMap;
extern vmCvar_t ui_currentMap;
extern vmCvar_t ui_mapIndex;

extern vmCvar_t ui_browserMaster;
extern vmCvar_t ui_browserGameType;
extern vmCvar_t ui_browserSortKey;
extern vmCvar_t ui_browserShowFull;
extern vmCvar_t ui_browserShowEmpty;

extern vmCvar_t ui_serverStatusTimeOut;
// -NERVE - SMF

//
// ui_qmenu.c
//

#define RCOLUMN_OFFSET          ( BIGCHAR_WIDTH )
#define LCOLUMN_OFFSET          ( -BIGCHAR_WIDTH )

#define SLIDER_RANGE            10
#define MAX_EDIT_LINE           256

#define MAX_MENUDEPTH           8
//#define MAX_MENUITEMS			128
#define MAX_MENUITEMS           256

#define MTYPE_NULL              0
#define MTYPE_SLIDER            1
#define MTYPE_ACTION            2
#define MTYPE_SPINCONTROL       3
#define MTYPE_FIELD             4
#define MTYPE_RADIOBUTTON       5
#define MTYPE_BITMAP            6
#define MTYPE_TEXT              7
#define MTYPE_SCROLLLIST        8
#define MTYPE_PTEXT             9
#define MTYPE_BTEXT             10

#define QMF_BLINK               0x00000001
#define QMF_SMALLFONT           0x00000002
#define QMF_LEFT_JUSTIFY        0x00000004
#define QMF_CENTER_JUSTIFY      0x00000008
#define QMF_RIGHT_JUSTIFY       0x00000010
#define QMF_NUMBERSONLY         0x00000020  // edit field is only numbers
#define QMF_HIGHLIGHT           0x00000040
#define QMF_HIGHLIGHT_IF_FOCUS  0x00000080  // steady focus
#define QMF_PULSEIFFOCUS        0x00000100  // pulse if focus
#define QMF_HASMOUSEFOCUS       0x00000200
#define QMF_NOONOFFTEXT         0x00000400
#define QMF_MOUSEONLY           0x00000800  // only mouse input allowed
#define QMF_HIDDEN              0x00001000  // skips drawing
#define QMF_GRAYED              0x00002000  // grays and disables
#define QMF_INACTIVE            0x00004000  // disables any input
#define QMF_NODEFAULTINIT       0x00008000  // skip default initialization
#define QMF_OWNERDRAW           0x00010000
#define QMF_PULSE               0x00020000
#define QMF_LOWERCASE           0x00040000  // edit field is all lower case
#define QMF_UPPERCASE           0x00080000  // edit field is all upper case
#define QMF_SILENT              0x00100000

// callback notifications
#define QM_GOTFOCUS             1
#define QM_LOSTFOCUS            2
#define QM_ACTIVATED            3

typedef struct _tag_menuframework
{
	int cursor;
	int cursor_prev;

	int nitems;
	void *items[MAX_MENUITEMS];

	void ( *draw )( void );
	sfxHandle_t ( *key )( int key );

	qboolean wrapAround;
	qboolean fullscreen;
	qboolean showlogo;

	// JOSEPH 11-9-99
	int specialmenutype;
	// END JOSEPH
} menuframework_s;

typedef struct
{
	int type;
	const char *name;
	int id;
	int x, y;
	int left;
	int top;
	int right;
	int bottom;
	menuframework_s *parent;
	int menuPosition;
	unsigned flags;

	void ( *callback )( void *self, int event );
	void ( *statusbar )( void *self );
	void ( *ownerdraw )( void *self );
} menucommon_s;

typedef struct {
	int cursor;
	int scroll;
	int widthInChars;
	char buffer[MAX_EDIT_LINE];
	int maxchars;
} mfield_t;

typedef struct
{
	menucommon_s generic;
	mfield_t field;
} menufield_s;

typedef struct
{
	menucommon_s generic;

	float minvalue;
	float maxvalue;
	float curvalue;

	float range;
} menuslider_s;

typedef struct
{
	menucommon_s generic;

	int oldvalue;
	int curvalue;
	int numitems;
	int top;

	const char **itemnames;

	int width;
	int height;
	int columns;
	int seperation;
} menulist_s;

typedef struct
{
	menucommon_s generic;
} menuaction_s;

typedef struct
{
	menucommon_s generic;
	int curvalue;
} menuradiobutton_s;

typedef struct
{
	menucommon_s generic;
	char*           focuspic;
	char*           errorpic;
	qhandle_t shader;
	qhandle_t focusshader;
	int width;
	int height;
	float*          focuscolor;
} menubitmap_s;

typedef struct
{
	menucommon_s generic;
	char*           string;
	int style;
	float*          color;
} menutext_s;

extern void         Menu_Cache( void );
extern void         Menu_Focus( menucommon_s *m );
extern void         Menu_AddItem( menuframework_s *menu, void *item );
extern void         Menu_AdjustCursor( menuframework_s *menu, int dir );
extern void         Menu_Draw( menuframework_s *menu );
// JOSEPH 11-9-99
extern void         Menu_Draw_Inactive( menuframework_s *menu );
// END JOSEPH
extern void         *Menu_ItemAtCursor( menuframework_s *m );
extern sfxHandle_t  Menu_ActivateItem( menuframework_s *s, menucommon_s* item );
extern void         Menu_SetCursor( menuframework_s *s, int cursor );
extern void         Menu_SetCursorToItem( menuframework_s *m, void* ptr );
extern sfxHandle_t  Menu_DefaultKey( menuframework_s *s, int key );
extern void         Bitmap_Init( menubitmap_s *b );
extern void         Bitmap_Draw( menubitmap_s *b );
extern void         ScrollList_Draw( menulist_s *l );
// JOSEPH 11-23-99
extern void         ScrollList_Draw2( menulist_s *l );
// END JOSEPH
extern sfxHandle_t  ScrollList_Key( menulist_s *l, int key );
extern sfxHandle_t menu_in_sound;
extern sfxHandle_t menu_move_sound;
extern sfxHandle_t menu_out_sound;
extern sfxHandle_t menu_buzz_sound;
extern sfxHandle_t menu_null_sound;
extern vec4_t menu_text_color;
extern vec4_t menu_grayed_color;
extern vec4_t menu_dark_color;
extern vec4_t menu_highlight_color;
extern vec4_t menu_red_color;
extern vec4_t menu_black_color;
extern vec4_t menu_dim_color;
extern vec4_t color_black;
// JOSEPH 11-29-99
extern vec4_t color_halfblack;
// END JOSEPH
extern vec4_t color_white;
extern vec4_t color_yellow;
extern vec4_t color_blue;
extern vec4_t color_orange;
extern vec4_t color_red;
extern vec4_t color_dim;
extern vec4_t name_color;
extern vec4_t list_color;
extern vec4_t listbar_color;
// JOSEPH 11-23-99
extern vec4_t listbar_color2;
// END JOSEPH
extern vec4_t text_color_disabled;
extern vec4_t text_color_normal;
extern vec4_t text_color_highlight;

extern char *ui_medalNames[];
extern char *ui_medalPicNames[];
extern char *ui_medalSounds[];

//
// ui_mfield.c
//
extern void         MField_Clear( mfield_t *edit );
extern void         MField_KeyDownEvent( mfield_t *edit, int key );
extern void         MField_CharEvent( mfield_t *edit, int ch );
extern void         MField_Draw( mfield_t *edit, int x, int y, int style, vec4_t color );
// JOSEPH 11-23-99
extern void         MenuField_Draw2( menufield_s *f, int specialtype );
// END JOSEPH
extern void         MenuField_Init( menufield_s* m );
extern void         MenuField_Draw( menufield_s *f );
extern sfxHandle_t  MenuField_Key( menufield_s* m, int* key );

//
// ui_main.c
//
void            UI_Report();
void            UI_Load();
void            UI_LoadMenus( const char *menuFile, qboolean reset );
void            UI_SetActiveMenu( uiMenuCommand_t menu );
uiMenuCommand_t UI_GetActiveMenu( void );
int             UI_AdjustTimeByGame( int time );
void            UI_ShowPostGame( qboolean newHigh );
void            UI_ClearScores();
void            UI_LoadArenas( void );

//
// ui_menu.c
//
extern void MainMenu_Cache( void );
extern void UI_MainMenu( void );
extern void UI_RegisterCvars( void );
extern void UI_UpdateCvars( void );

//
// ui_credits.c
//
extern void UI_CreditMenu( void );

//
// ui_ingame.c
//
extern void InGame_Cache( void );
extern void UI_InGameMenu( void );

//
// ui_confirm.c
//
extern void ConfirmMenu_Cache( void );
extern void UI_ConfirmMenu( const char *question, void ( *draw )( void ), void ( *action )( qboolean result ) );

//
// ui_setup.c
//
extern void UI_SetupMenu_Cache( void );
extern void UI_SetupMenu( void );

//
// ui_team.c
//
extern void UI_TeamMainMenu( void );
extern void TeamMain_Cache( void );

//
// ui_connect.c
//
extern void UI_DrawConnectScreen( qboolean overlay );

//
// ui_controls2.c
//
extern void UI_ControlsMenu( void );
extern void Controls_Cache( void );

//
// ui_demo2.c
//
extern void UI_DemosMenu( void );
extern void Demos_Cache( void );

//
// ui_cinematics.c
//
extern void UI_CinematicsMenu( void );
extern void UI_CinematicsMenu_f( void );
extern void UI_CinematicsMenu_Cache( void );

//
// ui_cdkey.c
//
extern void UI_CDKeyMenu( void );
extern void UI_CDKeyMenu_Cache( void );
extern void UI_CDKeyMenu_f( void );

//
// ui_playermodel.c
//
extern void UI_PlayerModelMenu( void );
extern void PlayerModel_Cache( void );

//
// ui_playersettings.c
//
extern void UI_PlayerSettingsMenu( void );
extern void PlayerSettings_Cache( void );

//
// ui_preferences.c
//
extern void UI_PreferencesMenu( void );
extern void Preferences_Cache( void );

//
// ui_specifyserver.c
//
extern void UI_SpecifyServerMenu( void );
extern void SpecifyServer_Cache( void );

//
// ui_servers2.c
//
#define MAX_FAVORITESERVERS 16

extern void UI_ArenaServersMenu( void );
extern void ArenaServers_Cache( void );

//
// ui_startserver.c
//
extern void UI_StartServerMenu( qboolean multiplayer );
extern void StartServer_Cache( void );
extern void ServerOptions_Cache( void );
extern void UI_BotSelectMenu( char *bot );
extern void UI_BotSelectMenu_Cache( void );

//
// ui_serverinfo.c
//
extern void UI_ServerInfoMenu( void );
extern void ServerInfo_Cache( void );

//
// ui_video.c
//
extern void UI_GraphicsOptionsMenu( void );
extern void GraphicsOptions_Cache( void );
extern void DriverInfo_Cache( void );

//
// ui_players.c
//

typedef struct {
	// model info
	qhandle_t legsModel;
	qhandle_t legsSkin;
	lerpFrame_t legs;

	qhandle_t torsoModel;
	qhandle_t torsoSkin;
	lerpFrame_t torso;

	qhandle_t headModel;
	qhandle_t headSkin;

	animation_t animations[MAX_ANIMATIONS];

	qhandle_t weaponModel;
	qhandle_t barrelModel;
	qhandle_t flashModel;
	vec3_t flashDlightColor;
	int muzzleFlashTime;

	// currently in use drawing parms
	vec3_t viewAngles;
	vec3_t moveAngles;
	weapon_t currentWeapon;
	int legsAnim;
	int torsoAnim;

	// animation vars
	weapon_t weapon;
	weapon_t lastWeapon;
	weapon_t pendingWeapon;
	int weaponTimer;
	int pendingLegsAnim;
	int torsoAnimationTimer;

	int pendingTorsoAnim;
	int legsAnimationTimer;

	qboolean chat;
	qboolean newModel;

	qboolean barrelSpinning;
	float barrelAngle;
	int barrelTime;

	int realWeapon;

	// NERVE - SMF - added fields so it will work with wolf's skeletal animation system
	// parsed from the start of the cfg file
	gender_t gender;
	footstep_t footsteps;
	vec3_t headOffset;
	int version;
	qboolean isSkeletal;
	int numAnimations;

	qhandle_t backpackModel;
	qhandle_t helmetModel;
	// -NERVE - SMF
} playerInfo_t;

void UI_DrawPlayer( float x, float y, float w, float h, playerInfo_t *pi, int time );
void UI_PlayerInfo_SetModel( playerInfo_t *pi, const char *model );
void UI_PlayerInfo_SetInfo( playerInfo_t *pi, int legsAnim, int torsoAnim, vec3_t viewAngles, vec3_t moveAngles, weapon_t weaponNum, qboolean chat );
qboolean UI_RegisterClientModelname( playerInfo_t *pi, const char *modelSkinName );

//
// ui_atoms.c
//
typedef struct {
	int frametime;
	int realtime;
	int cursorx;
	int cursory;
	int menusp;
	menuframework_s*    activemenu;
	menuframework_s*    stack[MAX_MENUDEPTH];
	glconfig_t glconfig;
	qboolean debug;
	qhandle_t whiteShader;
	qhandle_t menuBackShader;
	qhandle_t menuBackNoLogoShader;
	qhandle_t charset;
	qhandle_t charsetProp;
	qhandle_t charsetPropGlow;
	qhandle_t charsetPropB;
	qhandle_t cursor;
	qhandle_t rb_on;
	qhandle_t rb_off;
	// JOSEPH 11-9-99
	qhandle_t menu;
	qhandle_t menu1a;
	qhandle_t menu1b;
	qhandle_t menu2a;
	qhandle_t menu2b;
	qhandle_t menuchars;
	// END JOSEPH
	float scale;
	float bias;
	qboolean demoversion;
	qboolean firstdraw;
} uiStatic_t;

// new ui stuff
#define UI_NUMFX 7
#define MAX_HEADS 64
#define MAX_ALIASES 64
#define MAX_HEADNAME  32
#define MAX_TEAMS 64
#define MAX_GAMETYPES 16
#define MAX_MAPS 128
#define MAX_SPMAPS 16
#define PLAYERS_PER_TEAM 5
#define MAX_PINGREQUESTS        16
#define MAX_ADDRESSLENGTH       64
#define MAX_HOSTNAMELENGTH      22
#define MAX_MAPNAMELENGTH       16
#define MAX_STATUSLENGTH        64
#define MAX_LISTBOXWIDTH        59
#define UI_FONT_THRESHOLD       0.1
#define MAX_DISPLAY_SERVERS     2048
#define MAX_DISPLAY_SAVEGAMES   256
#define MAX_SERVERSTATUS_LINES  128
#define MAX_SERVERSTATUS_TEXT   1024
#define MAX_FOUNDPLAYER_SERVERS 16
#define TEAM_MEMBERS 5
#define GAMES_ALL           0
#define GAMES_FFA           1
#define GAMES_TEAMPLAY      2
#define GAMES_TOURNEY       3
#define GAMES_CTF           4
#define MAPS_PER_TIER 3
#define MAX_TIERS 16
#define MAX_MODS 64
#define MAX_DEMOS 256
#define MAX_MOVIES 256
#define MAX_PLAYERMODELS 256
#define MAX_SAVEGAMES 256
#define MAX_SPAWNPOINTS 128     // NERVE - SMF
#define MAX_SPAWNDESC   128     // NERVE - SMF

typedef struct {
	const char *name;
	const char *imageName;
	qhandle_t headImage;
	qboolean female;
} characterInfo;

//----(SA)	added
typedef struct {
	const char *savegameFile;       // mysave[.svg]
	const char *savegameName;       // "beginning of game (SA)"
	const char *imageName;          //
	const char *mapName;            // escape1
	const char *savegameInfoText;   // "[mapname]/n[attempts]/n[health]/n[etc]"	 loaded from savegame file
	qhandle_t sshotImage;           // handle to 'levelshots/<mapName>.tga'

	int episode;                    // 1
//	int			time;
//	int			date;
	const char *time;               // formatted time string
	const char *date;

	qtime_t tm;

} savegameInfo;
//----(SA)	end

typedef struct {
	const char *name;
	const char *ai;
	const char *action;
} aliasInfo;

typedef struct {
	const char *teamName;
	const char *imageName;
	const char *teamMembers[TEAM_MEMBERS];
	qhandle_t teamIcon;
	qhandle_t teamIcon_Metal;
	qhandle_t teamIcon_Name;
	int cinematic;
} teamInfo;

typedef struct {
	const char *gameType;
	int gtEnum;
} gameTypeInfo;

typedef struct {
	const char *mapName;
	const char *mapLoadName;
	const char *imageName;
	const char *opponentName;
	int teamMembers;
	int typeBits;
	int cinematic;
	int timeToBeat[MAX_GAMETYPES];
	qhandle_t levelShot;
	qboolean active;
} mapInfo;

typedef struct {
	const char *tierName;
	const char *maps[MAPS_PER_TIER];
	int gameTypes[MAPS_PER_TIER];
	qhandle_t mapHandles[MAPS_PER_TIER];
} tierInfo;

typedef struct serverFilter_s {
	const char *description;
	const char *basedir;
} serverFilter_t;

typedef struct {
	char adrstr[MAX_ADDRESSLENGTH];
	int start;
} pinglist_t;


typedef struct serverStatus_s {
	pinglist_t pingList[MAX_PINGREQUESTS];
	int numqueriedservers;
	int currentping;
	int nextpingtime;
	int maxservers;
	int refreshtime;
	int numServers;
	int sortKey;
	int sortDir;
	int lastCount;
	qboolean refreshActive;
	int currentServer;
	int displayServers[MAX_DISPLAY_SERVERS];
	int numDisplayServers;
	int numPlayersOnServers;
	int nextDisplayRefresh;
	int nextSortTime;
	qhandle_t currentServerPreview;
	int currentServerCinematic;
	int motdLen;
	int motdWidth;
	int motdPaintX;
	int motdPaintX2;
	int motdOffset;
	int motdTime;
	char motd[MAX_STRING_CHARS];
} serverStatus_t;


typedef struct savegameStatus_s {

	int sortKey;
	int sortDir;
	int displaySavegames[MAX_DISPLAY_SAVEGAMES];
//	int				numDisplaySavegames;

} savegameStatus_t;


typedef struct {
	char adrstr[MAX_ADDRESSLENGTH];
	char name[MAX_ADDRESSLENGTH];
	int startTime;
	int serverNum;
	qboolean valid;
} pendingServer_t;

typedef struct {
	int num;
	pendingServer_t server[MAX_SERVERSTATUSREQUESTS];
} pendingServerStatus_t;

typedef struct {
	char address[MAX_ADDRESSLENGTH];
	char *lines[MAX_SERVERSTATUS_LINES][4];
	char text[MAX_SERVERSTATUS_TEXT];
	char pings[MAX_CLIENTS * 3];
	int numLines;
} serverStatusInfo_t;

typedef struct {
	const char *modName;
	const char *modDescr;
} modInfo_t;

typedef struct {
	displayContextDef_t uiDC;
	int newHighScoreTime;
	int newBestTime;
	int showPostGameTime;
	qboolean newHighScore;
	qboolean demoAvailable;
	qboolean soundHighScore;

	int characterCount;
	int botIndex;
	characterInfo characterList[MAX_HEADS];

	int aliasCount;
	aliasInfo aliasList[MAX_ALIASES];

	int teamCount;
	teamInfo teamList[MAX_TEAMS];

	int numGameTypes;
	gameTypeInfo gameTypes[MAX_GAMETYPES];

	int numJoinGameTypes;
	gameTypeInfo joinGameTypes[MAX_GAMETYPES];

	int redBlue;
	int playerCount;
	int myTeamCount;
	int teamIndex;
	int playerRefresh;
	int playerIndex;
	int playerNumber;
	qboolean teamLeader;
	char playerNames[MAX_CLIENTS][MAX_NAME_LENGTH];
	char teamNames[MAX_CLIENTS][MAX_NAME_LENGTH];
	int teamClientNums[MAX_CLIENTS];

	int mapCount;
	mapInfo mapList[MAX_MAPS];


	int tierCount;
	tierInfo tierList[MAX_TIERS];

	int skillIndex;

	modInfo_t modList[MAX_MODS];
	int modCount;
	int modIndex;

	const char *demoList[MAX_DEMOS];
	int demoCount;
	int demoIndex;

	const char *movieList[MAX_MOVIES];
	int movieCount;
	int movieIndex;
	int previewMovie;

//----(SA)	added
//	const char			*savegameList[MAX_SAVEGAMES];
	savegameInfo savegameList[MAX_SAVEGAMES];
	int savegameCount;
//	int					savegameIndex;
	savegameStatus_t savegameStatus;
//----(SA)	end

	serverStatus_t serverStatus;

	// for the showing the status of a server
	char serverStatusAddress[MAX_ADDRESSLENGTH];
	serverStatusInfo_t serverStatusInfo;
	int nextServerStatusRefresh;

	// to retrieve the status of server to find a player
	pendingServerStatus_t pendingServerStatus;
	char findPlayerName[MAX_STRING_CHARS];
	char foundPlayerServerAddresses[MAX_FOUNDPLAYER_SERVERS][MAX_ADDRESSLENGTH];
	char foundPlayerServerNames[MAX_FOUNDPLAYER_SERVERS][MAX_ADDRESSLENGTH];
	int currentFoundPlayerServer;
	int numFoundPlayerServers;
	int nextFindPlayerRefresh;

	int currentCrosshair;
	int startPostGameTime;
	sfxHandle_t newHighScoreSound;

	int q3HeadCount;
	char q3HeadNames[MAX_PLAYERMODELS][64];
	qhandle_t q3HeadIcons[MAX_PLAYERMODELS];
	int q3SelectedHead;

	int effectsColor;

	qboolean inGameLoad;

	// NERVE - SMF
	char spawnPoints[MAX_SPAWNPOINTS][MAX_SPAWNDESC];
	int spawnCount;
	// -NERVE - SMF

}   uiInfo_t;

extern uiInfo_t uiInfo;


extern void         UI_Init( void );
extern void         UI_Shutdown( void );
extern void         UI_MouseEvent( int dx, int dy );
extern void         UI_Refresh( int realtime );
extern qboolean     UI_ConsoleCommand( int realTime );
extern float        UI_ClampCvar( float min, float max, float value );
extern void         UI_DrawNamedPic( float x, float y, float width, float height, const char *picname );
extern void         UI_DrawHandlePic( float x, float y, float w, float h, qhandle_t hShader );
extern void         UI_FillRect( float x, float y, float width, float height, const float *color );
extern void 		UI_DrawRect( float x, float y, float width, float height, float size, const float *color );

extern void         UI_UpdateScreen( void );
extern void         UI_SetColor( const float *rgba );
extern void         UI_LerpColor( vec4_t a, vec4_t b, vec4_t c, float t );
extern void         UI_DrawBannerString( int x, int y, const char* str, int style, vec4_t color );
extern float        UI_ProportionalSizeScale( int style );
extern void         UI_DrawProportionalString( int x, int y, const char* str, int style, vec4_t color );
extern int          UI_ProportionalStringWidth( const char* str );
extern void         UI_DrawString( int x, int y, const char* str, int style, vec4_t color );
extern void         UI_DrawChar( int x, int y, int ch, int style, vec4_t color );
extern qboolean     UI_CursorInRect( int x, int y, int width, int height );
extern void         UI_AdjustFrom640( float *x, float *y, float *w, float *h );
extern void         UI_DrawTextBox( int x, int y, int width, int lines );
extern qboolean     UI_IsFullscreen( void );
extern void         UI_SetActiveMenu( uiMenuCommand_t menu );
extern void         UI_PushMenu( menuframework_s *menu );
extern void         UI_PopMenu( void );
extern void         UI_ForceMenuOff( void );
extern char         *UI_Argv( int arg );
extern char         *UI_Cvar_VariableString( const char *var_name );
extern void         UI_Refresh( int time );

extern void         UI_StartDemoLoop( void );
void                UI_LoadBestScores( const char *map, int game );           // NERVE - SMF
extern qboolean m_entersound;
extern uiStatic_t uis;

//
// ui_spLevel.c
//
void UI_SPLevelMenu_Cache( void );
void UI_SPLevelMenu( void );
void UI_SPLevelMenu_f( void );
void UI_SPLevelMenu_ReInit( void );

//
// ui_spArena.c
//
void UI_SPArena_Start( const char *arenaInfo );

//
// ui_spPostgame.c
//
void UI_SPPostgameMenu_Cache( void );
void UI_SPPostgameMenu_f( void );

//
// ui_spSkill.c
//
void UI_SPSkillMenu( const char *arenaInfo );
void UI_SPSkillMenu_Cache( void );

//
// ui_syscalls.c
//
void            trap_UI_Print( const char *string );
void            trap_UI_Error( const char *string );
int             trap_Milliseconds( void );
void            trap_Cvar_Register( vmCvar_t *vmCvar, const char *varName, const char *defaultValue, int flags );
void            Cvar_Update( vmCvar_t *vmCvar );
void            trap_Cvar_Set( const char *var_name, const char *value );
float           trap_Cvar_VariableValue( const char *var_name );
void            trap_Cvar_VariableStringBuffer( const char *var_name, char *buffer, int bufsize );
void            trap_Cvar_SetValue( const char *var_name, float value );
void            trap_Cvar_Reset( const char *name );
void            trap_Cvar_Create( const char *var_name, const char *var_value, int flags );
void            trap_Cvar_InfoStringBuffer( int bit, char *buffer, int bufsize );
int             trap_Argc( void );
void            trap_Argv( int n, char *buffer, int bufferLength );
void            trap_Cmd_ExecuteText( int exec_when, const char *text );    // don't use EXEC_NOW!
int             trap_FS_FOpenFile( const char *qpath, fileHandle_t *f, fsMode_t mode );
void            trap_FS_Read( void *buffer, int len, fileHandle_t f );
void            trap_FS_Seek( fileHandle_t f, long offset, int origin ); //----(SA)	added
int            trap_FS_Write( const void *buffer, int len, fileHandle_t f );
void            trap_FS_FCloseFile( fileHandle_t f );
int             trap_FS_GetFileList(  const char *path, const char *extension, char *listbuf, int bufsize );
int             trap_FS_Delete( const char *filename );
qhandle_t       trap_UI_RegisterModel( const char *name );
qhandle_t       trap_UI_RegisterSkin( const char *name );
qhandle_t       RE_RegisterShaderNoMip( const char *name );
void            trap_R_ClearScene( void );
void            trap_R_AddRefEntityToScene( const refEntity_t *re );
void            trap_R_AddPolyToScene( qhandle_t hShader, int numVerts, const polyVert_t *verts );
void            trap_R_AddLightToScene( const vec3_t org, float intensity, float r, float g, float b, int overdraw );
void            trap_R_AddCoronaToScene( const vec3_t org, float r, float g, float b, float scale, int id, int flags );
void            trap_R_RenderScene( const refdef_t *fd );
void            trap_R_SetColor( const float *rgba );
void            trap_R_DrawStretchPic( float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t hShader );
void            trap_R_ModelBounds( clipHandle_t model, vec3_t mins, vec3_t maxs );
void            trap_UpdateScreen( void );
int             trap_CM_LerpTag( orientation_t *tag, const refEntity_t *refent, const char *tagName, int startIndex );
void            trap_S_StartLocalSound( sfxHandle_t sfx, int channelNum );
sfxHandle_t     trap_UI_S_RegisterSound( const char *sample );
void            trap_S_FadeAllSound( float targetvol, int time ); //----(SA)	added
void            trap_Key_KeynumToStringBuf( int keynum, char *buf, int buflen );
void            trap_Key_GetBindingBuf( int keynum, char *buf, int buflen );
void            trap_Key_SetBinding( int keynum, const char *binding );
qboolean        trap_Key_IsDown( int keynum );
qboolean        trap_Key_GetOverstrikeMode( void );
void            trap_Key_SetOverstrikeMode( qboolean state );
void            trap_Key_ClearStates( void );
int             trap_Key_GetCatcher( void );
void            trap_Key_SetCatcher( int catcher );
void            trap_GetClipboardData( char *buf, int bufsize );
void            GetClientState( uiClientState_t *state );
int             GetConfigString( int index, char* buff, int buffsize );
void            trap_GetClientState( uiClientState_t *state );	// TODO: Remove
int             trap_GetConfigString( int index, char* buff, int buffsize );	// TODO: Remove
int             trap_LAN_GetServerCount( int source );          // NERVE - SMF

int             trap_MemoryRemaining( void );

// NERVE - SMF - multiplayer traps
void            trap_LAN_MarkServerVisible( int source, int n, qboolean visible );
void            trap_LAN_ResetPings( int n );
void            trap_LAN_SaveCachedServers();
int             trap_LAN_CompareServers( int source, int sortKey, int sortDir, int s1, int s2 );
void            trap_LAN_GetServerAddressString( int source, int n, char *buf, int buflen );
void trap_LAN_GetServerInfo( int source, int n, char *buf, int buflen );
int             trap_LAN_AddServer( int source, const char *name, const char *addr );
void            trap_LAN_RemoveServer( int source, const char *addr );
int             trap_LAN_GetServerPing( int source, int n );
int             trap_LAN_ServerIsVisible( int source, int n );
int             trap_LAN_ServerStatus( const char *serverAddress, char *serverStatus, int maxLen );
void            trap_LAN_SaveCachedServers();
void            trap_LAN_LoadCachedServers();
void            trap_LAN_MarkServerVisible( int source, int n, qboolean visible );
// -NERVE - SMF

void            trap_GetCDKey( char *buf, int buflen );
void            trap_SetCDKey( char *buf );
void            trap_R_RegisterFont( const char *pFontname, int pointSize, fontInfo_t *font );
void            trap_S_StopBackgroundTrack( void );
void            trap_S_StartBackgroundTrack( const char *intro, const char *loop, int fadeupTime );
int             trap_CIN_PlayCinematic( const char *arg0, int xpos, int ypos, int width, int height, int bits );
e_status        trap_CIN_StopCinematic( int handle );
e_status        trap_CIN_RunCinematic( int handle );
void            trap_CIN_DrawCinematic( int handle );
void            trap_UI_CIN_SetExtents( int handle, int x, int y, int w, int h );
int             trap_RealTime( qtime_t *qtime );
void            trap_R_RemapShader( const char *oldShader, const char *newShader, const char *timeOffset );

qboolean        trap_GetLimboString( int index, char *buf );            // NERVE - SMF

//
// ui_addbots.c
//
void UI_AddBots_Cache( void );
void UI_AddBotsMenu( void );

//
// ui_removebots.c
//
void UI_RemoveBots_Cache( void );
void UI_RemoveBotsMenu( void );

//
// ui_teamorders.c
//
extern void UI_TeamOrdersMenu( void );
extern void UI_TeamOrdersMenu_f( void );
extern void UI_TeamOrdersMenu_Cache( void );

//
// ui_loadconfig.c
//
void UI_LoadConfig_Cache( void );
void UI_LoadConfigMenu( void );

//
// ui_saveconfig.c
//
void UI_SaveConfigMenu_Cache( void );
void UI_SaveConfigMenu( void );

//
// ui_display.c
//
void UI_DisplayOptionsMenu_Cache( void );
void UI_DisplayOptionsMenu( void );

//
// ui_sound.c
//
void UI_SoundOptionsMenu_Cache( void );
void UI_SoundOptionsMenu( void );

//
// ui_network.c
//
void UI_NetworkOptionsMenu_Cache( void );
void UI_NetworkOptionsMenu( void );

//
// ui_gameinfo.c
//
typedef enum {
	AWARD_ACCURACY,
	AWARD_IMPRESSIVE,
	AWARD_EXCELLENT,
	AWARD_GAUNTLET,
	AWARD_FRAGS,
	AWARD_PERFECT
} awardType_t;

const char *UI_GetArenaInfoByNumber( int num );
const char *UI_GetArenaInfoByMap( const char *map );
const char *UI_GetSpecialArenaInfo( const char *tag );
int UI_GetNumArenas( void );
int UI_GetNumSPArenas( void );
int UI_GetNumSPTiers( void );

char *UI_GetBotInfoByNumber( int num );
char *UI_GetBotInfoByName( const char *name );
int UI_GetNumBots( void );

void UI_GetBestScore( int level, int *score, int *skill );
void UI_SetBestScore( int level, int score );
int UI_TierCompleted( int levelWon );
qboolean UI_ShowTierVideo( int tier );
qboolean UI_CanShowTierVideo( int tier );
int  UI_GetCurrentGame( void );
void UI_NewGame( void );
void UI_LogAwardData( int award, int data );
int UI_GetAwardLevel( int award );

void UI_SPUnlock_f( void );
void UI_SPUnlockMedals_f( void );

void UI_InitGameinfo( void );

//
// ui_main.c
//
void UI_OwnerDraw( float x, float y, float w, float h, float text_x, float text_y, int ownerDraw, int ownerDrawFlags, int align, float special, int font, float scale, vec4_t color, qhandle_t shader, int textStyle );
