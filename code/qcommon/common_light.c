/*
===========================================================================
Copyright (C) 2025 the OpenMoHAA team

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

#include "q_shared.h"
#include <stdarg.h>

#ifdef _WIN32
#include <windows.h>

static LARGE_INTEGER s_perf_freq;
static LARGE_INTEGER s_perf_base;
static qboolean      s_perf_init;

int Sys_Milliseconds(void)
{
    LARGE_INTEGER now;

    if (!s_perf_init) {
        QueryPerformanceFrequency(&s_perf_freq);
        QueryPerformanceCounter(&s_perf_base);
        s_perf_init = qtrue;
    }

    QueryPerformanceCounter(&now);
    return (int)((now.QuadPart - s_perf_base.QuadPart) * 1000 / s_perf_freq.QuadPart);
}
#else
#include <time.h>

int Sys_Milliseconds(void)
{
    struct timespec ts;

    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
        return 0;
    }

    return (int)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}
#endif

void QDECL Com_Printf(const char *fmt, ...)
{
    va_list argptr;
    char    text[1024];

    va_start(argptr, fmt);
    Q_vsnprintf(text, sizeof(text), fmt, argptr);
    va_end(argptr);

    printf("%s", text);
}

void QDECL Com_DPrintf(const char *fmt, ...)
{
    va_list argptr;
    char    text[1024];

    va_start(argptr, fmt);
    Q_vsnprintf(text, sizeof(text), fmt, argptr);
    va_end(argptr);

    printf("%s", text);
}

void QDECL Com_DPrintf2(const char *fmt, ...)
{
    va_list argptr;
    char    text[1024];

    va_start(argptr, fmt);
    Q_vsnprintf(text, sizeof(text), fmt, argptr);
    va_end(argptr);

    printf("%s", text);
}

void QDECL Com_Error(int code, const char *fmt, ...)
{
    va_list argptr;
    char    text[1024];

    va_start(argptr, fmt);
    Q_vsnprintf(text, sizeof(text), fmt, argptr);
    va_end(argptr);

    fprintf(stderr, "%s", text);
}
