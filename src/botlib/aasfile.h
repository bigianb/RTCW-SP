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

//NOTE:	int = default signed
//					default long

#define AASID                       ( ( 'S' << 24 ) + ( 'A' << 16 ) + ( 'A' << 8 ) + 'E' )
#define AASVERSION                  8

//presence types
#define PRESENCE_NONE               1
#define PRESENCE_NORMAL             2
#define PRESENCE_CROUCH             4

//travel types
#define MAX_TRAVELTYPES             32
#define TRAVEL_INVALID              1       //temporary not possible
#define TRAVEL_WALK                 2       //walking
#define TRAVEL_CROUCH               3       //crouching
#define TRAVEL_BARRIERJUMP          4       //jumping onto a barrier
#define TRAVEL_JUMP                 5       //jumping
#define TRAVEL_LADDER               6       //climbing a ladder
#define TRAVEL_WALKOFFLEDGE         7       //walking of a ledge
#define TRAVEL_SWIM                 8       //swimming
#define TRAVEL_WATERJUMP            9       //jump out of the water
#define TRAVEL_TELEPORT             10      //teleportation
#define TRAVEL_ELEVATOR             11      //travel by elevator
#define TRAVEL_ROCKETJUMP           12      //rocket jumping required for travel
#define TRAVEL_BFGJUMP              13      //bfg jumping required for travel
#define TRAVEL_GRAPPLEHOOK          14      //grappling hook required for travel
#define TRAVEL_DOUBLEJUMP           15      //double jump
#define TRAVEL_RAMPJUMP             16      //ramp jump
#define TRAVEL_STRAFEJUMP           17      //strafe jump
#define TRAVEL_JUMPPAD              18      //jump pad
#define TRAVEL_FUNCBOB              19      //func bob

//additional travel flags
#define TRAVELTYPE_MASK             0xFFFFFF
#define TRAVELFLAG_NOTTEAM1         ( 1 << 24 )
#define TRAVELFLAG_NOTTEAM2         ( 2 << 24 )

//face flags
#define FACE_SOLID                  1       //just solid at the other side
#define FACE_LADDER                 2       //ladder
#define FACE_GROUND                 4       //standing on ground when in this face
#define FACE_GAP                    8       //gap in the ground
#define FACE_LIQUID                 16
#define FACE_LIQUIDSURFACE          32

//area contents
#define AREACONTENTS_WATER              1
#define AREACONTENTS_LAVA               2
#define AREACONTENTS_SLIME              4
#define AREACONTENTS_CLUSTERPORTAL      8
#define AREACONTENTS_TELEPORTAL         16
#define AREACONTENTS_ROUTEPORTAL        32
#define AREACONTENTS_TELEPORTER         64
#define AREACONTENTS_JUMPPAD            128
#define AREACONTENTS_DONOTENTER         256
#define AREACONTENTS_VIEWPORTAL         512
// Rafael - nopass
#define AREACONTENTS_DONOTENTER_LARGE   1024
#define AREACONTENTS_MOVER              2048

//number of model of the mover inside this area
#define AREACONTENTS_MODELNUMSHIFT      24
#define AREACONTENTS_MAXMODELNUM        0xFF
#define AREACONTENTS_MODELNUM           ( AREACONTENTS_MAXMODELNUM << AREACONTENTS_MODELNUMSHIFT )

//area flags
#define AREA_GROUNDED               1       //bot can stand on the ground
#define AREA_LADDER                 2       //area contains one or more ladder faces
#define AREA_LIQUID                 4       //area contains a liquid
// Ridah
#define AREA_DISABLED               8
#define AREA_USEFORROUTING          1024

//aas file header lumps
#define AAS_LUMPS                   14
#define AASLUMP_BBOXES              0
#define AASLUMP_VERTEXES            1
#define AASLUMP_PLANES              2
#define AASLUMP_EDGES               3
#define AASLUMP_EDGEINDEX           4
#define AASLUMP_FACES               5
#define AASLUMP_FACEINDEX           6
#define AASLUMP_AREAS               7
#define AASLUMP_AREASETTINGS        8
#define AASLUMP_REACHABILITY        9
#define AASLUMP_NODES               10
#define AASLUMP_PORTALS             11
#define AASLUMP_PORTALINDEX         12
#define AASLUMP_CLUSTERS            13

//========== bounding box =========

//bounding box
typedef struct aas_bbox_s
{
	int presencetype;
	int flags;
	vec3_t mins, maxs;
} aas_bbox_t;

//============ settings ===========

//reachability to another area
typedef struct aas_reachability_s
{
	int areanum;                        //number of the reachable area
	int facenum;                        //number of the face towards the other area
	int edgenum;                        //number of the edge towards the other area
	vec3_t start;                       //start point of inter area movement
	vec3_t end;                         //end point of inter area movement
	int traveltype;                 //type of travel required to get to the area
	unsigned short int traveltime; //travel time of the inter area movement
} aas_reachability_t;

//area settings
typedef struct aas_areasettings_s
{
	//could also add all kind of statistic fields
	int contents;                       //contents of the convex area
	int areaflags;                      //several area flags
	int presencetype;                   //how a bot can be present in this convex area
	int cluster;                        //cluster the area belongs to, if negative it's a portal
	int clusterareanum;             //number of the area in the cluster
	int numreachableareas;          //number of reachable areas from this one
	int firstreachablearea;         //first reachable area in the reachable area index
	// Ridah, add a ground steepness stat, so we can avoid terrain when we can take a close-by flat route
	float groundsteepness;          // 0 = flat, 1 = steep
} aas_areasettings_t;

//cluster portal
typedef struct aas_portal_s
{
	int areanum;                        //area that is the actual portal
	int frontcluster;                   //cluster at front of portal
	int backcluster;                    //cluster at back of portal
	int clusterareanum[2];          //number of the area in the front and back cluster
} aas_portal_t;

//cluster portal index
typedef int aas_portalindex_t;

//cluster
typedef struct aas_cluster_s
{
	int numareas;                       //number of areas in the cluster
	int numreachabilityareas;           //number of areas with reachabilities
	int numportals;                     //number of cluster portals
	int firstportal;                    //first cluster portal in the index
} aas_cluster_t;

//============ 3d definition ============

typedef vec3_t aas_vertex_t;

//just a plane in the third dimension
typedef struct aas_plane_s
{
	vec3_t normal;                      //normal vector of the plane
	float dist;                         //distance of the plane (normal vector * distance = point in plane)
	int type;
} aas_plane_t;

//edge
typedef struct aas_edge_s
{
	int v[2];                           //numbers of the vertexes of this edge
} aas_edge_t;

//edge index, negative if vertexes are reversed
typedef int aas_edgeindex_t;

//a face bounds a convex area, often it will also seperate two convex areas
typedef struct aas_face_s
{
	int planenum;                       //number of the plane this face is in
	int faceflags;                      //face flags (no use to create face settings for just this field)
	int numedges;                       //number of edges in the boundary of the face
	int firstedge;                      //first edge in the edge index
	int frontarea;                      //convex area at the front of this face
	int backarea;                       //convex area at the back of this face
} aas_face_t;

//face index, stores a negative index if backside of face
typedef int aas_faceindex_t;

//convex area with a boundary of faces
typedef struct aas_area_s
{
	int areanum;                        //number of this area
	//3d definition
	int numfaces;                       //number of faces used for the boundary of the convex area
	int firstface;                      //first face in the face index used for the boundary of the convex area
	vec3_t mins;                        //mins of the convex area
	vec3_t maxs;                        //maxs of the convex area
	vec3_t center;                      //'center' of the convex area
} aas_area_t;

//nodes of the bsp tree
typedef struct aas_node_s
{
	int planenum;
	int children[2];                    //child nodes of this node, or convex areas as leaves when negative
										//when a child is zero it's a solid leaf
} aas_node_t;

//=========== aas file ===============

//header lump
typedef struct
{
	int fileofs;
	int filelen;
} aas_lump_t;

//aas file header
typedef struct aas_header_s
{
	int ident;
	int version;
	int bspchecksum;
	//data entries
	aas_lump_t lumps[AAS_LUMPS];
} aas_header_t;


//====== additional information ======
/*

-	when a node child is a solid leaf the node child number is zero
-	two adjacent areas (sharing a plane at opposite sides) share a face
	this face is a portal between the areas
-	when an area uses a face from the faceindex with a positive index
	then the face plane normal points into the area
-	the face edges are stored counter clockwise using the edgeindex
-	two adjacent convex areas (sharing a face) only share One face
	this is a simple result of the areas being convex
-	the convex areas can't have a mixture of ground and gap faces
	other mixtures of faces in one area are allowed
-	areas with the AREACONTENTS_CLUSTERPORTAL in the settings have
	cluster number zero
-	edge zero is a dummy
-	face zero is a dummy
-	area zero is a dummy
-	node zero is a dummy
*/
