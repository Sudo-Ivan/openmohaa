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

#include "g_local.h"
#include "skill_combat_profile.h"
#include "gamecvars.h"

cvar_t *g_sp_ai_debug;

typedef struct mapSkillPreset_s {
    const char *prefix;
    float       accuracyExtra;
    float       noticeExtra;
} mapSkillPreset_t;

static const skillCombatProfile_t skillProfiles[3] = {
    // easy
    {1.20f, 0.85f, 0.75f, 0.80f, 0.70f, 1.35f, 1.10f, 0.15f, 0.10f, 1.15f},
    // medium
    {1.00f, 1.00f, 1.00f, 1.00f, 1.00f, 1.00f, 0.75f, 0.35f, 0.25f, 1.00f},
    // hard
    {0.55f, 1.35f, 1.45f, 1.40f, 1.35f, 0.65f, 0.35f, 0.70f, 0.55f, 0.80f},
};

static const mapSkillPreset_t mapPresets[] = {
    // Playtest matrix targets (skill 2): base m1-m6, Spearhead (libya/sicily/norway),
    // Breakthrough outliers get the same hard uplift via prefix match.
    {"m1l1", 1.10f, 1.05f},
    {"m1l2", 1.10f, 1.05f},
    {"m2l1", 1.15f, 1.10f},
    {"m2l2", 1.15f, 1.10f},
    {"m3l1", 1.20f, 1.15f},
    {"m3l2", 1.20f, 1.15f},
    {"m4l1", 1.20f, 1.15f},
    {"m4l2", 1.25f, 1.20f},
    {"m5l1", 1.25f, 1.20f},
    {"m5l2", 1.25f, 1.20f},
    {"m6l1", 1.30f, 1.25f},
    {"m6l2", 1.30f, 1.25f},
    {"m6l3", 1.30f, 1.25f},
    {"libya", 1.15f, 1.10f},
    {"sicily", 1.20f, 1.15f},
    {"norway", 1.20f, 1.15f},
    {"m1l1b", 1.15f, 1.10f},
    {"m2l1b", 1.20f, 1.15f},
    {"m3l1b", 1.20f, 1.15f},
    {NULL, 1.0f, 1.0f},
};

static skillCombatProfile_t g_activeProfile;
static qboolean             g_profileReady = qfalse;
static char                 g_profileMap[MAX_QPATH];

static int SkillIndex(void)
{
    int s = skill ? skill->integer : 1;
    if (s < 0) {
        s = 0;
    }
    if (s > 2) {
        s = 2;
    }
    return s;
}

static void SkillCombatProfile_Rebuild(void)
{
    const skillCombatProfile_t *base = &skillProfiles[SkillIndex()];
    float                       accExtra = 1.0f;
    float                       noticeExtra = 1.0f;
    int                         i;

    g_activeProfile = *base;

    if (g_profileMap[0] && SkillIndex() >= 2) {
        for (i = 0; mapPresets[i].prefix; i++) {
            if (!Q_stricmpn(g_profileMap, mapPresets[i].prefix, (int)strlen(mapPresets[i].prefix))) {
                accExtra    = mapPresets[i].accuracyExtra;
                noticeExtra = mapPresets[i].noticeExtra;
                break;
            }
        }
    }

    g_activeProfile.accuracyMult *= accExtra;
    g_activeProfile.noticeMult *= noticeExtra;
    g_profileReady = qtrue;

    if (g_sp_ai_debug && g_sp_ai_debug->integer) {
        gi.Printf(
            "SkillCombatProfile skill=%d map=%s aimScatter=%.2f accuracy=%.2f notice=%.2f\n",
            SkillIndex(),
            g_profileMap[0] ? g_profileMap : "(none)",
            g_activeProfile.aimScatterMult,
            g_activeProfile.accuracyMult,
            g_activeProfile.noticeMult
        );
    }
}

void SkillCombatProfile_Init(void)
{
    g_sp_ai_debug = gi.Cvar_Get("g_sp_ai_debug", "0", 0);
    g_profileMap[0] = '\0';
    SkillCombatProfile_Rebuild();
}

void SkillCombatProfile_SetMap(const char *mapname)
{
    const char *slash;
    const char *dot;
    size_t      len;

    g_profileMap[0] = '\0';
    if (!mapname || !mapname[0]) {
        SkillCombatProfile_Rebuild();
        return;
    }

    slash = strrchr(mapname, '/');
    if (slash) {
        mapname = slash + 1;
    }

    Q_strncpyz(g_profileMap, mapname, sizeof(g_profileMap));
    dot = strchr(g_profileMap, '.');
    if (dot) {
        g_profileMap[dot - g_profileMap] = '\0';
    }
    len = strlen(g_profileMap);
    if (len > 4 && !Q_stricmp(g_profileMap + len - 4, "_sml")) {
        g_profileMap[len - 4] = '\0';
    }

    SkillCombatProfile_Rebuild();
}

const skillCombatProfile_t *SkillCombatProfile_Get(void)
{
    if (!g_profileReady || (skill && skill->modified)) {
        if (skill) {
            skill->modified = qfalse;
        }
        SkillCombatProfile_Rebuild();
    }
    return &g_activeProfile;
}

qboolean SkillCombatProfile_Active(void)
{
    return (g_gametype && g_gametype->integer == GT_SINGLE_PLAYER) ? qtrue : qfalse;
}
