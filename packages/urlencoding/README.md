# `@cap/urlencoding`

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:urlencoding` ([https://github.com/kornelski/rust_urlencoding](https://github.com/kornelski/rust_urlencoding))（原库名 `urlencoding` → CP 包名 `@cap/urlencoding`）

Zero-heap RFC 3986 percent-encoding and decoding library in pure CP, modeled after Rust's `urlencoding` v2 crate. Encodes all bytes outside `A-Z`, `a-z`, `0-9`, `-`, `_`, `.`, `~` as uppercase `%XX`, supports binary buffers, strict decoding with error reporting (`-1`), and lossy passthrough decoding.

零堆分配纯 CP RFC 3986 URL 百分号编解码库，完整对标 `crates.io:urlencoding` v2：仅保留 `A-Z`, `a-z`, `0-9`, `-`, `_`, `.`, `~`，其余字节编码为大写 `%XX`，支持二进制切片编码、严格解码（畸形转义/缓冲不足返回 `-1`）与宽松透传解码。

## Installation

```bash
cap add @cap/urlencoding
```

## API

| Function | Signature | Description |
| --- | --- | --- |
| `encode` | `(input: string, out: []u8): usize` | RFC 3986 percent-encode string into `out` |
| `encodeBinary` | `(data: []const u8, out: []u8): usize` | Percent-encode arbitrary bytes into `out` |
| `decode` | `(input: string, out: []u8): i64` | Strictly decode `%XX`; returns `-1` on malformed escape or short buffer |
| `decodeLossy` | `(input: string, out: []u8): usize` | Decode valid `%XX`, passing invalid `%` sequences through verbatim |
| `encodedLen` | `(input: string): usize` | Compute exact encoded byte length |
| `isUnreserved` | `(b: u8): bool` | Check if byte is RFC 3986 unreserved (`A-Z a-z 0-9 - _ . ~`) |

## License

MIT
