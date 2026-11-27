[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent 速查](README_AGENT.md)

# hmac

面向 CP 的零堆分配 RFC 2104 流式 **HMAC-SHA256** 消息认证码，内置 **恒定时间校验** 与小写十六进制输出（对齐 RustCrypto `hmac`）。

所有密钥填充块（`ipad`/`opad`）、内外层 SHA-256 状态与校验缓冲区均位于栈上，全程零堆内存分配。

## 源包溯源与零成本迁移

| 属性 | 说明 |
| --- | --- |
| 源生态平台 | **crates.io** |
| 原包名与版本 | `hmac`（CP 生态版 `v1.0.0`） |
| 上游仓库 | `https://github.com/RustCrypto/MACs` |
| 开源协议 | MIT（上游：MIT OR Apache-2.0） |

从 Rust 迁移至 CP 保持原包名，零成本切换：

```rust
// Rust (crates.io: hmac)
use hmac::{Hmac, Mac};
let mut mac = Hmac::<Sha256>::new_from_slice(key)?;
mac.update(msg);
mac.verify_slice(&expected_tag)?;
```

```cp
// CP (cap: hmac) — 零堆流式 HMAC-SHA256 + 恒定时间校验
import { HmacSha256, hmacSha256Hex } from "hmac";
let mac = HmacSha256.newFromSlice(key);
mac.update(msg);
const ok = mac.verifySlice(expected_tag);
```

## 安装

```bash
cap add hmac
```

## 实战示例

### 基础用法：流式 HMAC-SHA256 与一次性十六进制输出

```cp
import { HmacSha256, encodeHex32, hmacSha256Hex } from "hmac";

// 1. 一次性计算 64 字节小写 Hex 认证码（RFC 4231 测试向量 2）
let hexBuf: [64]u8 = undefined;
const hexLen = hmacSha256Hex("Jefe", "what do ya want for nothing?", hexBuf[0..64]);
// hexLen == 64 -> "5bdcc146bf60754e6a042426089575c75a003f089d2739839dec58b964ec3843"

// 2. 流式多分片计算原始 32 字节摘要
let mac = HmacSha256.newFromKey("webhook-secret");
mac.updateStr("timestamp=1700000000.");
mac.updateStr("{\"event\":\"paid\"}");
let tag: [32]u8 = undefined;
mac.finalize(tag[0..32]);

// 可选：将 32 字节原始标签编码为 64 字节小写 Hex
const n = encodeHex32(tag[0..32], hexBuf[0..64]); // 64
```

### 安全与边界用法：恒定时间校验与超长密钥（> 64 字节）

```cp
import { HmacSha256 } from "hmac";

// 1. 恒定时间标签校验（内部调用 ctEqual，防止计时侧信道）
let verifier = HmacSha256.newFromKey("webhook-secret");
verifier.updateStr("timestamp=1700000000.{\"event\":\"paid\"}");
const valid = verifier.verifySlice(expectedTag[0..32]);

// 2. 超过 64 字节的密钥按 RFC 2104 自动先做 SHA-256 哈希
// 二进制密钥或超过 256 字节的长密钥请务必使用 newFromSlice！
let longKey: [131]u8 = [0xAA; 131];
let macLong = HmacSha256.newFromSlice(longKey[0..131]);
macLong.updateStr("Test Using Larger Than Block-Size Key - Hash Key First");
let digest: [32]u8 = undefined;
macLong.finalize(digest[0..32]);
```

## API 速查表

| 导出符号 | 签名 | 说明 |
| --- | --- | --- |
| `HmacSha256.newFromSlice` | `static fn(key: []u8): HmacSha256` | 从任意长度二进制密钥初始化（`> 64` 字节自动预哈希）。 |
| `HmacSha256.newFromKey` | `static fn(key: string): HmacSha256` | 从字符串密钥初始化（上限 256 字节）。 |
| `HmacSha256.update` | `fn(*_, data: []u8)` | 追加字节切片更新 MAC 状态。 |
| `HmacSha256.updateStr` | `fn(*_, msg: string)` | 按 64 字节栈分片追加字符串更新 MAC。 |
| `HmacSha256.finalize` | `fn(*_, out32: []u8)` | 结束计算并将 32 字节标签写入 `out32`。 |
| `HmacSha256.verifySlice` | `fn(*_, expected: []u8): bool` | 结束计算并与 `expected` 进行恒定时间比对（`ctEqual`）。 |
| `encodeHex32` | `fn(digest: []u8, out64: []u8): usize` | 将 32 字节摘要转为 64 字节小写 Hex，返回 `64` 或 `0`。 |
| `hmacSha256Hex` | `fn(key: string, msg: string, out64: []u8): usize` | 一次性计算字符串 HMAC-SHA256 并写入 64 字节 Hex。 |

## 安全特性

- **零堆分配**：密钥、填充块与摘要全栈驻留，不产生堆残留。
- **恒定时间比对**：`verifySlice` 校验长度等于 32 并使用 `ctEqual` 比较。

## 协议

MIT — 对齐 RustCrypto `hmac` 的纯 CP 净室实现。
