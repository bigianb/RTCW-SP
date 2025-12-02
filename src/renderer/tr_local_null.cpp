#include "tr_local.h"

bool GLimp_SpawnRenderThread( void ( *function )( void ) ) {
	return false;
}

void *GLimp_RendererSleep( void ) {
	return NULL;
}

void GLimp_FrontEndSleep( void ) {

}

void GLimp_WakeRenderer( void *data ) {

}
