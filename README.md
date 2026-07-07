# MarketMakerSimulator🏃‍♀️‍➡️
Financial market streaming and order execution simulator written in C++.

Low latency, data locality, and high performance are paramount.


This project is in a bit of a Frankenstein mode right now where I use it as a catch-all for trading-related applications.
The goal is to break up the repo into its individual core components, 2 for now:
 - An exchange-side simulator that replicates exchange dynamics
 - A market maker simulator to plug into the exchange(s) and process market data with trading applications


<mark>Only tested on Linux</mark>, and it works on any distro. Not tested on MacOS *yet*. There will never be Windows support.

# Requirements / Dependencies
The easiest way to get a known-good environment is by using Nix (see below), but if you prefer system packages, the list below covers what CMake asks for.

Minimum / runtime build deps:
- A C++ compiler with C++23 support (g++ or clang)
- CMake (>= 4.0)
- Ninja (default) or Make
- pkg-config

Additional (for tests / benchmarks / dev):
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

This will drop you into a shell with the proper dependencies. Then build the project:

```bash
make all
```

## Using non-Nix package managers
If you prefer not to use Nix, just install the dependencies shown above for your distro.

Once you have the dependencies, run the Makefile from the project root:

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

# Troubleshooting
- For development you may want additional tools such as `clang-format`, `ccache`, `valgrind`/`memcheck`, or benchmarking tooling available in `benchmarks/`.
- If you run into missing packages or CMake cannot find a library, double-check that the dev/header packages are installed and that `pkg-config` can locate them.
