# vmake

A fast, minimal build system for C/C++ projects. vmake tracks file hashes to only recompile what actually changed, scans `#include` directives so header changes trigger the right rebuilds, and compiles in parallel using all your CPU cores by default.

No dependencies. No DSL to learn. Just a clean config file and a single binary.

---

## Features

- **Incremental builds** — hashes every source and header, skips unchanged files
- **Header dependency tracking** — modifying a `.h` file recompiles all `.c` files that include it
- **Parallel compilation** — uses all CPU cores by default, configurable with `-j`
- **Multiple targets** — define as many executables as you need in one config
- **Simple config syntax** — readable, minimal, no XML or CMake dialect

---

## Performance

Benchmarked against `make` building vmake itself (7 source files, clang, `-O3`).

### Full rebuild

| Tool | Time |
|---|---|
| `make` | 1.196s |
| `vmake` | 0.736s |

### Incremental (4 files changed)

| Tool | Time |
|---|---|
| `make` | 0.509s |
| `vmake` | 0.270s |

---

## Tested on real projects

| Project | Files | Result |
|---|---|---|
| [zlib](https://github.com/madler/zlib) | ~20 | ✅ builds successfully |
| [kilo](https://github.com/antirez/kilo) | 1 | ✅ builds and runs |
| vmake itself | 7 | ✅ self-hosting |

---

## Installation

```sh
git clone https://github.com/venoosoo/vmake
cd vmake
make
```

Copy the binary somewhere on your PATH:

```sh
cp build/vmake ~/.local/bin/
```

> Requires Linux and a C compiler (clang or gcc).

---

## Config

Create a `vmake.config` in your project root:

```
executable "myapp" {
    sources = [
        "src/",
    ];
    includes = [
        "include/",
    ];
    output = "build/";
    cc = "clang";
    flags = "-O2 -Wall";
}
```

| Field | Description |
|---|---|
| `sources` | Source files or directories — trailing `/` expands to all `.c` files in that dir |
| `includes` | Include directories, passed as `-I` flags |
| `output` | Output directory for the compiled binary |
| `cc` | Compiler (`clang`, `gcc`, etc.) |
| `flags` | Compiler flags, space-separated |

### Multiple targets

```
executable "engine" {
    sources = [ "engine/src/" ];
    includes = [ "engine/include/" ];
    output = "build/";
    cc = "clang";
    flags = "-O3";
}

executable "tools" {
    sources = [ "tools/src/" ];
    includes = [ "tools/include/" ];
    output = "build/";
    cc = "gcc";
    flags = "-O2 -Wall";
}
```

---

## Usage

```sh
vmake                            # build all targets
vmake --target engine            # build one target
vmake --target engine tools      # build specific targets
vmake -j 4                       # limit to 4 parallel jobs
vmake -f other.config            # use a different config file
vmake -B                         # force full rebuild
vmake -c                         # clean build artifacts and cache
vmake -q                         # quiet (no stdout)
vmake -v                         # verbose (prints debug info)
vmake -V                         # print version
vmake -h                         # print help
```

---

## How it works

1. Parses `vmake.config` and expands any directory entries in `sources`
2. Scans each `.c` file for `#include "..."` directives to build a header → source map
3. Hashes every source and header file with xxHash64
4. Compares hashes against `.vmake_cache` from the last build
5. Recompiles only files that changed — or files that include a changed header
6. Links all object files into the final binary
7. Writes the updated cache to `.vmake_cache` for next run

Object files go in `.vmake/obj/`. The cache lives in `.vmake_cache` at your project root.

---

## .gitignore

```
build/
.vmake/
.vmake_cache
```

---

## Limitations

- Executables only (no static/shared library targets yet)
- No support for linker-only flags (`-lz`, `-lm`, etc.) yet
- No Windows support yet

---

## License

MIT