[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/hex

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:hex` ([https://github.com/KokaKiwi/rust-hex](https://github.com/KokaKiwi/rust-hex))

Zero-allocation hexadecimal encoder/decoder, fixed-width `u32`/`u64` hex formatters, and canonical 16-bytes/line HexDump utility for CP — clean-room MIT parity with Rust `hex`.

零堆分配的 CP 十六进制编解码与 HexDump 调试引擎，支持大小写编码、严格解码与 `0x`/`0X` 前缀、定长 32/64 位整数十六进制格式化以及经典 16 字节/行 Hex+ASCII 调试转储（纯 CP 洁净室 MIT 实现）。

## Install / 安装

```bash
cap add @cap/hex
```

## API Reference / 接口一览

| Symbol | Signature | Description |
| :--- | :--- | :--- |
| `encodeHex` / `encodeHexUpper` | `(input: string, out: []u8) -> string` | 小写 / 大写十六进制编码 |
| `decodeHex` | `(hex_str: string, out: []u8) -> string?` | 严格解码（校验偶数长度、合法字符集与缓冲区容量） |
| `decodeHexPrefixed` | `(hex_str: string, out: []u8) -> string?` | 支持可选 `0x`/`0X` 前缀的十六进制解码 |
| `isValidHex` | `(s: string) -> bool` | 校验字符串是否为合法偶数长度十六进制 |
| `encodeU32Hex` / `encodeU64Hex` | `(v, out: []u8) -> string` | 定长 8 / 16 字符大端小写十六进制编码 |
| `hexDump` | `(data: string, out: []u8) -> string` | 16 字节/行 Canonical Hex+ASCII 调试转储 |

## Usage / 使用示例

```cp
import { encodeHex, decodeHexPrefixed, encodeU32Hex, hexDump } from "@cap/hex";

let buf: [128]u8 = undefined;
const enc = encodeHex("Hello", buf[0..128]);                // "48656c6c6f"
const dec = decodeHexPrefixed("0x48656c6c6f", buf[0..128]); // "Hello"
const u32h = encodeU32Hex(0x00ff12ab, buf[0..128]);         // "00ff12ab"
```

## License

MIT
