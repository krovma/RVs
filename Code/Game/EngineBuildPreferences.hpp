#pragma once
//-----------------------------------------------------------------------------------------------
// EngineBuildPreferences.hpp
//
// Defines build preferences that the Engine should use when building for this particular game.
//
// Note that this file is an exception to the rule "engine code shall not know about game code".
//	Purpose: Each game can now direct the engine via #defines to build differently for that game.
//	Downside: ALL games must now have this Code/Game/EngineBuildPreferences.hpp file.
//

//#define ENGINE_DISABLE_AUDIO	// (If uncommented) Disables AudioSystem code and fmod linkage.

#define ENGINE_DEBUG_SHADERS	// comment this if shader works well

#define MEM_TRACKING_DISABLE  0
#define MEM_TRACKING_COUNTING 1
#define MEM_TRACKING_VERBOSE  2
#if defined(_DEBUG)
#define MEM_TRACKING MEM_TRACKING_VERBOSE
#else
#define MEM_TRACKING MEM_TRACKING_DISABLE
#endif