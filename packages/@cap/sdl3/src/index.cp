// @cap/sdl3 — SDL3 (v3.2.8) Cross-Platform Multimedia, Window, Audio & Input Abstraction for CP
// License: Zlib (Upstream: libsdl-org/SDL)

import std from "std";

pub @c {
    fn cp_sdl_version(): i32;
    fn cp_sdl_revision(): [*:0]const u8;
    fn cp_sdl_cpu_count(): i32;
    fn cp_sdl_system_ram(): i32;
    fn cp_sdl_init_headless(): i32;
    fn cp_sdl_quit();
    fn cp_sdl_ticks_ns(): u64;
}

pub err SdlError {
    InitFailed,
}

/// Decoded SDL3 semantic version (major.minor.micro).
pub struct SdlVersion {
    pub major: i32,
    pub minor: i32,
    pub micro: i32,
}

/// Returns the raw packed SDL3 version integer (`major * 1000000 + minor * 1000 + micro`).
pub fn rawVersion(): i32 {
    unsafe {
        return cp_sdl_version();
    }
}

/// Returns the decoded SDL3 semantic version.
pub fn version(): SdlVersion {
    const v = rawVersion();
    return SdlVersion{
        .major = @divTrunc(v, 1000000),
        .minor = @mod(@divTrunc(v, 1000), 1000),
        .micro = @mod(v, 1000),
    };
}

/// Returns the linked SDL3 build revision string.
pub fn revision(): string {
    unsafe {
        return std.mem.span(cp_sdl_revision());
    }
}

/// Returns the number of logical CPU cores detected by SDL3.
pub fn cpuCount(): i32 {
    unsafe {
        return cp_sdl_cpu_count();
    }
}

/// Returns the amount of RAM configured in the system, in MiB.
pub fn systemRamMB(): i32 {
    unsafe {
        return cp_sdl_system_ram();
    }
}

/// Returns the number of nanoseconds since SDL library initialization.
pub fn ticksNs(): u64 {
    unsafe {
        return cp_sdl_ticks_ns();
    }
}

/// Owned RAII guard for an initialized SDL3 context.
/// Automatically calls `SDL_Quit()` on scope exit via `~init()`.
pub struct SdlContext {
    active: bool = false,

    /// Initializes SDL3 in headless-safe mode (`SDL_INIT_EVENTS`).
    pub static fn initHeadless(): SdlContext! {
        let ok: i32 = 0;
        unsafe {
            ok = cp_sdl_init_headless();
        }
        if (ok == 0) {
            return SdlError.InitFailed;
        }
        return SdlContext{ .active = true };
    }

    /// Returns true if this SDL3 context is currently initialized and active.
    pub fn isActive(): bool {
        return _.active;
    }

    /// Returns elapsed nanoseconds since SDL initialization.
    pub fn ticksNs(): u64 {
        unsafe {
            return cp_sdl_ticks_ns();
        }
    }

    ~init() {
        if (_.active) {
            _.active = false;
            unsafe {
                cp_sdl_quit();
            }
        }
    }
}
