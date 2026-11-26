# @cap/percent-encoding

Zero-allocation WHATWG & RFC 3986 percent-encoding engine with customizable 128-bit `AsciiSet` bitmaps for CP — clean-room MIT parity with Rust `percent-encoding` (1.15B downloads on crates.io).

纯 CP 零堆分配 WHATWG / RFC 3986 可定制字符集百分号编解码引擎，内置 7 套标准字符集与严格/宽容双模式解码，防止 URL 注入与畸形编码攻击。

## Install / 安装

```bash
cap add @cap/percent-encoding
```

## API

| Symbol | Signature | Description |
| --- | --- | --- |
| `AsciiSet` | `struct { lo: u64, hi: u64 }` | 128-bit ASCII bitmap (`empty`, `add`, `remove`, `contains`; non-ASCII `>= 0x80` always encoded) |
| `controlsSet` | `() -> AsciiSet` | `0x00..0x1F` + `0x7F` |
| `fragmentSet` | `() -> AsciiSet` | `controls` + space `" < > \`` |
| `querySet` | `() -> AsciiSet` | `controls` + space `" # < >` |
| `pathSegmentSet` | `() -> AsciiSet` | `query` + `? \` { } /` |
| `userinfoSet` | `() -> AsciiSet` | `pathSegment` + `: ; = @ [ \ ] ^ \|` |
| `nonAlphanumericSet` | `() -> AsciiSet` | Encode everything except `A-Z a-z 0-9` |
| `rfc3986UnreservedSet` | `() -> AsciiSet` | Encode everything except `A-Z a-z 0-9 - . _ ~` |
| `percentEncode` | `(input: string, set: AsciiSet, out: []u8) -> string` | Uppercase `%XX` percent-encoding |
| `percentDecode` | `(input: string, out: []u8) -> string` | Lenient percent-decoding (preserves invalid `%`) |
| `percentDecodeStrict` | `(input: string, out: []u8) -> string?` | Strict percent-decoding (`null` on malformed `%ZZ` / truncated `%A`) |
| `formUrlEncode` / `formUrlDecode` | `(input: string, out: []u8) -> string` | `application/x-www-form-urlencoded` (`space` ↔ `+`) |

## Usage / 使用示例

```cp
import {
    pathSegmentSet,
    percentEncode,
    percentDecodeStrict,
    formUrlEncode,
} from "@cap/percent-encoding";

let out: [128]u8 = undefined;
const enc = percentEncode("a/b?c=1", pathSegmentSet(), out[0..128]);
// "a%2Fb%3Fc=1"
```
