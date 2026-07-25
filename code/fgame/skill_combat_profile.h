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

#pragma once

#include "../qcommon/q_shared.h"

typedef struct skillCombatProfile_s {
    float aimScatterMult;
    float accuracyMult;
    float noticeMult;
    float soundAwarenessMult;
    float grenadeAwarenessMult;
    float coverHoldMult;
    float shareEnemyDelay;
    float stepSideBias;
    float runAndShootStayChance;
    float suppressScatterMult;
} skillCombatProfile_t;

void                          SkillCombatProfile_Init(void);
void                          SkillCombatProfile_SetMap(const char *mapname);
const skillCombatProfile_t   *SkillCombatProfile_Get(void);
qboolean                      SkillCombatProfile_Active(void);
