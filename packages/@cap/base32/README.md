# `@cap/base32` — 零分配 Base32 编解码器（RFC 4648 / Crockford / z-base-32）

> 独立 Clean-Room 纯 CP 实现，严格遵循 RFC 4648 §6/§7、Douglas Crockford Base32 与 z-base-32 公开规范（对标 Rust `base32` / `data-encoding` crate 与 npm `hi-base32`）| 许可协议：MIT

## 概述

`@cap/base32` 提供纯 CP、零外部依赖、零堆分配、流式 O(1) 栈空间的工业级 Base32 编解码库，适用于 TOTP 2FA（`otpauth://`）、Tor Onion v3、IPFS CID、DNSSEC NSEC3 与人类可读短码等场景：

- **四大标准字母表**：`Base32Alphabet.Rfc4648`（RFC 4648 §6）、`Base32Alphabet.Rfc4648Hex`（RFC 4648 §7）、`Base32Alphabet.Crockford`（防混淆字符集）、`Base32Alphabet.ZBase32`（人类友好字符集）；
- **大小写不敏感与 Crockford 容错**：全部字母表解码均自动兼容大小写；`Crockford` 自动容错 `I/L -> 1`、`O -> 0` 并忽略 `-` 分隔连字符；
- **严格规范校验**：兼容带 `=` 填充与无填充输入，严格拦截非法字符、非法长度、畸形填充及非零规范尾比特（RFC 4648 §3.5）。

## 导出 API

| 符号 / 签名 | 说明 |
| :--- | :--- |
| `Base32Error` | 错误枚举：`InvalidCharacter`、`InvalidLength`、`InvalidPadding`、`BufferTooSmall` |
| `Base32Alphabet` | 字母表枚举：`Rfc4648`、`Rfc4648Hex`、`Crockford`、`ZBase32` |
| `ALPHABET_RFC4648`, `ALPHABET_RFC4648_HEX`, `ALPHABET_CROCKFORD`, `ALPHABET_ZBASE32` | 4 套 32 字节标准字母表常量 |
| `encodedLen(input_len: usize, padded: bool): usize` | 计算编码后精确字节长度 |
| `decodedMaxLen(input_len: usize): usize` | 计算解码后最大字节长度上界 |
| `encode(input, alphabet, padded, out)` | 通用零分配 Base32 编码 |
| `decode(input, alphabet, out)` | 通用零分配 Base32 解码（严格校验） |
| `encodeStd(input, out)` / `decodeStd(input, out)` | RFC 4648 §6 标准 Base32（编码带 `=` 填充） |
| `encodeStdNoPad(input, out)` | RFC 4648 §6 无填充编码（TOTP / IPFS 常用） |
| `encodeHex(input, out)` / `decodeHex(input, out)` | RFC 4648 §7 Extended Hex Base32（DNSSEC 常用） |
| `encodeCrockford(input, out)` / `decodeCrockford(input, out)` | Crockford Base32（无填充编码，容错 `I/L/O/-` 解码） |

## 使用示例

### 示例 1：TOTP 2FA（RFC 6238 / `otpauth://`）密钥编码与解码

```cp
import { encodeStdNoPad, decodeStd } from "@cap/base32";

fn demoTotpSecret() {
    // RFC 6238 / RFC 4226 经典 20 字节 TOTP 测试密钥 "12345678901234567890"
    const raw_secret: []const u8 = "12345678901234567890";
    let key_buf: [64]u8 = [0; 64];
    const totp_key = encodeStdNoPad(raw_secret, key_buf[0..]) catch return;
    // totp_key == "GEZDGNBVGY3TQOJQGEZDGNBVGY3TQOJQ"

    let out_buf: [32]u8 = [0; 32];
    const restored = decodeStd(totp_key, out_buf[0..]) catch return;
    // restored == "12345678901234567890"
    * = restored;
}
```

### 示例 2：RFC 4648 标准与 Extended Hex 编解码

```cp
import { encodeStd, decodeStd, encodeHex, decodeHex } from "@cap/base32";

fn demoRfc4648() {
    let std_buf: [32]u8 = [0; 32];
    const s = encodeStd("foobar", std_buf[0..]) catch return;
    // s == "MZXW6YTBOI======"

    let hex_buf: [32]u8 = [0; 32];
    const h = encodeHex("foobar", hex_buf[0..]) catch return;
    // h == "CPNMUOJ1E8======"

    let out1: [16]u8 = [0; 16];
    let out2: [16]u8 = [0; 16];
    const d1 = decodeStd(s, out1[0..]) catch return;
    const d2 = decodeHex(h, out2[0..]) catch return;
    * = d1;
    * = d2;
}
```

### 示例 3：Crockford 容错解码与 z-base-32 往返

```cp
import {
    Base32Alphabet,
    encode,
    decode,
    encodeCrockford,
    decodeCrockford,
} from "@cap/base32";

fn demoCrockfordAndZBase32() {
    // Crockford 自动容错小写、I/L->1、O->0 与连字符 '-'
    let out: [32]u8 = [0; 32];
    const parsed = decodeCrockford("c5h6-6p35-cpjmg", out[0..]) catch return;

    let enc: [32]u8 = [0; 32];
    const re_encoded = encodeCrockford(parsed, enc[0..]) catch return;

    // z-base-32 人类可读编码
    let zb_enc: [32]u8 = [0; 32];
    let zb_out: [32]u8 = [0; 32];
    const zb = encode("hello", Base32Alphabet.ZBase32, false, zb_enc[0..]) catch return;
    const zb_dec = decode(zb, Base32Alphabet.ZBase32, zb_out[0..]) catch return;
    * = re_encoded;
    * = zb_dec;
}
```

## 许可协议

MIT License — Copyright (c) 2026 CP Contributors.
