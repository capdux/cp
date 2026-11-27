# pem — AI Agent Quick Ref

Zero-heap RFC 7468 PEM armor multi-block parser, validator, and 64-column wrapped encoder for certificates and keys (`v1.0.0`, `MIT`)

## Import

```cp
import {
    PemBlock,
    PemBundle,
    isValidLabel,
    parsePemFrom,
    parsePem,
    parsePemBundle,
    extractBase64,
    encodePem,
} from "pem";
```

## Core Types & Constants

```cp
class PemBlock { valid: bool, label: string, raw_body: string, start_offset: usize, /* +1 fields */ }
class PemBundle { blocks: [16]PemBlock, count: usize }
```

## API Signatures

```cp
fn isValidLabel(label: string): bool;
fn parsePemFrom(input: string, from_offset: usize): PemBlock;
fn parsePem(input: string): PemBlock;
fn parsePemBundle(input: string): PemBundle;
fn extractBase64(blk: PemBlock, out: []u8): string;
fn encodePem(label: string, base64_payload: string, out: []u8): string;
```

## Minimal Usage

```cp
import { isValidLabel } from "pem";

pub fn main() {
    let result = isValidLabel("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"pem"`; install via `cap add pem`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
