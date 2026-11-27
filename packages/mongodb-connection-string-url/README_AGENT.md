# mongodb-connection-string-url — AI Agent Quick Ref

Zero-allocation MongoDB connection string (mongodb:// & mongodb+srv://) parser, option extractor, and password-redacting formatter for CP. (`v1.0.0`, `MIT`)

## Import

```cp
import {
    MongoHost,
    MongoUri,
    muStartsWithIgnoreCase,
    muParsePort,
    muWrite,
    muWriteU16,
    muEmptyResult,
    parseMongoUri,
} from "mongodb-connection-string-url";
```

## Core Types & Constants

```cp
class MongoHost { host: string, port: u16 }
class MongoUri { is_srv: bool, username: string, password: string, hosts: [8]MongoHost, /* +6 fields */ }
```

## API Signatures

```cp
fn muStartsWithIgnoreCase(s: &[]u8, prefix: &[]u8): bool;
fn muParsePort(s: &[]u8, out: &mut u16): bool;
fn muWrite(out: []u8, pos: &mut usize, src: &[]u8): bool;
fn muWriteU16(out: []u8, pos: &mut usize, val: u16): bool;
fn muEmptyResult(): MongoUri;
fn parseMongoUri(uri: string): MongoUri;
fn getQueryOption(uri: string, key: string): string;
fn formatMongoUri(cfg: MongoUri, redact_password: bool, out: []u8): usize;
```

## Minimal Usage

```cp
import { parseMongoUri } from "mongodb-connection-string-url";

pub fn main() {
    let result = parseMongoUri("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"mongodb-connection-string-url"`; install via `cap add mongodb-connection-string-url`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
