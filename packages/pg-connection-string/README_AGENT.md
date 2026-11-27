# pg-connection-string — AI Agent Quick Ref

Zero-heap PostgreSQL connection string parser and URI builder supporting both postgresql:// URIs and libpq key=value DSNs. (`v1.0.0`, `MIT`)

## Import

```cp
import {
    PgConfig,
    makeInvalid,
    isWs,
    parsePortStrict,
    isValidSslMode,
    parseUri,
    parseKeyValue,
    parse,
} from "pg-connection-string";
```

## Core Types & Constants

```cp
class PgConfig { host: string, port: u16, user: string, password: string, /* +3 fields */ }
```

## API Signatures

```cp
fn makeInvalid(): PgConfig;
fn isWs(c: u8): bool;
fn parsePortStrict(s: string): i32;
fn isValidSslMode(mode: string): bool;
fn parseUri(uri: string): PgConfig;
fn parseKeyValue(dsn: string): PgConfig;
fn parse(input: string): PgConfig;
fn appendSlice(out: []u8, pos: usize, s: string): i64;
fn appendU16(out: []u8, pos: usize, v: u16): i64;
fn formatUri(cfg: PgConfig, include_password: bool, out: []u8): usize;
```

## Minimal Usage

```cp
import { parsePortStrict } from "pg-connection-string";

pub fn main() {
    let result = parsePortStrict("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"pg-connection-string"`; install via `cap add pg-connection-string`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
