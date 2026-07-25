# AGENTS.md -- OpenMoHAA Agent Guide

## Commit Message Format

Conventional Commits: `type(scope): description`

```
fix(client): round com_maxfps division for accurate FPS capping (#530)
docs(readme): add SHA for adversarial bug-finding tests
test: add adversarial tests that found 4 bugs
feat: add MOTD cvar, round end logging, mapname.cfg
chore: fix compiler warnings across the codebase
ci(workflows): update actions and restructure release uploads
```

- **Types:** `feat`, `fix`, `test`, `docs`, `chore`, `ci`, `Revert`
- **Scopes:** `client`, `fgame`, `renderer`, `server`, `net`, `readme`, `workflows`, `misc`
- **Issues:** `(#NNN)` for GitHub issues
- **No co-authors, no emdashes.** Keep subject line concise.
- Bug fixes should be logged in README.md "Our changes" with commit SHA.

## Build System

```sh
cmake -B build -G "Ninja Multi-Config" -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build --config RelWithDebInfo --parallel
```

Key CMake options: `BUILD_SERVER`, `BUILD_CLIENT`, `BUILD_RENDERER_GL1/2`,
`BUILD_GAME_LIBRARIES`, `BUILD_GAME_QVMS`, `USE_OPENAL`, `USE_HTTP`.

Minimum CMake 3.25, C17/C++17, Flex+Bison required.

## Test System

```sh
# quick test build (no engine build):
cmake -S . -B build-test -G Ninja -DCMAKE_BUILD_TYPE=Debug \
    -DBUILD_SERVER=OFF -DBUILD_CLIENT=OFF -DBUILD_GAME_LIBRARIES=OFF \
    -DBUILD_RENDERER_GL1=OFF -DBUILD_RENDERER_GL2=OFF \
    -DUSE_OPENAL=OFF -DUSE_HTTP=OFF -DUSE_CODEC_VORBIS=OFF -DUSE_CODEC_OPUS=OFF -DUSE_CODEC_MAD=OFF
cmake --build build-test --parallel
ctest --test-dir build-test --output-on-failure
```

All test targets (defined in `cmake/tests/`):

| Test | Source | What it tests |
|------|--------|---------------|
| `test_lz77` | `code/corepp/tests/test_lz77.cpp` | LZ77 compress/decompress round-trip |
| `test_netsec` | `code/qcommon/tests/test_netsec.cpp` | Huffman, path traversal |
| `test_md5` | `code/fgame/tests/test_md5.cpp` | MD5 RFC 1321 vectors |
| `test_bans` | `code/server/tests/test_bans.cpp` | Ban file parsing |
| `test_oracles` | `code/qcommon/tests/test_oracles.cpp` | Deterministic functions vs known-good values |
| `test_smoke` | `code/qcommon/tests/test_smoke.cpp` | Core components init/basic function |
| `test_acceptance` | `code/qcommon/tests/test_acceptance.cpp` | Combined feature-level behavior |
| `test_adversarial` | `code/qcommon/tests/test_adversarial.cpp` | Edge-case bugs, UB, crashes (will crash on COM_Compress(NULL)) |

Test pattern: standalone C++ main(), no framework, return 0 on pass, print FAIL on fail.

Test compilation units to link for qcommon tests:
- `code/qcommon/q_shared.c` -- string utils, parsing, info, growlist, endian
- `code/qcommon/q_math.c` -- vector/quaternion/angle math
- `code/qcommon/crc.c` -- CRC-16/CCITT-FALSE (needs `extern "C" { #include "crc.h" }` in C++ tests)
- `code/qcommon/common_light.c` -- Com_Printf/Com_Error stubs

## Project Structure

```
code/
  qcommon/      -- Shared: math, filesystem, networking, crypto, msg buffers
  server/       -- Server engine: clients, world, bans, GameSpy
  client/       -- Client engine: renderer binding, input, net channels
  fgame/        -- Game logic: player, items, weapons, AI, scripting
  cgame/        -- Client-side game: prediction, HUD, scoreboard
  renderergl1/  -- OpenGL 1.x renderer
  renderergl2/  -- OpenGL 2.x renderer (OFF by default)
  script/       -- Script engine: compiler, VM
  tiki/         -- TIKI model/animation engine
  skeletor/     -- Skeletal animation engine
  botlib/       -- Bot AI: navmesh, pathfinding
  corepp/       -- C++ utilities (LZ77)
  sdl/          -- SDL platform layer
  sys/          -- Entry points, threading
  parser/       -- Flex/Bison generated parser
  thirdparty/   -- recastnavigation, libmad
  tools/        -- QVM tools (q3asm, q3lcc, q3rcc)
cmake/
  tests/        -- CTest definitions per test target
  compilers/    -- Compiler flags (gcc, clang, msvc)
  platforms/    -- Platform settings (linux, macos, windows)
  libraries/    -- Third-party library finders
```

## Code Style (`.clang-format`)

- C++17, 4-space indent, no tabs, column limit 120
- Pointers/references right-aligned (`int *p`, `int &r`)
- Brace after class/function/namespace, not after enum/struct/union
- No bin-packing arguments
- `#pragma once` in all headers
- Functions: `snake_case` with module prefix (`SV_SendClientGameState`, `Q_stricmp`)
- Types/Classes: `PascalCase` (`Sentient`, `Actor`, `Player`)

## AI Architecture

### Single-Player AI (Actor system)

Single-player uses the `Actor` class (`code/fgame/actor.h`, 2340 lines) with a state-machine approach:

- **States:** `THINKSTATE_IDLE`, `THINKSTATE_CURIOUS`, `THINKSTATE_ATTACK`, `THINKSTATE_COVER`, etc.
- **Accuracy:** `GunTarget()` in `actor.cpp:10623` calculates scatter using cvars like `g_aiScatterWide`, `g_aiScatterHeight`, `g_aiminaccuracy`. Uses uniform random `crandom()` spread — no aim settling over time. Note: `mAccuracy` is 0–1 scale (set via `accuracy` evar, divided by 100 in `EventSetAccuracy`).
- **Cover system:** Full cover state machine in `actor_cover.cpp` (733 lines). Uses `PathNode` with `AI_CONCEALMENT`/`AI_COVER` flags. Team-specific hide durations (Americans 2-4s, Germans 4-15s). No "peek and shoot" behavior.
- **Grenades:** `actor_grenade.cpp`. Flee/kick/throw/martyr states. No cooked throws or area-denial.
- **Pathfinding:** `PathNode`-based A* in `navigate.cpp`. Also supports Recast/Detour navmesh via `g_navigation_legacy` cvar (default 0 = Recast). Fallback on blocked path is `DoFailSafeMove` → `THINKSTATE_NOCLIP` (clips through walls).
- **Weapon selection:** `Sentient::BestWeapon()` in `sentient_combat.cpp:853` selects by static `GetRank()` only — no distance awareness. Actors never switch weapons based on engagement range.
- **Hearing:** `m_fHearing` defaults to `2048` units (line 2822). No per-actor variation. Sound events are handled via `ReceiveAIEvent()`.
- **Key cvars:** `g_aiDamageMult`, `g_aiPathRetry`, `g_aiSupressScatter`, `g_aimcoverfactor`, `g_aimaxdeviation`, `g_aiminaccuracy`, `g_aiScatterWide`, `g_aiScatterHeight`.

### Multiplayer Bots (BotController system)

Multiplayer bots use `BotController` (`code/fgame/playerbot.h`, 330 lines) with a simpler state machine:

- **States:** Attack, Curious, Grenade, Idle, Weapon
- **Navigation:** Uses `IPather` interface — either `LegacyPather` (PathNode A*) or `RecastPather` (Detour navmesh). Navmesh auto-generated from BSP at runtime via Recast.
- **Difficulty:** Controlled by cvars only (`g_bot_attack_react_min_delay`, `g_bot_attack_spreadmult`, `g_bot_turn_speed`, etc.). Hardcoded "Average" skill from `G_GetBotSkill()`. The new `g_bot_skill` (1-5) cvar maps to presets.
- **Weapon selection:** `FindWeaponWithAmmo()` in `playerbot.cpp:1168` selects by `GetRank()`. Now has distance-based preference via `m_pEnemy` distance check.
- **Team behavior:** Bots detect teammates to avoid friendly fire and ignore teammate sound events. New `g_bot_flanking` adds separation force to prevent clumping.
- **Event broadcasting:** `BroadcastEvent()` shares `AI_EVENT_*` across bots (weapon fire, explosions, etc.) — bots become curious and investigate.
- **No squad/flank/cover coordination** — each bot acts independently.
- **Key files:** `playerbot.cpp` (1397 lines), `playerbot_movement.cpp` (1078 lines), `playerbot_rotation.cpp` (153 lines), `playerbot_master.cpp` (114 lines).

## CI / Build Failure Patterns

### Cross-Platform Issues

1. **MSVC compile-time division by zero** — `test_acceptance.cpp:313` and `test_adversarial.cpp:378` used `1.0f / 0.0f` and `0.0f / 0.0f` which MSVC rejects as error C2124. Fix: use `INFINITY` and `NAN` macros from `<cmath>`.

2. **Windows grep pipe (PowerShell)** — `2>&1 | (grep ... || true)` works in bash but PowerShell parses `(cmd)` differently. Error: "Expressions are only allowed as the first element of a pipeline". Fix: add `shell: bash` to the step or skip filtering on Windows.

3. **Windows bash path escapes** — `D:\a\openmohaa\openmohaa` with `shell: bash` causes `\a` to be interpreted as BEL character, mangling the path. Fix: use forward slashes or default PowerShell.

4. **macOS/Windows linker: missing `cvar_t *` definitions** — Adding `extern cvar_t *g_x` in the header AND `gi.Cvar_Get("g_x", ...)` in `CVAR_Init()` is not enough. The `cvar_t *g_x;` file-scope definition must also exist in `gamecvars.cpp`. Without it, Linux links OK but macOS/Windows fail with "unresolved external symbol".

5. **macOS/Windows linker: `G_LogPrintf` unresolved** — Declared in `g_local.h` but never implemented anywhere. Use `gi.Printf()` instead.

### MSVC-Specific Warnings

- `/we4715` treats C4715 (not all control paths return a value) as error
- `C4305` truncation from `double` to `vec_t` — common in q_math.c
- `C4459` declaration of `i` hides global declaration — many instances in q_math.c where function-local `int i` shadows file-scope `int i = 0`
- `C4456` declaration hides previous local declaration (e.g., `final` in q_math.c:4320)

### Common Gotchas

- **`gi.Cvar_Set` vs `gi.cvar_set`** — Only `gi.cvar_set` exists (lowercase). `gi.Cvar_Set` compiles on some platforms but not others.
- **`cvar_t*` definitions** must be at file scope in `gamecvars.cpp`, not just in `CVAR_Init()`. The linker needs the symbol itself, not just a `Cvar_Get` assignment.
- **Windows `shell: bash`** — Only use for Linux/macOS builds. Windows bash mangles `\` paths.
- **`extern "C"` cvars** — All cvar declarations in `gamecvars.h` are inside `extern "C"`, so the definitions in `gamecvars.cpp` must match. Plain `cvar_t *g_x;` has C++ linkage by default but the `extern "C"` declaration expects C linkage. This works because the `.cpp` file includes the header which provides the `extern "C"` declaration context.

## Important Conventions

- **`extern "C"`** used extensively in shared headers. Files like `crc.h` lack extern C guards -- wrap with `extern "C" { #include "crc.h" }` in C++ tests.
- **Interop safety:** Network protocol, file formats, and serialized state must NOT change. Bugs affecting interop with non-OpenMoHAA clients must not be "fixed" without extreme care. Internal engine bugs (crash in math lib, string overflow, etc.) are safe to fix.
- **CVAR system** needs `Cvar_Init()` for standalone testing, which registers `cheats` cvar and requires `Cmd_AddCommand` stubs.
- **MSG buffer** functions depend on Huffman init (`MSG_initHuffman`) and `com_protocol` cvar for protocol version branching. The Pack/Unpack and NegateValue functions are pure and standalone.
- **QVM architecture**: Game modules can run as native shared libs OR QVM bytecode. Don't break either path.
- **Platform endianness:** X86_64/AArch64 are little-endian. The codebase has `Q3_LITTLE_ENDIAN` / `Q3_BIG_ENDIAN` defines. `ShortSwapPtr`/`LongSwapPtr` are no-ops on LE (the name is misleading -- they read little-endian values from pointers).

## Known Bugs Found by Tests

These are safe to fix (internal engine bugs, no interop impact):

1. **`COM_Compress(NULL)`** -- NULL dereference at `q_shared.c:479`. The `if (in)` check guards the loop body but `*out = 0` at line 479 is unconditional. Fix: add `if (!data_p) return 0;` at top.

2. **`PlaneIntersectRay`** -- Division by zero at `q_math.c:588` when ray is parallel to plane (`planeDotRay == 0`). Produces `-inf`/`-nan`. Fix: add `if (planeDotRay == 0.0f) return;` guard.

3. **`Q_strreplace`** -- Buffer overflow at `q_shared.c:1387`. When replacement exceeds `destsize`, the second `strncpy(s + lreplace, ...)` writes past buffer. Fix: check `lstart + lreplace >= destsize` before writing.

4. **`COM_ParseExt`** -- Tokens exceeding `MAX_TOKEN_CHARS` (1024) silently discarded at `q_shared.c:596-600`. Returns empty string with no error. Fix: return partial token or set error flag.

5. **`AngleNormalize360(-720)`** returns 360, not 0. Range is `(0, 360]` instead of `[0, 360)`. Mathematically equivalent but inconsistent.

6. **`BoundsAdd`**, **`ZeroBounds`**, **`ColorBytes3`**, **`ColorBytes4`** -- declared in `q_shared.h` but never implemented. Link fail if called.

7. **`EulerToQuat`** type-puns `float*` to `int*` at `q_math.c:1944` for zero-check. Strict-aliasing UB, works on GCC/Clang with default flags.

8. **`MatToQuat`** potential NaN from `sqrtf(negative)` at `q_math.c:1860` for specific 180-degree rotation matrices (numerical edge case, not triggered for axis-aligned rotations).

## CI / Workflows

- All workflows in `.github/workflows/` support `workflow_dispatch`
- Actions pinned to full commit SHAs (not version tags)
- `unit-testing.yml` runs all CTest tests on ubuntu-latest with clang
- `branches-build.yml` builds Linux/Windows/macOS on every push
- `tags-publish-release.yml` creates GitHub Release on `v*.*.*` tags
- Concurrency cancels in-progress runs on new pushes (except releases)

## Things to NEVER Change (interop boundary)

- Network protocol: `TARGET_GAME_PROTOCOL_MOH` (8), `TARGET_GAME_PROTOCOL_MOHTA` (17), etc.
- Message buffer format (`msg_t` read/write byte layout, Huffman encoding format)
- File formats: PK3/ZIP, BSP, TIKI, SKB, DPK
- CVar values prefixed `com_`, `sv_`, `cl_` that affect net behavior
- Demo file format (`dm_` files)
- Game state serialization (`playerState_s`, `entityState_s`)
