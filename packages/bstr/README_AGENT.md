# bstr@1.0.0 (Agent Quickref)

> Zero-allocation byte string utilities (Rust bstr v1 parity) powered by @cap/memchr SWAR search, line splitting, lossy UTF-8 decoding, and ASCII transforms | License: MIT
> Import: `import { ... } from "bstr";` | Full types: `src/index.d.cp`
> Deps: `memchr`

## Functions
- `export fn bstrFind(haystack: &[]u8, needle: &[]u8): i64;`
- `export fn bstrRfind(haystack: &[]u8, needle: &[]u8): i64;`
- `export fn bstrFindByte(haystack: &[]u8, b: u8): i64;`
- `export fn bstrRfindByte(haystack: &[]u8, b: u8): i64;`
- `export fn bstrContains(haystack: &[]u8, needle: &[]u8): bool;`
- `export fn bstrStartsWith(s: &[]u8, prefix: &[]u8): bool;`
- `export fn bstrEndsWith(s: &[]u8, suffix: &[]u8): bool;`
- `export fn bstrTrimStart(s: &[]u8): SliceRange;`
- `export fn bstrTrimEnd(s: &[]u8): SliceRange;`
- `export fn bstrTrim(s: &[]u8): SliceRange;`
- `export fn bstrLineCount(s: &[]u8): usize;`
- `export fn bstrLineAt(s: &[]u8, idx: usize): SliceRange;`
- `export fn bstrReplaceAll(src: &[]u8, pattern: &[]u8, replacement: &[]u8, out: []u8): usize;`
- `export fn bstrIsUtf8(src: &[]u8): bool;`
- `export fn bstrToUtf8Lossy(src: &[]u8, out: []u8): usize;`
- `export fn bstrIsAscii(s: &[]u8): bool;`
- `export fn bstrToLowerAscii(src: &[]u8, out: []u8): usize;`
- `export fn bstrToUpperAscii(src: &[]u8, out: []u8): usize;`
- `export fn bstrEqIgnoreAsciiCase(a: &[]u8, b: &[]u8): bool;`
