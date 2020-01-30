#pragma once
#include "Engine/Core/EngineCommon.hpp"

constexpr float	WORLD_WIDTH = 200.f;
constexpr float	WORLD_HEIGHT = 100.f;
constexpr float WORLD_CENTER_X = WORLD_WIDTH / 2.f;
constexpr float	WORLD_CENTER_Y = WORLD_HEIGHT / 2.f;

constexpr unsigned int RESOLUTION_WIDTH = 1350u;
constexpr unsigned int RESOLUTION_HEIGHT = 900u;

#if !defined(UNIT_TEST_LEVEL)
#if defined(_DEBUG)
#define UNIT_TEST_LEVEL 5
#else
#define UNIT_TEST_LEVEL 1000
#endif
#endif