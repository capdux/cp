[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：吸收自 **c++** [`onnxruntime`](https://github.com/microsoft/onnxruntime)（`MIT`）（原库名 `onnxruntime`，CP 包名 `@cap/onnx`）。

# `@cap/onnx`

High-performance cross-platform ML inference engine for CP, powered by **Microsoft ONNX Runtime v1.32.0** (MIT License) using official prebuilt shared libraries (`sys.onnxruntime.prebuilt`) with zero local C++ build time.

## Architecture

- **Prebuilt Dynamic Library**: Links directly against Microsoft's official `libonnxruntime` (`prebuilt/<os>-<arch>/libonnxruntime.{so,dylib,dll}`), with automatic runtime `LD_LIBRARY_PATH` / `DYLD_LIBRARY_PATH` injection by `cap run`.
- **Thin C ABI Shim (`c/onnx_ffi.c`)**: ~50-line bridge isolating CP from the large version-sensitive `OrtApi` function-table struct layout, compiled in <0.2s into `libonnx_shim.a`.
- **Zero-Leak RAII (`src/index.cp`)**: Provides `OrtEnv`, `OrtSessionOptions`, and `versionString()` with deterministic `~init()` cleanup.

## Downloading Official Prebuilt Binaries

Official prebuilt shared libraries are distributed via Microsoft's official NuGet package `Microsoft.ML.OnnxRuntime` (`v1.32.0`, MIT License) and are excluded from Git tracking (`prebuilt/` is gitignored):

- NuGet Archive: `https://api.nuget.org/v3-flatcontainer/microsoft.ml.onnxruntime/1.32.0/microsoft.ml.onnxruntime.1.32.0.nupkg`

| Target | Path Inside `.nupkg` | Destination Path |
| --- | --- | --- |
| `linux-x86_64` | `runtimes/linux-x64/native/libonnxruntime.so` | `prebuilt/linux-x86_64/libonnxruntime.so` |
| `linux-aarch64` | `runtimes/linux-arm64/native/libonnxruntime.so` | `prebuilt/linux-aarch64/libonnxruntime.so` |
| `macos-aarch64` | `runtimes/osx-arm64/native/libonnxruntime.dylib` | `prebuilt/macos-aarch64/libonnxruntime.dylib` |
| `macos-x86_64` | `runtimes/osx-x64/native/libonnxruntime.dylib` | `prebuilt/macos-x86_64/libonnxruntime.dylib` |
| `windows-x86_64` | `runtimes/win-x64/native/onnxruntime.dll` | `prebuilt/windows-x86_64/onnxruntime.dll` |

Example setup on Linux x86_64:

```bash
curl -sSL -o /tmp/ort.nupkg https://api.nuget.org/v3-flatcontainer/microsoft.ml.onnxruntime/1.32.0/microsoft.ml.onnxruntime.1.32.0.nupkg
unzip -j -o /tmp/ort.nupkg "build/native/include/onnxruntime_*.h" -d include/
unzip -j -o /tmp/ort.nupkg "runtimes/linux-x64/native/libonnxruntime.so" -d prebuilt/linux-x86_64/
strip --strip-unneeded prebuilt/linux-x86_64/libonnxruntime.so
ln -sf libonnxruntime.so prebuilt/linux-x86_64/libonnxruntime.so.1
```

## Quick Start

```cp
import { OrtEnv, OrtSessionOptions, versionString } from "@cap/onnx";
import std from "std";

pub fn main(): void! {
    std.debug.print("ONNX Runtime: {s}\n", .{versionString()});
    let env = try OrtEnv.create("cp_agent");
    let opts = try OrtSessionOptions.create();
    try opts.setIntraOpThreads(4);
    std.debug.print("env_valid={} opts_valid={}\n", .{env.isValid(), opts.isValid()});
}
```
