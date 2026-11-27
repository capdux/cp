# sfv — AI Agent Quick Ref

Zero-allocation W3C User-Agent Client Hints (Sec-CH-UA*) RFC 8941 structured header parser and formatter for CP. (`v1.0.0`, `MIT`)

## Import

```cp
import {
    BrandVersion,
    uahToLower,
    uahIsOws,
    uahTrimOws,
    uahEqIgnoreCase,
    uahFindIgnoreCaseFrom,
    uahScanQuoted,
    uahSkipToNextItem,
} from "sfv";
```

## Core Types & Constants

```cp
class BrandVersion { brand: string, version: string }
```

## API Signatures

```cp
fn uahToLower(c: u8): u8;
fn uahIsOws(c: u8): bool;
fn uahTrimOws(s: &[]u8): &[]u8;
fn uahEqIgnoreCase(a: &[]u8, b: &[]u8): bool;
fn uahFindIgnoreCaseFrom(haystack: &[]u8, needle: &[]u8, start_idx: usize): usize;
fn uahScanQuoted(header: &[]u8, pos: &mut usize, innerStart: &mut usize, innerEnd: &mut usize): bool;
fn uahSkipToNextItem(header: &[]u8, pos: &mut usize);
fn uahParseNextBrand(header: &[]u8, pos: &mut usize, item: &mut BrandVersion): bool;
fn parseBrands(header: string, out: []BrandVersion): usize;
fn isGreaseBrand(brand: string): bool;
fn findBrandVersion(header: string, brand: string): string;
fn parseMobile(header: string): i32;
fn parsePlatform(header: string): string;
fn uahQuotedEncodedLen(s: &[]u8): usize;
fn uahWriteQuoted(out: []u8, pos: &mut usize, s: &[]u8);
fn formatBrands(brands: &[]BrandVersion, out: []u8): usize;
```

## Minimal Usage

```cp
import { parseBrands } from "sfv";

pub fn main() {
    let result = parseBrands("example", /* out: []BrandVersion */);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"sfv"`; install via `cap add sfv`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
