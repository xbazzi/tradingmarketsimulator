# FastInAHurry🏃‍♀️‍➡️
Financial market streaming and order execution simulator written in C++.

Low latency, data locality, and high performance are paramount.

<mark>Only tested on Linux</mark>, and it works on any distro. Not tested on MacOS *yet*. There will never be Windows support.

# Requirements / Dependencies
This project requires a modern C++ toolchain and several libraries that the build system expects. The easiest way to get a known-good environment is by using Nix (see below), but if you prefer system packages the list below covers what CMake asks for.

Minimum / runtime build deps:
- A C++ compiler with C++23 support (g++ or clang)
- CMake (>= 3.15)
- Make or Ninja
- pkg-config
- Boost
- OpenSSL development headers
- nlohmann_json (CMake: `find_package(nlohmann_json REQUIRED)`)
- toml11 (CMake: `find_package(toml11 REQUIRED)`) — package name may vary per distro

Additional (for tests / benchmarks / dev):
- Google Test (`find_package(GTest REQUIRED)`)
- Google Benchmark (`find_package(benchmark REQUIRED)`)
- quicklib — pulled via CMake FetchContent; make sure a copy of quicklib is available or adjust the FetchContent SOURCE_DIR in `CMakeLists.txt` (the project grabs it from GitHub by default)

Notes:
- `GTest` and `benchmark` are only required if you build and run `all`, `unit_tests` or `benchmarks` targets.
- `toml11` package names vary; if your distro doesn't provide it, install from the upstream repo or add a small header-only copy.
- `quicklib` is available at `https://github.com/xbazzi/quicklib.git`. 

Again, using the Nix development environment will install all the dependencies and set up the environment variables for you.

Example package install commands:

Debian/Ubuntu:

```bash
sudo apt update
sudo apt install -y build-essential cmake ninja-build pkg-config \
  libssl-dev libboost-all-dev nlohmann-json3-dev libgtest-dev libbenchmark-dev
# toml11 may not be packaged; install from source if needed
```

Fedora/RHEL/CentOS:

```bash
sudo dnf install -y gcc-c++ cmake ninja-build pkgconfig \
  openssl-devel boost-devel nlohmann-json-devel gtest-devel benchmark-devel
# toml11 may not be packaged; install from source if needed
```

Arch:

```bash
sudo pacman -S --needed base-devel cmake ninja openssl boost pkgconf nlohmann-json gtest benchmark
# toml11 may not be packaged; install from source if needed
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

For a help printout of all targets, run `make help`.

# Running
First, run the server:

```bash
./build/bin/server &
```

Then run the client, preferably in another terminal so the output doesn't get interleaved with the server's:

```bash
./build/bin/client
```

# The benefits of Nix
If you have direnv, run `direnv allow` and enjoy having your packages loaded automatically when you `cd` into the repo; no need to run `nix develop` every time you're in a new shell or to switch shells for multiple projects.

# Troubleshooting
- For development you may want additional tools such as `clang-format`, `ccache`, `valgrind`/`memcheck`, or benchmarking tooling available in `benchmarks/`.
- If you run into missing packages or CMake cannot find a library, double-check that the dev/header packages are installed and that `pkg-config` can locate them.
