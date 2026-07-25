# OpenMoHAA (Sudo-Ivan fork)

## What is OpenMoHAA?

OpenMoHAA is an open-source project aimed at preserving and enhancing **Medal of Honor: Allied Assault** (including Spearhead and Breakthrough expansions) by providing more features and bugfixes, across modern platforms and architectures.

Powered by [ioquake3](https://github.com/ioquake/ioq3) and the [F.A.K.K SDK](https://code.idtech.space/ritual/fakk2-sdk), OpenMoHAA provides:
- Full compatibility with the original game: assets, scripts and multiplayer
- Better support for modern systems
- Cross-platform support (Linux, Windows, macOS)
- Support for both single-player and multiplayer modes
- Includes all fixes from Spearhead 2.15 and Breakthrough 2.40b
- More fixes and features, such as bots and a ban system

## Our changes

- Added `workflow_dispatch` manual trigger to all GitHub Actions workflows
- Hardened OOB connect path and added netsec tests

*OpenMoHAA is an independent project and is not affiliated with or endorsed by Electronic Arts.*

## Getting started

- [Installing OpenMoHAA](docs/markdown/01-intro/01-installation.md)
- [How to play: Launching the game, expansions & file locations](docs/markdown/02-running/01-running.md)
- [FAQ & Troubleshooting](docs/markdown/02-running/03-faq.md)
- [Setting up a game server](docs/markdown/02-running/02-running-server.md)

## Current state

- [List of differences](docs/markdown/01-intro/04-differences.md)

### Single-player

The entire single-player campaign should work (Allied Assault, Spearhead and Breakthrough).

### Multiplayer

- Almost fully stable
- All official game modes are supported, including those from Spearhead and Breakthrough:
  - Free-For-All
  - Team-Deathmatch
  - Round-based match
  - Objective match
  - Tug-of-War (Spearhead)
  - Liberation (Breakthrough)
- Popular mods like **Freeze-Tag** are supported
- Built-in bots for offline practice and for testing
  - [Setting up bots](docs/markdown/02-running/01-running.md#Playing-with-bots)

You can host your own [OpenMoHAA server](docs/markdown/02-running/02-running-server.md#) or join others using OpenMoHAA.

## Screenshots

|                                                                                   |                                                                            |
|-----------------------------------------------------------------------------------|----------------------------------------------------------------------------|
| ![](docs/assets/images/v0.60.0-x86_64/mohdm1_1.png)                                      | ![](docs/assets/images/v0.60.0-x86_64/training_1.png)                               |
| ![](docs/assets/images/v0.60.0-x86_64/flughafen_1.png)                                   | ![](docs/assets/images/v0.60.0-x86_64/flughafen_2.png)                            |
| ![](docs/assets/images/v0.60.0-x86_64/mohdm2_1.png "Playing Freeze-Tag mode with bots")  | ![](docs/assets/images/v0.60.0-x86_64/training_3.png "Single-Player training")    |

*More screenshots [here](docs/assets/images)*

## Development & Compiling

- [Building from source](docs/markdown/04-coding/01-compiling.md)

## Additional documentation

- [Game settings & configuration](docs/markdown/03-configuration/01-configuration.md)
- [Code & Scripting reference](docs/markdown/04-coding/02-coding.md)

## Third party librairies

The following third party tools and libraries are used by the project

- [Flex](https://github.com/westes/flex)
- [Bison](https://savannah.gnu.org/projects/bison/)
- [SDL](http://www.libsdl.org/)
- [OpenAL](https://www.openal.org/)
- [LibMAD](http://www.underbit.com/products/mad/)
- [cURL](https://curl.se/)
- [Libogg](https://github.com/gcp/libogg)
- [Libvorbis](https://xiph.org/vorbis/)
- [Libopus](https://opus-codec.org/)
