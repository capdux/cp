# env-paths — AI Agent Quick Ref

Zero-allocation cross-platform application directory resolver (Linux XDG, macOS Library, Windows AppData) in pure CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    EnvPlatform,
    EnvPathsInput,
    evpTrimTrailingSep,
    evpAppend,
    evpAppendByte,
    evpAppendAppName,
    evpJoinUnix2,
    evpJoinUnix3,
} from "env-paths";
```

## Core Types & Constants

```cp
enum EnvPlatform { Linux, MacOS, Windows }
class EnvPathsInput { platform: EnvPlatform, home: string, xdg_data_home: string, xdg_config_home: string, /* +6 fields */ } // methods: new(...) +3
```

## API Signatures

```cp
fn evpTrimTrailingSep(s: string, sep: u8): string;
fn evpAppend(out: []u8, pos: usize, s: string): usize;
fn evpAppendByte(out: []u8, pos: usize, b: u8): usize;
fn evpAppendAppName(out: []u8, pos: usize, name: string, suffix: string): usize;
fn evpJoinUnix2(out: []u8, base: string, name: string, suffix: string): usize;
fn evpJoinUnix3(out: []u8, base: string, mid: string, name: string, suffix: string): usize;
fn evpWinAppBase(out: []u8, explicit: string, home: string, fallback_sub: string): usize;
fn evpWinDir(out: []u8, explicit: string, home: string, fallback_sub: string, name: string, suffix: string, leaf: string): usize;
fn envDataDir(name: string, inp: EnvPathsInput, out: []u8): usize;
fn envConfigDir(name: string, inp: EnvPathsInput, out: []u8): usize;
fn envCacheDir(name: string, inp: EnvPathsInput, out: []u8): usize;
fn envLogDir(name: string, inp: EnvPathsInput, out: []u8): usize;
fn envTempDir(name: string, inp: EnvPathsInput, out: []u8): usize;
```

## Minimal Usage

```cp
import { evpTrimTrailingSep } from "env-paths";

pub fn main() {
    let result = evpTrimTrailingSep("example", 0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"env-paths"`; install via `cap add env-paths`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
