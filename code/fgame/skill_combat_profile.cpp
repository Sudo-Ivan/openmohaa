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
static cvar_t *g_sp_ai_notify;

static int g_lastNotifySkill = -1;
static char g_lastNotifyMap[MAX_QPATH] = "";

static const char *SkillCombatProfile_SkillName(int skillIndex)
{
    switch (skillIndex) {
    case 0:
        return "easy";
    case 2:
        return "hard";
    default:
        return "medium";
    }
}

typedef struct mapSkillPreset_s {
    const char *prefix;
    float       accuracyExtra;
    float       noticeExtra;
} mapSkillPreset_t;

static const skillCombatProfile_t skillProfiles[3] = {
    {1.20f, 0.85f, 0.75f, 0.80f, 0.70f, 1.35f, 1.10f, 0.15f, 0.10f, 1.15f},
    {1.00f, 1.00f, 1.00f, 1.00f, 1.00f, 1.00f, 0.75f, 0.35f, 0.25f, 1.00f},
    {0.55f, 1.35f, 1.45f, 1.40f, 1.35f, 0.65f, 0.35f, 0.70f, 0.55f, 0.80f},
};

static const mapSkillPreset_t mapPresetsDefault[] = {
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

#define MAX_MAP_SKILL_PRESETS 128

static mapSkillPreset_t g_mapPresets[MAX_MAP_SKILL_PRESETS];
static int              g_numMapPresets;

static void SkillCombatProfile_AddPreset(const char *prefix, float accuracyExtra, float noticeExtra)
{
    int i;

    if (!prefix || !prefix[0]) {
        return;
    }

    for (i = 0; i < g_numMapPresets; i++) {
        if (!Q_stricmp(g_mapPresets[i].prefix, prefix)) {
            g_mapPresets[i].accuracyExtra = accuracyExtra;
            g_mapPresets[i].noticeExtra   = noticeExtra;
            return;
        }
    }

    if (g_numMapPresets >= MAX_MAP_SKILL_PRESETS) {
        return;
    }

    g_mapPresets[g_numMapPresets].prefix        = (char *)gi.Malloc(strlen(prefix) + 1);
    Q_strncpyz((char *)g_mapPresets[g_numMapPresets].prefix, prefix, strlen(prefix) + 1);
    g_mapPresets[g_numMapPresets].accuracyExtra = accuracyExtra;
    g_mapPresets[g_numMapPresets].noticeExtra   = noticeExtra;
    g_numMapPresets++;
}

static void SkillCombatProfile_LoadDefaultPresets(void)
{
    int i;

    g_numMapPresets = 0;
    for (i = 0; mapPresetsDefault[i].prefix; i++) {
        SkillCombatProfile_AddPreset(
            mapPresetsDefault[i].prefix, mapPresetsDefault[i].accuracyExtra, mapPresetsDefault[i].noticeExtra
        );
    }
}

static void SkillCombatProfile_LoadPresetFile(void)
{
    char        *buffer;
    char        *cursor;
    const char  *token;
    char         prefix[MAX_QPATH];
    float        acc;
    float        notice;

    if (gi.FS_ReadFile("sp_skill_presets.cfg", (void **)&buffer, qtrue) <= 0) {
        return;
    }

    cursor = buffer;
    while (1) {
        token = COM_Parse(&cursor);
        if (!token[0]) {
            break;
        }
        if (token[0] == '#') {
            while (*cursor && *cursor != '\n') {
                cursor++;
            }
            continue;
        }

        Q_strncpyz(prefix, token, sizeof(prefix));

        token = COM_Parse(&cursor);
        if (!token[0]) {
            break;
        }
        acc = (float)atof(token);

        token = COM_Parse(&cursor);
        if (!token[0]) {
            break;
        }
        notice = (float)atof(token);

        if (acc > 0.0f && notice > 0.0f) {
            SkillCombatProfile_AddPreset(prefix, acc, notice);
        }
    }

    gi.FS_FreeFile(buffer);
}

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

static void SkillCombatProfile_Notify(void)
{
    int skillIndex;

    if (!SkillCombatProfile_Active()) {
        return;
    }

    if (!g_sp_ai_notify) {
        g_sp_ai_notify = gi.Cvar_Get("g_sp_ai_notify", "1", 0);
    }
    if (!g_sp_ai_notify->integer) {
        return;
    }

    skillIndex = SkillIndex();
    if (skillIndex == g_lastNotifySkill && !Q_stricmp(g_profileMap, g_lastNotifyMap)) {
        return;
    }

    g_lastNotifySkill = skillIndex;
    Q_strncpyz(g_lastNotifyMap, g_profileMap, sizeof(g_lastNotifyMap));

    gi.Printf(
        "----- OpenMoHAA SP AI: skill combat profile active (%s",
        SkillCombatProfile_SkillName(skillIndex)
    );
    if (g_profileMap[0]) {
        gi.Printf(", map %s", g_profileMap);
    }
    gi.Printf(") -----\n");
    gi.Printf(
        "  aimScatter x%.2f | accuracy x%.2f | notice x%.2f\n",
        g_activeProfile.aimScatterMult,
        g_activeProfile.accuracyMult,
        g_activeProfile.noticeMult
    );
}

static void SkillCombatProfile_Rebuild(void)
{
    const skillCombatProfile_t *base = &skillProfiles[SkillIndex()];
    float                       accExtra = 1.0f;
    float                       noticeExtra = 1.0f;
    int                         i;

    g_activeProfile = *base;

    if (g_profileMap[0] && SkillIndex() >= 2) {
        for (i = 0; i < g_numMapPresets; i++) {
            if (!Q_stricmpn(g_profileMap, g_mapPresets[i].prefix, (int)strlen(g_mapPresets[i].prefix))) {
                accExtra    = g_mapPresets[i].accuracyExtra;
                noticeExtra = g_mapPresets[i].noticeExtra;
                break;
            }
        }
    }

    g_activeProfile.accuracyMult *= accExtra;
    g_activeProfile.noticeMult *= noticeExtra;
    g_profileReady = qtrue;

    SkillCombatProfile_Notify();

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
    g_sp_ai_debug   = gi.Cvar_Get("g_sp_ai_debug", "0", 0);
    g_sp_ai_notify  = gi.Cvar_Get("g_sp_ai_notify", "1", 0);
    g_profileMap[0] = '\0';
    g_lastNotifySkill = -1;
    g_lastNotifyMap[0] = '\0';
    SkillCombatProfile_LoadDefaultPresets();
    SkillCombatProfile_LoadPresetFile();
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
