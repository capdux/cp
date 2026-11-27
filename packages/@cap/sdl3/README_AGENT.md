# @cap/sdl3 — AI Agent Quick Ref

SDL3 (v3.2.8) cross-platform window, audio, input & GPU abstraction for CP via prebuilt dynamic library (`v3.2.8`, `Zlib`)

## Import

```cp
import {
    SdlError,
    SdlVersion,
    SdlContext,
    cp_sdl_version,
    cp_sdl_revision,
    cp_sdl_cpu_count,
    cp_sdl_system_ram,
    cp_sdl_init_headless,
} from "@cap/sdl3";
```

## Core Types & Constants

```cp
error SdlError { InitFailed }
class SdlVersion { major: i32, minor: i32, micro: i32 }
class SdlContext { active: bool } // methods: initHeadless(...) +3
```

## API Signatures

```cp
fn cp_sdl_version(): i32;
fn cp_sdl_revision(): [*:0]&u8!;
fn cp_sdl_cpu_count(): i32;
fn cp_sdl_system_ram(): i32;
fn cp_sdl_init_headless(): i32;
fn cp_sdl_quit();
fn cp_sdl_ticks_ns(): u64;
fn rawVersion(): i32;
fn version(): SdlVersion;
fn revision(): string;
fn cpuCount(): i32;
fn systemRamMB(): i32;
fn ticksNs(): u64;
```

## Minimal Usage

```cp
import { version } from "@cap/sdl3";

pub fn main() {
    let result = version();
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"@cap/sdl3"`; install via `cap add @cap/sdl3`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Error unions (`!T`)**: Handle fallible calls with `try`, `catch`, or `?` error propagation (CP has no `try/catch` blocks).
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
- **Native FFI safety**: Ensure C pointers are non-null and released deterministically via `defer` or `__cp_deinit`.
