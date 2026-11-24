# `@cap/llama`

High-performance local LLM inference engine for CP, powered by **`ggml-org/llama.cpp` (`b12676`, MIT License)** using official prebuilt dynamic libraries (`sys.llama.prebuilt`) with zero local C++ build time.

## Architecture

- **Prebuilt Dynamic Libraries**: Links directly against official `libllama.so` + `libggml*.so` (`prebuilt/<os>-<arch>/`), preserving `SONAME` symlinks (`libllama.so -> libllama.so.1 -> libllama.so.1.0.0`, `libggml.so*`, `libggml-base.so*`) and dynamic CPU backend plugins (`libggml-cpu-*.so`). `cap run` injects `LD_LIBRARY_PATH` / `DYLD_LIBRARY_PATH` automatically.
- **Thin C ABI Shim (`c/llama_ffi.c`)**: ~45-line bridge isolating CP from version-sensitive `llama_model_params` struct layout, compiled in <0.2s into `libllama_shim.a`.
- **Zero-Leak RAII (`src/index.cp`)**: Provides `LlamaBackend`, `LlamaModel`, `systemInfo()`, `supportsMmap()`, `supportsMlock()`, `supportsGpuOffload()`, `maxDevices()`, and `timeUs()` with deterministic `~init()` cleanup.

## Downloading Official Prebuilt Binaries

Official prebuilt shared libraries are distributed via GitHub Releases of [`ggml-org/llama.cpp`](https://github.com/ggml-org/llama.cpp/releases/tag/b12676) (MIT License) and are excluded from Git tracking (`prebuilt/` is gitignored):

- Release Tag: `b12676`
- Linux x86_64 Archive: `https://github.com/ggml-org/llama.cpp/releases/download/b12676/llama-b12676-bin-ubuntu-x64.tar.gz`

Example setup on Linux x86_64:

```bash
curl -fSL -o /tmp/llama.tar.gz \
  https://github.com/ggml-org/llama.cpp/releases/download/b12676/llama-b12676-bin-ubuntu-x64.tar.gz
tar -xzf /tmp/llama.tar.gz -C /tmp
mkdir -p prebuilt/linux-x86_64
cp -a /tmp/llama-b12676/libllama.so* /tmp/llama-b12676/libggml*.so* prebuilt/linux-x86_64/
find prebuilt/linux-x86_64 -maxdepth 1 -type f -name "*.so*" -exec strip --strip-unneeded {} +
```

## Quick Start

```cp
import { LlamaBackend, systemInfo, supportsMmap, supportsGpuOffload, maxDevices, timeUs } from "@cap/llama";
import std from "std";

pub fn main(): void! {
    let backend = LlamaBackend.init();
    std.debug.print("active={} maxDevices={} mmap={} gpu={} timeUs={}\n", .{
        backend.isActive(),
        maxDevices(),
        supportsMmap(),
        supportsGpuOffload(),
        timeUs(),
    });
    std.debug.print("systemInfo: {s}\n", .{systemInfo()});
}
```
