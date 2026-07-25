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

class Actor;
class Player;
class Sentient;

void     SpStealth_Init(void);
qboolean SpStealth_Active(void);

void SpStealth_LogConfirm(const char *reason, Actor *actor, Sentient *enemy);

float SpStealth_RearArcDotThreshold(void);
qboolean SpStealth_IsInRearBlindArc(Actor *actor, const vec3_t targetOrigin);
qboolean SpStealth_SoundOriginInRearArc(Actor *actor, const vec3_t soundOrigin);

float SpStealth_VisionScale(void);
float SpStealth_FootstepEventRadius(float baseRadius, Sentient *pedestrian);
float SpStealth_EffectiveNoticeMult(float profileNoticeMult, Sentient *enemy);

void SpStealth_UpdatePlayerStance(Player *player);
void SpStealth_ApplySquadShareAwareness(Actor *receiver, Actor *sharer, Sentient *enemy);
