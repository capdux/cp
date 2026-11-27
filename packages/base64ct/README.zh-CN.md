[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent 速查](README_AGENT.md)

# base64ct

面向 CP 的零堆分配 **恒定时间（Constant-Time）、无查找表、无秘密分支** Base64 编解码器（对齐 RustCrypto `base64ct`）。

全程采用纯位运算算术映射（零 64/256 字节查找表、零依赖秘密数据的条件分支），防止编解码密钥、令牌或 PEM 数据时产生缓存计时侧信道泄漏，并严格执行 RFC 4648 规范位校验。

## 源包溯源与零成本迁移

| 属性 | 说明 |
| --- | --- |
| 源生态平台 | **crates.io**（兼顾 npm `base64-arraybuffer`） |
| 原包名与版本 | `base64ct`（CP 生态版 `v1.0.0`） |
| 上游仓库 | `https://github.com/RustCrypto/formats` |
| 开源协议 | MIT |

从 Rust 迁移至 CP 保持原包名，零学习成本：

```rust
// Rust (crates.io: base64ct)
use base64ct::{Base64, Base64UrlUnpadded, Encoding};
let enc = Base64::encode(&secret_key, &mut buf)?;
let dec = Base64UrlUnpadded::decode(jwt_part, &mut out)?;
```

```cp
// CP (cap: base64ct) — 无查表无分支恒定时间，零堆分配
import { encodeStd, encodeUrlUnpadded, decodeStd, decodeUrlUnpadded } from "base64ct";
const n = encodeStd(secret_key, buf);
const res = decodeUrlUnpadded(jwt_part, out);
```

## 安装

```bash
cap add base64ct
```

## 实战示例

### 基础用法：标准与 URL-Safe 恒定时间编解码

```cp
import {
    encodedLen,
    encodeStd,
    encodeUrlUnpadded,
    decodeStd,
    decodeUrlUnpadded
} from "base64ct";

let secret: [5]u8 = ['h', 'e', 'l', 'l', 'o'];

// 1. 预计算精确编码长度并进行标准 Base64 编码（带 '=' 填充）
const need = encodedLen(5, true); // 8
let encBuf: [16]u8 = undefined;
const encLen = encodeStd(secret[0..5], encBuf[0..16]); // encBuf[0..encLen] == "aGVsbG8="

// 2. 严格标准解码
let decBuf: [16]u8 = undefined;
const r1 = decodeStd("aGVsbG8=", decBuf[0..16]);
if (r1.ok) {
    // decBuf[0..r1.len] == "hello"
}

// 3. URL-Safe 无填充编解码（适用于 JWT / JWS / WebAuthn）
const uLen = encodeUrlUnpadded(secret[0..5], encBuf[0..16]); // "aGVsbG8"
const r2 = decodeUrlUnpadded("aGVsbG8", decBuf[0..16]);
// r2.ok == true, r2.len == 5
```

### 安全与边界用法：规范尾位校验与失败自动清零

```cp
import { decodeStd, decodeUrlUnpadded } from "base64ct";

let out: [16]u8 = undefined;

// 1. 恒定时间拒绝非规范填充尾位（防止 Base64 延展性问题）
const badCanon = decodeStd("aGVsbG9=", out[0..16]);
// badCanon.ok == false, badCanon.len == 0，且 out 中间输出已自动清零

// 2. 严格拒绝非法字符、空白符及非法长度（len % 4 == 1）
const badLen = decodeUrlUnpadded("a", out[0..16]);
// badLen.ok == false, badLen.len == 0
```

## API 速查表

| 导出符号 | 签名 | 说明 |
| --- | --- | --- |
| `DecodeResult` | `class { len: usize, ok: bool }` | 解码结果；失败时自动清零 `out` 并返回 `ok: false`。 |
| `encodedLen` | `fn(n: usize, padded: bool): usize` | 计算 `n` 字节输入编码后的精确长度。 |
| `encodeStd` | `fn(src: []u8, out: []u8): usize` | 标准字母表（`+`/`/`）带 `=` 填充；`out` 不足返回 `0`。 |
| `encodeStdUnpadded` | `fn(src: []u8, out: []u8): usize` | 标准字母表无填充编码。 |
| `encodeUrl` | `fn(src: []u8, out: []u8): usize` | URL-Safe 字母表（`-`/`_`）带 `=` 填充编码。 |
| `encodeUrlUnpadded` | `fn(src: []u8, out: []u8): usize` | URL-Safe 无填充编码（JWT/JOSE）。 |
| `decodeStd` | `fn(src: string, out: []u8): DecodeResult` | 恒定时间严格规范标准 Base64 解码。 |
| `decodeUrlUnpadded` | `fn(src: string, out: []u8): DecodeResult` | 恒定时间严格规范 URL-Safe 无填充解码。 |

## 安全特性

- **无查找表无分支**：6-bit 与 ASCII 互转完全由纯位算术实现，杜绝缓存计时侧信道。
- **严格 RFC 4648 规范性**：恒定时间校验末尾填充位必须全 0。
- **失败清零**：解码失败时立即擦除 `out` 缓冲区，避免泄露半解码明文。

## 协议

MIT — 对齐 RustCrypto `base64ct` 的纯 CP 净室实现。
