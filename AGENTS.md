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
| `test_deferredsave` | `code/qcommon/tests/test_deferredsave.cpp` | CSVG compression round-trip, buffer detach semantics |

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

9. **Save game freeze** -- LZ77 compression and synchronous disk writes in the save path (`SV_SaveGame` → `G_ArchiveLevel` → `Archiver::Close` → `ArchiveFile::Compress` + `FS_WriteFile`) block the main thread for 50-200ms, causing the game to freeze. Fix: split the save into a fast serialization phase (game paused briefly) and deferred compression+write phases processed across subsequent frames in `G_RunFrame`. Added `ArchiveFile::DetachBuffer()`, `Archiver::CloseDeferred()`, and a `DeferredSave_Flush()` phase machine. New test in `test_deferredsave` validates the CSVG compression format round-trip and buffer detach semantics.

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
