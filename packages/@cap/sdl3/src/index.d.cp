// Type definitions for CP

import {  } from "std";
export fn cp_sdl_version(): i32;
export fn cp_sdl_revision(): [*:0]&u8!;
export fn cp_sdl_cpu_count(): i32;
export fn cp_sdl_system_ram(): i32;
export fn cp_sdl_init_headless(): i32;
export fn cp_sdl_quit();
export fn cp_sdl_ticks_ns(): u64;
export error SdlError { InitFailed }

export class SdlVersion {
    major: i32,
    minor: i32,
    micro: i32,
}

export fn rawVersion(): i32;
export fn version(): SdlVersion;
export fn revision(): string;
export fn cpuCount(): i32;
export fn systemRamMB(): i32;
export fn ticksNs(): u64;
export class SdlContext {
    active: bool,
    fn initHeadless(): !SdlContext;
    fn isActive(): bool;
    fn ticksNs(): u64;
    fn __cp_deinit(_: &mut SdlContext);
}

