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

void CL_MapShared_Set(const char *mapname, byte *buffer, int length);
byte *CL_MapShared_Take(const char *mapname, int *length);
void CL_MapShared_Clear(void);

void CM_SetExternalMapData(const byte *data, int length);
void CM_ClearExternalMapData(void);

void RE_SetExternalMapData(const byte *data, int length);
void RE_ClearExternalMapData(void);
