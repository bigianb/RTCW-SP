#pragma once

#include "../game/q_shared.h"

bool loadCamera( int camNum, const char *name );
bool getCameraInfo( int camNum, int time, float *origin, float *angles, float *fov );
void startCamera( int camNum, int time );

