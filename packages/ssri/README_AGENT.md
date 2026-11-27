# ssri — AI Agent Quick Ref

Zero-allocation W3C Subresource Integrity (SRI) parser, validator, constant-time matcher, and formatter for CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    SriAlgo,
    SriHash,
    SriIntegrity,
    emptyHash,
    isValidAlgoName,
    parseAlgo,
    algoName,
    algoStrength,
} from "ssri";
```

## Core Types & Constants

```cp
enum SriAlgo { Sha256, Sha384, Sha512, Blake3, Unknown }
class SriHash { valid: bool, algo: SriAlgo, algo_name: string, digest: string, /* +1 fields */ }
class SriIntegrity { hashes: [MAX_HASHES]SriHash, count: usize } // methods: get(...) +1
```

## API Signatures

```cp
fn emptyHash(): SriHash;
fn isValidAlgoName(name: string): bool;
fn parseAlgo(name: string): SriAlgo;
fn algoName(algo: SriAlgo): string;
fn algoStrength(algo: SriAlgo): u8;
fn isBase64DataChar(c: u8): bool;
fn isValidBase64(s: string): bool;
fn isValidOptions(opts: string): bool;
fn parseSingleToken(tok: string): SriHash;
fn parseIntegrity(raw: string): SriIntegrity;
fn strongestHash(sri: SriIntegrity): SriHash?;
fn pickAlgorithm(sri: SriIntegrity, algo: SriAlgo): SriHash?;
fn matchesIntegrity(expected: SriIntegrity, actual: SriIntegrity): bool;
fn appendByte(out: []u8, pos: usize, c: u8): usize;
fn formatHash(algo: SriAlgo, base64_digest: string, out: []u8): string;
fn formatIntegrity(sri: SriIntegrity, out: []u8): string;
```

## Minimal Usage

```cp
import { isValidAlgoName } from "ssri";

pub fn main() {
    let result = isValidAlgoName("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"ssri"`; install via `cap add ssri`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
