/*
===========================================================================
Copyright (C) 2026 the OpenMoHAA team

This file is part of OpenMoHAA source code.

OpenMoHAA source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

OpenMoHAA source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with OpenMoHAA source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

#include "sp_population.h"
#include "gamecvars.h"
#include "worldspawn.h"

static cvar_t *g_sp_population_scale;

void SpPopulation_Init(void)
{
    g_sp_population_scale = gi.Cvar_Get("g_sp_population_scale", "1", CVAR_ARCHIVE);
}

float SpPopulation_Scale(void)
{
    if (!g_sp_population_scale) {
        return 1.0f;
    }
    if (g_sp_population_scale->value < 1.0f) {
        return 1.0f;
    }
    if (g_sp_population_scale->value > 2.0f) {
        return 2.0f;
    }
    return g_sp_population_scale->value;
}

void SpPopulation_ApplyWorld(World *levelWorld)
{
    float scale;

    if (!levelWorld || !g_gametype || g_gametype->integer != GT_SINGLE_PLAYER) {
        return;
    }

    scale = SpPopulation_Scale();
    if (scale <= 1.0f) {
        return;
    }

    levelWorld->m_fAIVisionDistance *= scale;
}

float SpPopulation_ScaledChance(float baseChance)
{
    float scale;
    float chance;

    if (!g_gametype || g_gametype->integer != GT_SINGLE_PLAYER) {
        return baseChance;
    }

    scale = SpPopulation_Scale();
    if (scale <= 1.0f || baseChance <= 0.0f) {
        return baseChance;
    }

    chance = baseChance * scale;
    if (chance > 1.0f) {
        chance = 1.0f;
    }
    return chance;
}
