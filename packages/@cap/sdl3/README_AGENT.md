# @cap/sdl3@3.2.8 (Agent Quickref)

> SDL3 (v3.2.8) cross-platform window, audio, input & GPU abstraction for CP via prebuilt dynamic library | License: Zlib
> Import: `import { ... } from "@cap/sdl3";` | Full types: `src/index.d.cp`

## Functions
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
