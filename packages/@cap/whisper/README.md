# `@cap/whisper`

High-performance local speech-to-text (ASR) engine for CP, powered by **`ggml-org/whisper.cpp` (`v1.9.6`, MIT License)** using prebuilt dynamic libraries (`sys.whisper.prebuilt`) with zero local C++ build time for consumers.

## Architecture

- **Prebuilt Dynamic Libraries**: Links directly against `libwhisper.so` + `libggml*.so` (`prebuilt/<os>-<arch>/`), preserving `SONAME` symlinks (`libwhisper.so -> libwhisper.so.1 -> libwhisper.so.1.9.6`, `libggml.so*`, `libggml-base.so*`, `libggml-cpu.so*`). `cap run` injects `LD_LIBRARY_PATH` / `DYLD_LIBRARY_PATH` automatically.
- **Thin C ABI Shim (`c/whisper_ffi.c`)**: ~45-line bridge isolating CP from version-sensitive `whisper_context_params` struct layout, compiled in <0.2s into `libwhisper_shim.a`.
- **Zero-Leak RAII (`src/index.cp`)**: Provides `WhisperContext` (with `initFromFile`, `isValid`, `nVocab`, and deterministic `~init()` cleanup), `versionString()`, `systemInfo()`, `langMaxId()`, `langId(code)`, and `langStr(id)`.

## Building Prebuilt Binaries (Linux x86_64)

`prebuilt/` is excluded from Git tracking. To build `libwhisper.so` and `libggml*.so` from upstream [`ggml-org/whisper.cpp`](https://github.com/ggml-org/whisper.cpp/releases/tag/v1.9.6) (`v1.9.6`, MIT License):

```bash
git clone --depth 1 --branch v1.9.6 https://github.com/ggml-org/whisper.cpp.git /tmp/whisper.cpp
cmake -S /tmp/whisper.cpp -B /tmp/whisper.cpp/build -G Ninja \
  -DBUILD_SHARED_LIBS=ON \
  -DWHISPER_BUILD_TESTS=OFF \
  -DWHISPER_BUILD_EXAMPLES=OFF \
  -DCMAKE_BUILD_TYPE=Release \
  -DGGML_NATIVE=OFF
cmake --build /tmp/whisper.cpp/build -j$(nproc)
mkdir -p prebuilt/linux-x86_64
cp -a /tmp/whisper.cpp/build/src/libwhisper.so* /tmp/whisper.cpp/build/ggml/src/libggml*.so* prebuilt/linux-x86_64/
find prebuilt/linux-x86_64 -maxdepth 1 -type f -name "*.so*" -exec strip --strip-unneeded {} +
```

## Quick Start

```cp
import { WhisperContext, versionString, systemInfo, langMaxId, langId, langStr } from "@cap/whisper";
import std from "std";

pub fn main(): void! {
    std.debug.print("version={s} langMaxId={} zh={} en={} lang0={s}\n", .{
        versionString(),
        langMaxId(),
        langId("zh"),
        langId("en"),
        langStr(0),
    });
    std.debug.print("systemInfo: {s}\n", .{systemInfo()});
}
```
