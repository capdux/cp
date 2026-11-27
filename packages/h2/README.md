[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/http2-hpack

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:h2` ([https://github.com/hyperium/h2](https://github.com/hyperium/h2))（原库名 `h2` → CP 包名 `@cap/http2-hpack`）

Zero-allocation RFC 7541 HTTP/2 HPACK core primitives: full 61-entry static table, N-bit prefixed integer codec, and indexed header field encoder for CP.

## Features

- **Zero heap allocation**: pure CP table lookups and buffer-based binary codecs.
- **Full RFC 7541 Appendix A static table**: `staticTableGet`, `staticTableFind`, `staticTableFindName` across all 61 standard entries.
- **RFC 7541 Section 5.1 N-bit prefixed integers**: `encodePrefixedInt` and overflow/truncation-safe `decodePrefixedInt` for `1..8` prefix bits.
- **Indexed header encoding**: `encodeIndexedHeader` (`0x80 | index` with 7-bit prefix).

## Usage

```cp
import { staticTableFind, encodeIndexedHeader, decodePrefixedInt } from "@cap/http2-hpack";

const idx = staticTableFind(":method", "GET"); // 2
let buf: [8]u8 = undefined;
const n = encodeIndexedHeader(idx, buf[0..8]); // [0x82]
const decRes = decodePrefixedInt(buf[0..n], 7 as u8); // value = 2
```
