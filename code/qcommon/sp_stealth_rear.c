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

#include "sp_stealth_rear.h"

#include <math.h>

float SpStealth_DotThresholdForRearDeg(float rearDeg)
{
    float blindHalf;

    if (rearDeg < 0.0f) {
        rearDeg = 0.0f;
    }
    if (rearDeg > 180.0f) {
        rearDeg = 180.0f;
    }

    blindHalf = 90.0f + (180.0f - rearDeg) * 0.5f;
    return cosf(blindHalf * (float)M_PI / 180.0f);
}

qboolean SpStealth_DotInRearBlindArc(float forwardDot, float rearDeg)
{
    return forwardDot < SpStealth_DotThresholdForRearDeg(rearDeg) ? qtrue : qfalse;
}
