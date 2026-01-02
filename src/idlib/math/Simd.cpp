/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2012 Robert Beckebans

This file is part of the Doom 3 BFG Edition GPL Source Code ("Doom 3 BFG Edition Source Code").

Doom 3 BFG Edition Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 BFG Edition Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 BFG Edition Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 BFG Edition Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 BFG Edition Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/


#ifdef __APPLE__
	#include <mach/mach_time.h>
#endif

#include "Simd.h"
#include "Simd_Generic.h"
#include "Simd_SSE.h"

idSIMDProcessor*	processor = nullptr;			// pointer to SIMD processor
idSIMDProcessor* 	generic = nullptr;				// pointer to generic SIMD implementation
idSIMDProcessor* 	SIMDProcessor = nullptr;

/*
================
idSIMD::Init
================
*/
void idSIMD::Init()
{
	generic = new idSIMD_Generic;
	generic->cpuid = CPUID_GENERIC;
	processor = nullptr;
	SIMDProcessor = generic;
}

/*
============
idSIMD::InitProcessor
============
*/
void idSIMD::InitProcessor( const char* module, bool forceGeneric )
{
	idSIMDProcessor* newProcessor;

	cpuid_t cpuid = CPUID_GENERIC; //idLib::sys->GetProcessorId();

	if( forceGeneric ){
		newProcessor = generic;
	} else {
		if( processor == nullptr ){
#if defined(USE_INTRINSICS_SSE)
			if( ( cpuid & CPUID_MMX ) && ( cpuid & CPUID_SSE ) )
			{
				processor = new( TAG_MATH ) idSIMD_SSE;
			}
			else
#endif
			{
				processor = generic;
			}
			processor->cpuid = cpuid;
		}

		newProcessor = processor;
	}

	if( newProcessor != SIMDProcessor )
	{
		SIMDProcessor = newProcessor;
//		idLib::common->Printf( "%s using %s for SIMD processing\n", module, SIMDProcessor->GetName() );
	}

	if( cpuid & CPUID_FTZ )
	{
//		idLib::sys->FPU_SetFTZ( true );
//		idLib::common->Printf( "enabled Flush-To-Zero mode\n" );
	}

	if( cpuid & CPUID_DAZ )
	{
//		idLib::sys->FPU_SetDAZ( true );
//		idLib::common->Printf( "enabled Denormals-Are-Zero mode\n" );
	}
}

void idSIMD::Shutdown()
{
	if( processor != generic ) {
		delete processor;
	}
	delete generic;
	generic = nullptr;
	processor = nullptr;
	SIMDProcessor = nullptr;
}
