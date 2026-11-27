# ldapjs — AI Agent Quick Ref

Zero-heap RFC 4514 LDAP Distinguished Name parser, value escaper/unescaper, validator, and case-insensitive attribute lookup. (`v1.0.0`, `MIT`)

## Import

```cp
import {
    RdnAttr,
    isWs,
    isValidAttrType,
    isEscapedAt,
    hasValidEscapes,
    parseDn,
    escapeValue,
    unescapeValue,
} from "ldapjs";
```

## Core Types & Constants

```cp
class RdnAttr { attr: string, raw_value: string }
```

## API Signatures

```cp
fn isWs(c: u8): bool;
fn isValidAttrType(attr: string): bool;
fn isEscapedAt(s: string, start: usize, idx: usize): bool;
fn hasValidEscapes(raw: string): bool;
fn parseDn(dn: string, out: []RdnAttr): usize;
fn escapeValue(value: string, out: []u8): usize;
fn unescapeValue(raw: string, out: []u8): usize;
fn findAttr(dn: string, target_attr: string, out: []u8, out_len: &mut usize): bool;
fn isValidDn(dn: string): bool;
```

## Minimal Usage

```cp
import { isValidAttrType } from "ldapjs";

pub fn main() {
    let result = isValidAttrType("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"ldapjs"`; install via `cap add ldapjs`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
