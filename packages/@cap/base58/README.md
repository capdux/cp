# `@cap/base58` — 零分配 Base58 与 Base58Check 编解码器

> 独立 Clean-Room 纯 CP 实现，遵循 Bitcoin / Solana / IPFS / Flickr / Ripple Base58 与 Base58Check 公开规范（对标 Rust `bs58` crate 与 npm `bs58`）| 许可协议：MIT

## 概述

`@cap/base58` 提供纯 CP、零外部依赖、零堆分配的 Base58 与 Base58Check 编解码器：
- **多标准字母表**：内置 `ALPHABET_BITCOIN`（Bitcoin / Solana / IPFS CIDv0）、`ALPHABET_FLICKR`、`ALPHABET_RIPPLE` 及任意自定义 58 字节字母表；
- **严格前导零保持**：完整保留前导 `0x00` 字节与 `'1'` 字符映射；
- **Base58Check 恒定时间校验**：`encodeCheck` / `decodeCheck` 内置双 SHA-256 四字节校验和与恒定时间比对。

## 导出 API

| 符号 / 签名 | 说明 |
| :--- | :--- |
| `ALPHABET_BITCOIN`, `ALPHABET_FLICKR`, `ALPHABET_RIPPLE` | 三大标准 Base58 字母表 |
| `encode(input, out)` / `decode(input, out)` | 标准 Bitcoin/Solana/IPFS Base58 编解码 |
| `encodeWithAlphabet(input, alphabet, out)` / `decodeWithAlphabet(input, alphabet, out)` | 指定字母表 Base58 编解码 |
| `encodeCheck(payload, out)` / `decodeCheck(input, out)` | Base58Check 双 SHA-256 校验和编解码（含恒定时间校验） |

## 使用示例

### 示例 1：标准 Base58 编解码

```cp
import { encode, decode } from "@cap/base58";

fn demoBasic() {
    let enc_buf: [64]u8 = [0; 64];
    const encoded = encode("Hello World!", enc_buf[0..]) catch return;
    // encoded == "2NEpo7TZRRrLZSi2U"

    let dec_buf: [64]u8 = [0; 64];
    const decoded = decode(encoded, dec_buf[0..]) catch return;
    // decoded == "Hello World!"
    * = decoded;
}
```

### 示例 2：Base58Check 地址与防篡改校验

```cp
import { encodeCheck, decodeCheck } from "@cap/base58";

fn demoCheck() {
    const payload: [21]u8 = [0x00, 0x01, 0x09, 0x66, 0x77, 0x60, 0x06, 0x95, 0x3D, 0x55, 0x67, 0x43, 0x9E, 0x5E, 0x39, 0xF8, 0x6A, 0x0D, 0x27, 0x3B, 0xEE];
    let addr_buf: [64]u8 = [0; 64];
    const addr = encodeCheck(payload[0..], addr_buf[0..]) catch return;
    // addr == "16UwLL9Risc3QfPqBUvKofHmBQ7wMtjvM"

    let out: [32]u8 = [0; 32];
    const verified = decodeCheck(addr, out[0..]) catch return;
    * = verified;
}
```
