#pragma once

#include "../game/q_shared.h"

qboolean loadCamera( int camNum, const char *name );
qboolean getCameraInfo( int camNum, int time, float *origin, float *angles, float *fov );
void startCamera( int camNum, int time );

