[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# @cap/sdl3

> 版本：`3.2.8` | 许可证：`Zlib` | 类型声明：[src/index.d.cp](./src/index.d.cp)

SDL3 (v3.2.8) cross-platform window, audio, input & GPU abstraction for CP via prebuilt dynamic library

## 安装与导入

```bash
cap add @cap/sdl3
```

```cp
import { cp_sdl_version } from "@cap/sdl3";
```

## 核心导出 API

- `export fn cp_sdl_version(): i32;`
- `export fn cp_sdl_revision(): [*:0]&u8!;`
- `export fn cp_sdl_cpu_count(): i32;`
- `export fn cp_sdl_system_ram(): i32;`
- `export fn cp_sdl_init_headless(): i32;`
- `export fn cp_sdl_quit();`
- `export fn cp_sdl_ticks_ns(): u64;`
- `export fn rawVersion(): i32;`
- `export fn version(): SdlVersion;`
- `export fn revision(): string;`
- `export fn cpuCount(): i32;`
- `export fn systemRamMB(): i32;`
- `export fn ticksNs(): u64;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`c` (`SDL@3.2.8`)，许可证：Zlib
