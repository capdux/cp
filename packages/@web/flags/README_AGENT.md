# @web/flags — AI Agent Quick Ref

Official deterministic FNV-1a Feature Flags & percentage rollout plugin for cap:web (`v1.0.0`, `MIT`)

## Import

```cp
import {
    FlagsConfig,
    FeatureFlag,
    flgStrEql,
    flgInList,
    flagBucket,
    flagReset,
    flagDefine,
    flagEval,
} from "@web/flags";
```

## Core Types & Constants

```cp
class FlagsConfig { status_path: string, user_header: string }
class FeatureFlag { name: string, enabled: bool, rollout_pct: u8, allow_users: string }
```

## API Signatures

```cp
fn flgStrEql(a: string, b: string): bool;
fn flgInList(list: string, target: string): bool;
fn flagBucket(flag_name: string, user_key: string): u8;
fn flagReset();
fn flagDefine(name: string, enabled: bool, rollout_pct: u8, allow_users: string): bool;
fn flagEval(flag_name: string, user_key: string): bool;
fn flgWriteRaw(dst: []u8, pos: &mut usize, s: string): bool;
fn flgWriteU8(dst: []u8, pos: &mut usize, v: u8): bool;
fn handleFlagsStatus(ctx: &mut Context): Response;
fn createFlagsPlugin(cfg: FlagsConfig): Plugin;
fn registerFlags(app: &mut App, cfg: FlagsConfig): &mut App;
```

## Minimal Usage

```cp
import { flgStrEql } from "@web/flags";

pub fn main() {
    let result = flgStrEql("example", "example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"@web/flags"`; install via `cap add @web/flags`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
