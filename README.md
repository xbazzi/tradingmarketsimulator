# Trading Market Simulator 🏦

[![GCC Debug](https://gitgud.boo/xbazzi/tradingmarketsimulator/actions/workflows/gcc-debug.yml/badge.svg)](https://gitgud.boo/xbazzi/tradingmarketsimulator/actions)
[![GCC Release](https://gitgud.boo/xbazzi/tradingmarketsimulator/actions/workflows/gcc-release.yml/badge.svg)](https://gitgud.boo/xbazzi/tradingmarketsimulator/actions)
[![Clang Debug](https://gitgud.boo/xbazzi/tradingmarketsimulator/actions/workflows/clang-debug.yml/badge.svg)](https://gitgud.boo/xbazzi/tradingmarketsimulator/actions)
[![GCC](https://img.shields.io/badge/GCC-14-blue?logo=gnu)](https://gcc.gnu.org/)
[![Clang](https://img.shields.io/badge/Clang-21-blue?logo=llvm)](https://clang.llvm.org/)
[![coverage](https://img.shields.io/endpoint?url=https://gitgud.boo/xbazzi/tradingmarketsimulator/raw/branch/badges/coverage.json)](https://gitgud.boo/xbazzi/tradingmarketsimulator/actions)

Trading simulator catch-all repo, for now.

Low latency, data locality, and high performance are paramount.

This project is in a bit of a Frankenstein mode right now with different trading-related components.
The current goal is to break up the repo into smaller pieces, 2 for now:
 - An exchange-side simulator that replicates exchange dynamics
 - A market maker simulator to plug into the exchange(s) and feed structured market data into trading apps

<mark>Only tested on Linux</mark>, and it works on any distro. Not tested on MacOS *yet*. There will never be Windows support.

# Requirements / Dependencies
The easiest way to get a known-good environment is by using Nix (see below), but if you prefer system packages, the list below covers what CMake asks for.

Minimum deps:
- A C++ compiler with C++23 support (g++ or clang)
- CMake (>= 3.25)
- Ninja (default) or Make

Required for running tests/benchmarks:
- Google Test (`find_package(GTest REQUIRED)`)
- Google Benchmark (`find_package(benchmark REQUIRED)`)

Fetched automatically via CMake `FetchContent` (no manual install needed):
- [fastinahurry](https://gitgud.boo/xbazzi/fastinahurry.git)
- [robin-hood-hashing](https://github.com/martinus/robin-hood-hashing)

Again, using the Nix development environment will install all the dependencies and set up the environment variables for you.

Example package install commands:

Debian/Ubuntu:

```bash
sudo apt update
sudo apt install -y build-essential cmake ninja-build pkg-config libgtest-dev libbenchmark-dev
```

Fedora/RHEL/CentOS:

```bash
sudo dnf install -y gcc-c++ cmake ninja-build pkgconfig gtest-devel benchmark-devel
```

Arch:

```bash
sudo pacman -S --needed base-devel cmake ninja pkgconf gtest benchmark
```

# Building
## Using Nix (recommended)
Install nix (and optionally direnv) and run the `flake.nix` to load all dependencies in your local environment:

```bash
nix develop
```

You'll be dropped into a shell with the proper dependencies. Then build the project:

```bash
make all
```

## Using non-Nix package managers
If you prefer not to use Nix, go ahead and install the dependencies shown above for your distro.
Then run the Makefile from the project root:

```bash
make all
```

Available targets:

| Target | Description |
|---|---|
| `make all` | Build all targets |
| `make options` | Build the options executable |
| `make tests` | Build and run unit tests |
| `make tests FILTER=pattern` | Run tests matching a gtest filter pattern |
| `make md_generator` | Build the market data generator binary |
| `make md_consumer` | Build the market data consumer binary |
| `make clean` | Remove the build directory |
| `make help` | Print all available targets |

# The benefits of Nix
If you have direnv, run `direnv allow` and enjoy having your packages loaded automatically when you `cd` into the repo; no need to run `nix develop` every time you're in a new shell or to switch shells for multiple projects.

Switching to `devenv` soon.

# Troubleshooting
- For development you may want additional tools such as `clang-format`, `ccache`, `valgrind`/`memcheck`, or benchmarking tooling available in `benchmarks/`.
- If you run into missing packages or CMake cannot find a library, double-check that the dev/header packages are installed and that `pkg-config` can locate them.
