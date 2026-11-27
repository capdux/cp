[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：吸收自 **c** [`SDL`](https://github.com/libsdl-org/SDL)（`Zlib`）（原库名 `SDL`，CP 包名 `@cap/sdl3`）。

# @cap/sdl3

SDL3 (v3.2.8) cross-platform window, audio, input, and GPU abstraction for CP, distributed with a prebuilt dynamic library (`libSDL3.so`) and a lightweight C ABI shim for zero local build overhead.

- **Upstream**: [libsdl-org/SDL](https://github.com/libsdl-org/SDL) (`release-3.2.8`)
- **License**: Zlib

## Usage

```cp
import { version, cpuCount, systemRamMB, SdlContext } from "@cap/sdl3";

pub fn main(): void! {
    const v = version();
    const ctx = SdlContext.initHeadless()?;
    // Automatically calls SDL_Quit() via ~init() on scope exit
}
```
