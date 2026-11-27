# spiffe — AI Agent Quick Ref

Zero-allocation CNCF SPIFFE ID (spiffe://<trust-domain>/<path>) parser, strict security validator, and formatter for CP. (`v1.0.0`, `MIT`)

## Import

```cp
import {
    SpiffeId,
    sidEqBytes,
    sidIsTrustDomainChar,
    sidIsPathSegmentChar,
    isValidTrustDomain,
    isValidPath,
    parseSpiffeId,
    belongsToTrustDomain,
} from "spiffe";
```

## Core Types & Constants

```cp
class SpiffeId { trust_domain: string, path: string, valid: bool }
```

## API Signatures

```cp
fn sidEqBytes(a: &[]u8, b: &[]u8): bool;
fn sidIsTrustDomainChar(c: u8): bool;
fn sidIsPathSegmentChar(c: u8): bool;
fn isValidTrustDomain(td: string): bool;
fn isValidPath(path: string): bool;
fn parseSpiffeId(uri: string): SpiffeId;
fn belongsToTrustDomain(id: SpiffeId, trust_domain: string): bool;
fn formatSpiffeId(trust_domain: string, path: string, out: []u8): usize;
```

## Minimal Usage

```cp
import { isValidTrustDomain } from "spiffe";

pub fn main() {
    let result = isValidTrustDomain("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"spiffe"`; install via `cap add spiffe`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
