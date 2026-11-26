# @cap/bstr

Pure CP, zero-allocation byte string utilities (clean-room MIT parity with Rust `bstr` v1).

## Design & Reuse

- **Cross-Package Reuse**: Depends on `@cap/memchr` and delegates byte/substring search, line splitting, and single-pass replacement directly to SWAR-accelerated `memchr`, `memrchr`, `memmemFind`, and `memmemRfind`.
- **Zero Heap Allocation**: Trimming and line splitting return zero-copy `SliceRange { start, end }` offsets; transforms write directly into caller-provided `[]u8` buffers.
- **Strict Lossy UTF-8**: `bstrToUtf8Lossy` enforces RFC 3629 validation (rejecting overlong encodings, UTF-16 surrogates, code points above `U+10FFFF`, and truncated sequences) and emits `U+FFFD` (`0xEF 0xBF 0xBD`) using Unicode maximal-subpart replacement.

## API

### Types
- `SliceRange { start: usize, end: usize }` with `len(): usize`, `isEmpty(): bool`

### Search & Matching (`@cap/memchr`)
- `bstrFind(haystack: []const u8, needle: []const u8): i64`
- `bstrRfind(haystack: []const u8, needle: []const u8): i64`
- `bstrFindByte(haystack: []const u8, b: u8): i64`
- `bstrRfindByte(haystack: []const u8, b: u8): i64`
- `bstrContains(haystack: []const u8, needle: []const u8): bool`
- `bstrStartsWith(s: []const u8, prefix: []const u8): bool`
- `bstrEndsWith(s: []const u8, suffix: []const u8): bool`

### Trimming & Line Splitting
- `bstrTrimStart(s: []const u8): SliceRange`
- `bstrTrimEnd(s: []const u8): SliceRange`
- `bstrTrim(s: []const u8): SliceRange`
- `bstrLineCount(s: []const u8): usize`
- `bstrLineAt(s: []const u8, idx: usize): SliceRange`

### Replacement, UTF-8 & ASCII
- `bstrReplaceAll(src: []const u8, pattern: []const u8, replacement: []const u8, out: []u8): usize`
- `bstrIsUtf8(src: []const u8): bool`
- `bstrToUtf8Lossy(src: []const u8, out: []u8): usize`
- `bstrIsAscii(s: []const u8): bool`
- `bstrToLowerAscii(src: []const u8, out: []u8): usize`
- `bstrToUpperAscii(src: []const u8, out: []u8): usize`
- `bstrEqIgnoreAsciiCase(a: []const u8, b: []const u8): bool`

## License

MIT
