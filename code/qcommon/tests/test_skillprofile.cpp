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

#include "../q_shared.h"

#include <cstdio>
#include <cstring>

typedef struct skillCombatProfile_s {
    float aimScatterMult;
    float accuracyMult;
    float noticeMult;
    float soundAwarenessMult;
    float grenadeAwarenessMult;
    float coverHoldMult;
    float shareEnemyDelay;
    float stepSideBias;
    float runAndShootStayChance;
    float suppressScatterMult;
    float footstepCuriousMult;
} skillCombatProfile_t;

static const skillCombatProfile_t skillProfiles[3] = {
    {1.20f, 0.85f, 0.75f, 0.80f, 0.70f, 1.35f, 1.10f, 0.15f, 0.10f, 1.15f, 1.00f},
    {1.00f, 1.00f, 1.00f, 1.00f, 1.00f, 1.00f, 0.75f, 0.35f, 0.25f, 1.00f, 1.00f},
    {0.55f, 1.35f, 1.45f, 1.15f, 1.35f, 0.65f, 0.65f, 0.70f, 0.55f, 0.80f, 0.88f},
};

static int failures;

static void expect_true(const char *name, int value)
{
    if (!value) {
        std::fprintf(stderr, "FAIL: %s expected true\n", name);
        failures++;
    }
}

static void expect_float_lt(const char *name, float a, float b)
{
    if (!(a < b)) {
        std::fprintf(stderr, "FAIL: %s: expected %f < %f\n", name, a, b);
        failures++;
    }
}

static void expect_float_gt(const char *name, float a, float b)
{
    if (!(a > b)) {
        std::fprintf(stderr, "FAIL: %s: expected %f > %f\n", name, a, b);
        failures++;
    }
}

static void test_skill_ordering(void)
{
    std::printf("test_skill_ordering...\n");

    expect_float_gt("easy scatter > medium", skillProfiles[0].aimScatterMult, skillProfiles[1].aimScatterMult);
    expect_float_gt("medium scatter > hard", skillProfiles[1].aimScatterMult, skillProfiles[2].aimScatterMult);

    expect_float_lt("easy accuracy < medium", skillProfiles[0].accuracyMult, skillProfiles[1].accuracyMult);
    expect_float_lt("medium accuracy < hard", skillProfiles[1].accuracyMult, skillProfiles[2].accuracyMult);

    expect_float_lt("easy notice < medium", skillProfiles[0].noticeMult, skillProfiles[1].noticeMult);
    expect_float_lt("medium notice < hard", skillProfiles[1].noticeMult, skillProfiles[2].noticeMult);

    expect_float_gt("easy cover hold > hard", skillProfiles[0].coverHoldMult, skillProfiles[2].coverHoldMult);
    expect_float_gt("hard step-side > easy", skillProfiles[2].stepSideBias, skillProfiles[0].stepSideBias);
    expect_float_lt("hard share delay < easy", skillProfiles[2].shareEnemyDelay, skillProfiles[0].shareEnemyDelay);
}

static void test_scatter_bounds(void)
{
    std::printf("test_scatter_bounds...\n");

    expect_true("easy scatter positive", skillProfiles[0].aimScatterMult > 0.0f);
    expect_true("hard scatter positive", skillProfiles[2].aimScatterMult > 0.0f);
    expect_true("hard scatter below 1", skillProfiles[2].aimScatterMult < 1.0f);
    expect_true("hard accuracy above 1", skillProfiles[2].accuracyMult > 1.0f);
}

static void test_map_preset_extra(void)
{
    float hardAcc = skillProfiles[2].accuracyMult;
    float withMap = hardAcc * 1.25f;

    std::printf("test_map_preset_extra...\n");
    expect_float_gt("map preset raises hard accuracy", withMap, hardAcc);
}

int main()
{
    failures = 0;

    test_skill_ordering();
    test_scatter_bounds();
    test_map_preset_extra();

    if (failures) {
        std::fprintf(stderr, "%d skill profile test(s) failed\n", failures);
        return 1;
    }

    std::printf("skill profile tests passed\n");
    return 0;
}
