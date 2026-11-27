# xml-name-validator — AI Agent Quick Ref

Zero-allocation W3C XML 1.0 (5th Ed.) & Namespaces in XML 1.0 Char, Name, NCName, and QName validator in pure CP (`v1.0.0`, `Apache-2.0`)

## Import

```cp
import {
    XnvDecoded,
    xnvDecodeUtf8,
    xnvValidateName,
    isXmlChar,
    isNameStartChar,
    isNameChar,
    isValidXmlName,
    isValidNCName,
} from "xml-name-validator";
```

## Core Types & Constants

```cp
class XnvDecoded { cp: u32, len: usize, ok: bool }
```

## API Signatures

```cp
fn xnvDecodeUtf8(s: string, pos: usize): XnvDecoded;
fn xnvValidateName(s: string, allow_colon: bool): bool;
fn isXmlChar(cp: u32): bool;
fn isNameStartChar(cp: u32): bool;
fn isNameChar(cp: u32): bool;
fn isValidXmlName(s: string): bool;
fn isValidNCName(s: string): bool;
fn isValidQName(s: string): bool;
```

## Minimal Usage

```cp
import { isValidXmlName } from "xml-name-validator";

pub fn main() {
    let result = isValidXmlName("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"xml-name-validator"`; install via `cap add xml-name-validator`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
