[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：吸收自 **crates.io** [`memchr`](https://github.com/BurntSushi/memchr)（`MIT OR Unlicense`）（包名 `@cap/memchr`）。

# @cap/memchr

Pure CP, zero-allocation SWAR-accelerated byte and substring search engine (clean-room MIT parity with Rust `memchr` v2).

## Performance & Design

- **8-Byte SWAR Parallel Scanning**: Scans 8 bytes per iteration in `u64` registers using Mycroft's zero-byte mask `(x -% 0x0101010101010101) & ~x & 0x8080808080808080` and hardware `@ctz` lane extraction.
- **Multi-Needle & Reverse Support**: Full forward (`memchr` / `memchr2` / `memchr3`) and reverse (`memrchr` / `memrchr2` / `memrchr3`) byte search.
- **Accelerated Substring Search**: `memmemFind` and `memmemRfind` combine SWAR `memchr`/`memrchr` jumping with first/last-byte prefiltering.
- **Zero Heap Allocation**: Operates directly on `[]const u8` slices.

## API

- `memchr(needle: u8, haystack: []const u8): i64`
- `memchr2(n1: u8, n2: u8, haystack: []const u8): i64`
- `memchr3(n1: u8, n2: u8, n3: u8, haystack: []const u8): i64`
- `memrchr(needle: u8, haystack: []const u8): i64`
- `memrchr2(n1: u8, n2: u8, haystack: []const u8): i64`
- `memrchr3(n1: u8, n2: u8, n3: u8, haystack: []const u8): i64`
- `memchrCount(needle: u8, haystack: []const u8): usize`
- `memchrFindAll(needle: u8, haystack: []const u8, out_offsets: []usize): usize`
- `memmemFind(haystack: []const u8, needle: []const u8): i64`
- `memmemRfind(haystack: []const u8, needle: []const u8): i64`

## License

MIT
