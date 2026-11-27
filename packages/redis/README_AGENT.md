# redis — AI Agent Quick Ref

Zero-allocation Redis / Valkey / Sentinel connection URL parser (redis://, rediss://, unix://) and password-redacting formatter for CP. (`v1.0.0`, `MIT`)

## Import

```cp
import {
    RedisConfig,
    ruStartsWithIgnoreCase,
    ruParseStrictU16,
    ruWrite,
    ruWriteU16,
    parseRedisUrl,
    formatRedisUrl,
} from "redis";
```

## Core Types & Constants

```cp
class RedisConfig { tls: bool, is_unix: bool, host: string, port: u16, /* +5 fields */ }
```

## API Signatures

```cp
fn ruStartsWithIgnoreCase(s: string, prefix: string): bool;
fn ruParseStrictU16(s: string): u16?;
fn ruWrite(out: []u8, pos: &mut usize, src: string): bool;
fn ruWriteU16(out: []u8, pos: &mut usize, val: u16): bool;
fn parseRedisUrl(url: string): RedisConfig;
fn formatRedisUrl(cfg: RedisConfig, redact_password: bool, out: []u8): usize;
```

## Minimal Usage

```cp
import { parseRedisUrl } from "redis";

pub fn main() {
    let result = parseRedisUrl("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"redis"`; install via `cap add redis`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
