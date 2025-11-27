#pragma once

typedef enum {
	F_NONE,
	F_STRING,
	F_ENTITY,           // index on disk, pointer in memory
	F_ITEM,             // index on disk, pointer in memory
	F_CLIENT,           // index on disk, pointer in memory
	F_FUNCTION
} saveFieldtype_t;

typedef struct {
	intptr_t ofs;
	saveFieldtype_t type;
} saveField_t;

//.......................................................................................
// this stores all functions in the game code
typedef struct {
	const char *funcStr;
	byte *funcPtr;
} funcList_t;
