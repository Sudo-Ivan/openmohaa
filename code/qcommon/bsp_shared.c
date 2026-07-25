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

#include "q_shared.h"
#include "bsp_shared.h"

static const byte *com_bspSharedData;
static int         com_bspSharedLength;

void Com_BspSharedSet(const byte *data, int length)
{
    com_bspSharedData   = data;
    com_bspSharedLength = length;
}

void Com_BspSharedClear(void)
{
    com_bspSharedData   = NULL;
    com_bspSharedLength = 0;
}

const byte *Com_BspSharedData(void)
{
    return com_bspSharedData;
}

int Com_BspSharedLength(void)
{
    return com_bspSharedLength;
}
