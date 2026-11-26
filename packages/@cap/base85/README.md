# @cap/base85

零堆分配的 CP Base85 多标准编解码引擎 — 洁净室 MIT 实现，同时支持 **ZeroMQ Z85（RFC 32）** 与 **Adobe Ascii85（PDF / PostScript）**，相比 Base64 节省约 25% 传输体积。

Zero-allocation Base85 multi-standard encoder/decoder (`Z85` RFC 32 & Adobe `Ascii85`) for CP — clean-room MIT parity with Rust `z85` and `ascii85`.

## 安装 / Install

```bash
cap add @cap/base85
```

## API 参考 / API Reference

| 函数 / Function | 签名 / Signature | 说明 / Description |
| --- | --- | --- |
| `encodeZ85` | `(input: string, out: []u8) -> string?` | ZeroMQ Z85 编码（输入须为 4 字节倍数） |
| `decodeZ85` | `(encoded: string, out: []u8) -> string?` | ZeroMQ Z85 解码（须为 5 字节倍数，校验溢出） |
| `encodeAscii85` | `(input: string, out: []u8) -> string?` | Adobe Ascii85 编码（支持任意长度与 `'z'` 全零压缩） |
| `decodeAscii85` | `(encoded: string, out: []u8) -> string?` | Adobe Ascii85 解码（自动跳过空白与 `<~...~>` 包裹符） |

## 使用示例 / Usage

```cp
import { encodeZ85, decodeZ85, encodeAscii85, decodeAscii85 } from "@cap/base85";

let raw: [8]u8 = [0x86, 0x4F, 0xD2, 0x6F, 0xB5, 0x59, 0xF7, 0x5B];
let buf: [64]u8 = undefined;
const z = encodeZ85(raw[0..8], buf[0..64]).?; // "HelloWorld"

const a85 = encodeAscii85("CapDux!", buf[0..64]).?;
```

## License

MIT
