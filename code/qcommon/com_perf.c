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

#include "qcommon.h"
#include "com_perf.h"

static cvar_t *com_perfprofile;
static cvar_t *com_loadScreenMs;
static cvar_t *com_touchMemory;

static void Com_PerfSetIfDefault(cvar_t *var, const char *value)
{
    if (!var) {
        return;
    }
    if (!var->modified) {
        Cvar_Set(var->name, value);
    }
}

void Com_PerfInit(void)
{
    com_perfprofile = Cvar_Get("com_perfprofile", "4", CVAR_ARCHIVE);
    Cvar_CheckRange(com_perfprofile, COM_PERFPROFILE_OFF, COM_PERFPROFILE_AUTO, qtrue);
    com_loadScreenMs = Cvar_Get("com_loadScreenMs", "0", CVAR_ARCHIVE);
    com_touchMemory  = Cvar_Get("com_touchMemory", "-1", CVAR_ARCHIVE);
    Cvar_CheckRange(com_touchMemory, -1, 1, qtrue);

    Cmd_AddCommand("perf_apply", Com_ApplyPerfProfileCommon);
}

int Com_PerfResolveProfile(void)
{
    int profile;

    if (!com_perfprofile) {
        return COM_PERFPROFILE_OFF;
    }

    profile = com_perfprofile->integer;
    if (profile == COM_PERFPROFILE_AUTO) {
        unsigned long long ram = Sys_TotalPhysicalMemory();
        cpuFeatures_t      cpu = Sys_GetProcessorFeatures();

        if (ram > 0 && ram <= (unsigned long long)2 * 1024 * 1024 * 1024) {
            profile = COM_PERFPROFILE_LEGACY;
        } else if (ram > 0 && ram <= (unsigned long long)8 * 1024 * 1024 * 1024) {
            profile = COM_PERFPROFILE_BALANCED;
        } else if (ram == 0) {
            profile = COM_PERFPROFILE_BALANCED;
        } else {
            profile = COM_PERFPROFILE_MODERN;
        }

        if (!(cpu & CF_SSE2)) {
            if (profile > COM_PERFPROFILE_LEGACY) {
                profile = COM_PERFPROFILE_LEGACY;
            }
        }
    }

    if (profile < COM_PERFPROFILE_OFF || profile > COM_PERFPROFILE_MODERN) {
        profile = COM_PERFPROFILE_BALANCED;
    }

    return profile;
}

void Com_ApplyPerfProfileCommon(void)
{
    int profile;

    if (!com_perfprofile) {
        Com_PerfInit();
    }

    profile = Com_PerfResolveProfile();
    if (profile == COM_PERFPROFILE_OFF) {
        Com_Printf("com_perfprofile is off (0). No changes applied.\n");
        return;
    }

    switch (profile) {
    case COM_PERFPROFILE_LEGACY:
        Com_PerfSetIfDefault(Cvar_Get("com_busyWait", "0", CVAR_ARCHIVE), "0");
        Com_PerfSetIfDefault(Cvar_Get("com_maxfps", "85", CVAR_ARCHIVE), "60");
        Com_PerfSetIfDefault(Cvar_Get("com_maxfpsUnfocused", "0", CVAR_ARCHIVE), "30");
        Com_PerfSetIfDefault(Cvar_Get("com_maxfpsMinimized", "0", CVAR_ARCHIVE), "15");
        Com_PerfSetIfDefault(Cvar_Get("low_anim_memory", "0", 0), "1");
        if (!com_loadScreenMs->integer) {
            Cvar_Set("com_loadScreenMs", "150");
        }
        break;
    case COM_PERFPROFILE_BALANCED:
        Com_PerfSetIfDefault(Cvar_Get("com_busyWait", "0", CVAR_ARCHIVE), "0");
        Com_PerfSetIfDefault(Cvar_Get("com_maxfps", "85", CVAR_ARCHIVE), "85");
        if (!com_loadScreenMs->integer) {
            Cvar_Set("com_loadScreenMs", "100");
        }
        break;
    case COM_PERFPROFILE_MODERN:
        Com_PerfSetIfDefault(Cvar_Get("com_busyWait", "0", CVAR_ARCHIVE), "1");
        Com_PerfSetIfDefault(Cvar_Get("com_maxfps", "85", CVAR_ARCHIVE), "144");
        Com_PerfSetIfDefault(Cvar_Get("com_maxfpsUnfocused", "0", CVAR_ARCHIVE), "60");
        Com_PerfSetIfDefault(Cvar_Get("low_anim_memory", "0", 0), "0");
        if (!com_loadScreenMs->integer) {
            Cvar_Set("com_loadScreenMs", "66");
        }
        break;
    default:
        break;
    }

    Com_Printf(
        "Applied com_perfprofile %d (%s)\n",
        profile,
        profile == COM_PERFPROFILE_LEGACY   ? "legacy"
        : profile == COM_PERFPROFILE_MODERN ? "modern"
                                            : "balanced"
    );
}

void Com_ApplyPerfProfileRenderer(void)
{
    int profile;

    profile = Com_PerfResolveProfile();
    if (profile == COM_PERFPROFILE_OFF) {
        return;
    }

    switch (profile) {
    case COM_PERFPROFILE_LEGACY:
        Com_PerfSetIfDefault(Cvar_Get("r_fastsky", "0", 0), "1");
        Com_PerfSetIfDefault(Cvar_Get("r_largemap", "0", 0), "0");
        Com_PerfSetIfDefault(Cvar_Get("r_subdivisions", "4", CVAR_ARCHIVE | CVAR_LATCH), "12");
        Com_PerfSetIfDefault(Cvar_Get("r_textureDetails", "1", 0), "0");
        Com_PerfSetIfDefault(Cvar_Get("r_ext_aniso_filter", "0", CVAR_ARCHIVE | CVAR_LATCH), "0");
        break;
    case COM_PERFPROFILE_BALANCED:
        Com_PerfSetIfDefault(Cvar_Get("r_largemap", "0", 0), "0");
        Com_PerfSetIfDefault(Cvar_Get("r_subdivisions", "4", CVAR_ARCHIVE | CVAR_LATCH), "6");
        break;
    case COM_PERFPROFILE_MODERN:
        Com_PerfSetIfDefault(Cvar_Get("r_largemap", "0", 0), "1");
        Com_PerfSetIfDefault(Cvar_Get("r_subdivisions", "4", CVAR_ARCHIVE | CVAR_LATCH), "4");
        Com_PerfSetIfDefault(Cvar_Get("r_ext_aniso_filter", "0", CVAR_ARCHIVE | CVAR_LATCH), "1");
        Com_PerfSetIfDefault(Cvar_Get("r_ext_max_anisotropy", "2", CVAR_ARCHIVE | CVAR_LATCH), "8");
        break;
    default:
        break;
    }
}

qboolean Com_ShouldTouchMemoryAfterLoad(void)
{
    int profile;

    if (!com_touchMemory) {
        return qtrue;
    }

    if (com_touchMemory->integer == 0) {
        return qfalse;
    }
    if (com_touchMemory->integer == 1) {
        return qtrue;
    }

    profile = Com_PerfResolveProfile();
    if (profile == COM_PERFPROFILE_LEGACY || Sys_LowPhysicalMemory()) {
        return qfalse;
    }

    if (Sys_TotalPhysicalMemory() > 0 && Sys_TotalPhysicalMemory() <= (unsigned long long)4 * 1024 * 1024 * 1024) {
        return qfalse;
    }

    return qtrue;
}

int Com_LoadScreenIntervalMs(void)
{
    if (!com_loadScreenMs) {
        return 100;
    }
    if (com_loadScreenMs->integer > 0) {
        return com_loadScreenMs->integer;
    }
    return 100;
}
