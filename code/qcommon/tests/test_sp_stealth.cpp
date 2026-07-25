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

// Playtest matrix (manual): crouch behind single AI; behind AI with forward squadmate;
// crouch-walk vs run; snow m6* vs interior; g_sp_stealth_debug 1; g_sp_population_scale 1 vs 1.35.

#include "../q_shared.h"
#include "../sp_stealth_rear.h"

#include <cstdio>

static int failures;

static void expect_true(const char *name, int value)
{
    if (!value) {
        std::fprintf(stderr, "FAIL: %s expected true\n", name);
        failures++;
    }
}

static void expect_false(const char *name, int value)
{
    if (value) {
        std::fprintf(stderr, "FAIL: %s expected false\n", name);
        failures++;
    }
}

static void test_rear_arc_behind(void)
{
    std::printf("test_rear_arc_behind...\n");

    expect_true("directly behind in blind arc", SpStealth_DotInRearBlindArc(-1.0f, 100.0f));
    expect_false("directly ahead not blind", SpStealth_DotInRearBlindArc(1.0f, 100.0f));
    expect_false("side at 0 not blind for 100deg", SpStealth_DotInRearBlindArc(0.0f, 100.0f));
}

static void test_rear_arc_threshold_ordering(void)
{
    std::printf("test_rear_arc_threshold_ordering...\n");

    float narrow = SpStealth_DotThresholdForRearDeg(60.0f);
    float wide   = SpStealth_DotThresholdForRearDeg(140.0f);

    expect_true("wider rear arc has higher threshold", wide > narrow);
}

int main()
{
    failures = 0;

    test_rear_arc_behind();
    test_rear_arc_threshold_ordering();

    if (failures) {
        std::fprintf(stderr, "%d sp stealth test(s) failed\n", failures);
        return 1;
    }

    std::printf("sp stealth tests passed\n");
    return 0;
}
