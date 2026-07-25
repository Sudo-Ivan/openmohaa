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

#include "sp_stealth.h"
#include "sp_stealth_rear.h"
#include "actor.h"
#include "actorenemy.h"
#include "player.h"
#include "gamecvars.h"
#include "entity.h"
#include "bg_public.h"

#include <math.h>

static cvar_t *g_sp_stealth_debug;
static cvar_t *g_sp_rear_stealth_deg;
static cvar_t *g_sp_vision_scale;

void SpStealth_Init(void)
{
    g_sp_stealth_debug   = gi.Cvar_Get("g_sp_stealth_debug", "0", 0);
    g_sp_rear_stealth_deg = gi.Cvar_Get("g_sp_rear_stealth_deg", "100", CVAR_ARCHIVE);
    g_sp_vision_scale    = gi.Cvar_Get("g_sp_vision_scale", "1", CVAR_ARCHIVE);
}

qboolean SpStealth_Active(void)
{
    return (g_gametype && g_gametype->integer == GT_SINGLE_PLAYER) ? qtrue : qfalse;
}

void SpStealth_LogConfirm(const char *reason, Actor *actor, Sentient *enemy)
{
    if (!SpStealth_Active() || !g_sp_stealth_debug || !g_sp_stealth_debug->integer) {
        return;
    }

    gi.Printf(
        "SP stealth confirm [%s] actor #%i enemy #%i\n",
        reason ? reason : "?",
        actor ? actor->entnum : -1,
        enemy ? enemy->entnum : -1
    );
}

float SpStealth_RearArcDotThreshold(void)
{
    float rearDeg;

    if (!g_sp_rear_stealth_deg) {
        return SpStealth_DotThresholdForRearDeg(100.0f);
    }

    rearDeg = g_sp_rear_stealth_deg->value;
    return SpStealth_DotThresholdForRearDeg(rearDeg);
}

static float SpStealth_ForwardDotToTarget(Actor *actor, const Vector& targetOrigin)
{
    return actor->ForwardDot2DTo(targetOrigin);
}

qboolean SpStealth_IsInRearBlindArc(Actor *actor, const vec3_t targetOrigin)
{
    float dot;
    Vector pos;

    if (!actor) {
        return qfalse;
    }

    pos = targetOrigin;
    dot = SpStealth_ForwardDotToTarget(actor, pos);
    return SpStealth_DotInRearBlindArc(dot, g_sp_rear_stealth_deg ? g_sp_rear_stealth_deg->value : 100.0f);
}

qboolean SpStealth_SoundOriginInRearArc(Actor *actor, const vec3_t soundOrigin)
{
    Vector pos;

    if (!actor || !soundOrigin) {
        return qfalse;
    }

    pos = soundOrigin;
    return SpStealth_IsInRearBlindArc(actor, pos);
}

float SpStealth_VisionScale(void)
{
    float scale;

    if (!SpStealth_Active() || !g_sp_vision_scale) {
        return 1.0f;
    }

    scale = g_sp_vision_scale->value;
    if (scale < 1.0f) {
        return 1.0f;
    }
    if (scale > 2.0f) {
        return 2.0f;
    }
    return scale;
}

float SpStealth_FootstepEventRadius(float baseRadius, Sentient *pedestrian)
{
    float stealth;

    if (!SpStealth_Active() || !pedestrian || baseRadius <= 0.0f) {
        return baseRadius;
    }

    stealth = pedestrian->stealthMovementScale;
    if (stealth < 0.0f) {
        stealth = 0.0f;
    }
    if (stealth > 1.0f) {
        return baseRadius;
    }

    return baseRadius * (0.35f + 0.65f * stealth);
}

float SpStealth_EffectiveNoticeMult(float profileNoticeMult, Sentient *enemy)
{
    float stealth;
    float bonus;

    if (!SpStealth_Active() || profileNoticeMult <= 0.0f) {
        return profileNoticeMult;
    }

    if (!enemy) {
        return profileNoticeMult;
    }

    stealth = enemy->stealthMovementScale;
    if (stealth >= 1.0f) {
        return profileNoticeMult;
    }

    bonus = (1.0f - stealth) * 0.55f;
    return profileNoticeMult / (1.0f + bonus);
}

void SpStealth_UpdatePlayerStance(Player *player)
{
    float stanceStealth;

    if (!SpStealth_Active() || !player) {
        return;
    }

    if (player->HasMovementStealthOverride()) {
        return;
    }

    if (player->maxs.z <= PRONE_MAXS_Z + 1.0f) {
        stanceStealth = 0.30f;
    } else if (player->maxs.z <= CROUCH_MAXS_Z + 1.0f) {
        if (player->maxs.z > CROUCH_RUN_MAXS_Z - 2.0f) {
            stanceStealth = 0.65f;
        } else {
            stanceStealth = 0.50f;
        }
    } else {
        stanceStealth = 1.0f;
    }

    player->stealthMovementScale = stanceStealth;
}

void SpStealth_ApplySquadShareAwareness(Actor *receiver, Actor *sharer, Sentient *enemy)
{
    ActorEnemy *pActorEnemy;
    float       partial;
    float       cap;

    if (!receiver || !sharer || !enemy) {
        return;
    }

    pActorEnemy = receiver->m_PotentialEnemies.AddPotentialEnemy(enemy);
    if (!pActorEnemy) {
        return;
    }

    if (level.inttime < pActorEnemy->m_iNextEnemyTime) {
        return;
    }

    pActorEnemy->m_vLastKnownPos = enemy->origin;

    if (receiver->CanSee(enemy, 0, receiver->m_fSight * 0.55f, false)) {
        partial = 0.45f;
        cap     = 0.55f;
        SpStealth_LogConfirm("confirm_squad_los", receiver, enemy);
    } else if (!SpStealth_IsInRearBlindArc(receiver, enemy->origin)) {
        partial = 0.22f;
        cap     = 0.35f;
        SpStealth_LogConfirm("confirm_squad_flank", receiver, enemy);
    } else {
        partial = 0.10f;
        cap     = 0.18f;
        SpStealth_LogConfirm("confirm_squad_behind", receiver, enemy);
        receiver->RaiseAlertness(0.12f);
        receiver->SetCuriousAnimHint(5);
    }

    if (pActorEnemy->m_fVisibility < partial) {
        pActorEnemy->m_fVisibility = partial;
    }
    if (pActorEnemy->m_fTotalVisibility < cap) {
        pActorEnemy->m_fTotalVisibility = cap;
    }

    if (pActorEnemy->m_fTotalVisibility < 1.0f) {
        receiver->SetEnemyPos(enemy->origin);
    }
}
