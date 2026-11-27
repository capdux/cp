# vcard — AI Agent Quick Ref

Zero-allocation RFC 6350 vCard 4.0 (.vcf) contact generator and parser for CP. (`v1.0.0`, `MIT`)

## Import

```cp
import {
    VCardData,
    ParsedVCard,
    isSpaceOrTab,
    writeRaw,
    writeEscaped,
    writeProp,
    formatVCard,
    parseVCard,
} from "vcard";
```

## Core Types & Constants

```cp
class VCardData { full_name: &[]u8, org: &[]u8, title: &[]u8, email: &[]u8, /* +2 fields */ }
class ParsedVCard { fn_start: usize, fn_len: usize, org_start: usize, org_len: usize, /* +11 fields */ }
```

## API Signatures

```cp
fn isSpaceOrTab(c: u8): bool;
fn writeRaw(dst: []u8, pos: &mut usize, src: &[]u8): bool;
fn writeEscaped(dst: []u8, pos: &mut usize, src: &[]u8): bool;
fn writeProp(dst: []u8, pos: &mut usize, propPrefix: &[]u8, val: &[]u8): bool;
fn formatVCard(card: VCardData, dst: []u8): i64;
fn parseVCard(vcf: &[]u8): ParsedVCard;
```

## Minimal Usage

```cp
import { isSpaceOrTab } from "vcard";

pub fn main() {
    let result = isSpaceOrTab(0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"vcard"`; install via `cap add vcard`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
