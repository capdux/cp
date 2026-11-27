# mysql2 — AI Agent Quick Ref

Zero-heap MySQL / MariaDB / TiDB connection string parser (mysql:// URI & Go-style user:pass@tcp(host:port)/db) with password redaction for CP (compatible with npm mysql2 & Rust mysql) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    MysqlConfig,
    eqBytes,
    startsWith,
    parsePort,
    indexOf,
    lastIndexOf,
    applyQueryParams,
    splitHostPort,
} from "mysql2";
```

## Core Types & Constants

```cp
class MysqlConfig { tls: bool, host: string, port: u16, user: string, /* +4 fields */ }
```

## API Signatures

```cp
fn eqBytes(a: string, b: string): bool;
fn startsWith(s: string, prefix: string): bool;
fn parsePort(s: string): i32;
fn indexOf(s: string, ch: u8): i64;
fn lastIndexOf(s: string, ch: u8): i64;
fn applyQueryParams(query: string, cfg: &mut MysqlConfig);
fn splitHostPort(hp: string, out_host: &mut string, out_port: &mut u16): bool;
fn parseUri(uri: string): MysqlConfig;
fn parseGoDsn(dsn: string): MysqlConfig;
fn parse(input: string): MysqlConfig;
fn appendStr(dst: []u8, pos: usize, src: string): usize;
fn appendU16(dst: []u8, pos: usize, val: u16): usize;
fn formatUri(cfg: MysqlConfig, redact_password: bool, out: []u8): usize;
```

## Minimal Usage

```cp
import { parsePort } from "mysql2";

pub fn main() {
    let result = parsePort("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"mysql2"`; install via `cap add mysql2`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
