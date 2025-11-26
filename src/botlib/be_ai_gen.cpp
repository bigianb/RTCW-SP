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


/*****************************************************************************
 * name:		be_ai_gen.c
 *
 * desc:		genetic selection
 *
 *
 *****************************************************************************/

#include "../game/q_shared.h"
#include "l_memory.h"
#include "l_log.h"
#include "l_utils.h"
#include "l_script.h"
#include "l_precomp.h"
#include "l_struct.h"
#include "aasfile.h"
#include "../game/botlib.h"
#include "../game/be_aas.h"
#include "be_aas_funcs.h"
#include "be_interface.h"
#include "../game/be_ai_gen.h"

//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
int GeneticSelection( int numranks, float *rankings ) {
	float sum, select;
	int i, index;

	sum = 0;
	for ( i = 0; i < numranks; i++ )
	{
		if ( rankings[i] < 0 ) {
			continue;
		}
		sum += rankings[i];
	} //end for
	if ( sum > 0 ) {
		//select a bot where the ones with the higest rankings have
		//the highest chance of being selected
		select = random() * sum;
		for ( i = 0; i < numranks; i++ )
		{
			if ( rankings[i] < 0 ) {
				continue;
			}
			sum -= rankings[i];
			if ( sum <= 0 ) {
				return i;
			}
		} //end for
	} //end if
	  //select a bot randomly
	index = random() * numranks;
	for ( i = 0; i < numranks; i++ )
	{
		if ( rankings[index] >= 0 ) {
			return index;
		}
		index = ( index + 1 ) % numranks;
	} //end for
	return 0;
} //end of the function GeneticSelection
