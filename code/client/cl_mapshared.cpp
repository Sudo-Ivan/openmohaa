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

#include "client.h"
#include "cl_mapshared.h"
#include "../qcommon/cm_public.h"

static byte *cl_mapSharedBuffer;
static int   cl_mapSharedLength;
static char  cl_mapSharedName[MAX_QPATH];

void CL_MapShared_Clear(void)
{
    if (cl_mapSharedBuffer) {
        FS_FreeFile(cl_mapSharedBuffer);
        cl_mapSharedBuffer = NULL;
    }
    cl_mapSharedLength  = 0;
    cl_mapSharedName[0] = '\0';
    CM_ClearExternalMapData();
}

void CL_MapShared_Set(const char *mapname, byte *buffer, int length)
{
    CL_MapShared_Clear();
    cl_mapSharedBuffer = buffer;
    cl_mapSharedLength = length;
    Q_strncpyz(cl_mapSharedName, mapname, sizeof(cl_mapSharedName));
    CM_SetExternalMapData(buffer, length);
}

byte *CL_MapShared_Take(const char *mapname, int *length)
{
    if (!cl_mapSharedBuffer || Q_stricmp(cl_mapSharedName, mapname)) {
        if (length) {
            *length = 0;
        }
        return NULL;
    }

    if (length) {
        *length = cl_mapSharedLength;
    }

    byte *buf            = cl_mapSharedBuffer;
    cl_mapSharedBuffer   = NULL;
    cl_mapSharedLength   = 0;
    cl_mapSharedName[0]  = '\0';
    CM_ClearExternalMapData();
    return buf;
}
