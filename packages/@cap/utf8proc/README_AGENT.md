# @cap/utf8proc — AI Agent Quick Ref

Clean Unicode 16.0 normalization (NFC/NFD/NFKC/NFKD), case-folding & charwidth library for CP (`v2.10.0`, `MIT`)

## Import

```cp
import {
    Utf8ProcError,
    NormString,
    utf8proc_version,
    utf8proc_unicode_version,
    utf8proc_charwidth,
    utf8proc_tolower,
    utf8proc_toupper,
    utf8proc_NFC,
} from "@cap/utf8proc";
```

## Core Types & Constants

```cp
error Utf8ProcError { NormalizeFailed, OutOfMemory }
class NormString { raw: [*:0]u8!?, len: usize } // methods: nfc(...) +6
```

## API Signatures

```cp
fn utf8proc_version(): [*:0]&u8!;
fn utf8proc_unicode_version(): [*:0]&u8!;
fn utf8proc_charwidth(codepoint: i32): i32;
fn utf8proc_tolower(c: i32): i32;
fn utf8proc_toupper(c: i32): i32;
fn utf8proc_NFC(str: [*:0]&u8!): [*:0]u8!?;
fn utf8proc_NFD(str: [*:0]&u8!): [*:0]u8!?;
fn utf8proc_NFKC(str: [*:0]&u8!): [*:0]u8!?;
fn utf8proc_NFKD(str: [*:0]&u8!): [*:0]u8!?;
fn utf8proc_NFKC_Casefold(str: [*:0]&u8!): [*:0]u8!?;
fn free(ptr: *auto!?);
fn versionString(): string;
fn unicodeVersion(): string;
fn charWidth(cp: i32): i32;
fn toLower(cp: i32): i32;
fn toUpper(cp: i32): i32;
```

## Minimal Usage

```cp
import { versionString } from "@cap/utf8proc";

pub fn main() {
    let result = versionString();
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"@cap/utf8proc"`; install via `cap add @cap/utf8proc`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Error unions (`!T`)**: Handle fallible calls with `try`, `catch`, or `?` error propagation (CP has no `try/catch` blocks).
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
- **Native FFI safety**: Ensure C pointers are non-null and released deterministically via `defer` or `__cp_deinit`.
