[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent 速查](README_AGENT.md)

# sha2

面向 CP 的零堆分配 FIPS 180-4 **SHA-256** 与 **SHA-224** 流式密码学哈希引擎，支持小写十六进制输出（对齐 RustCrypto `sha2`）。

所有内部哈希状态、64 字节字符串分片缓冲及 32/28 字节摘要均驻留栈上，全程零堆内存分配。

## 源包溯源与零成本迁移

| 属性 | 说明 |
| --- | --- |
| 源生态平台 | **crates.io** |
| 原包名与版本 | `sha2`（CP 生态版 `v1.0.0`） |
| 上游仓库 | `https://github.com/RustCrypto/hashes` |
| 开源协议 | MIT（上游：MIT OR Apache-2.0） |

从 Rust 迁移至 CP 保持原包名，零成本切换：

```rust
// Rust (crates.io: sha2)
use sha2::{Sha256, Sha224, Digest};
let mut h = Sha256::new();
h.update(b"hello world");
let digest = h.finalize();
```

```cp
// CP (cap: sha2) — 零堆分配流式 SHA-256 / SHA-224
import { Sha256, Sha224, sha256Hex, sha224Hex } from "sha2";
let h = Sha256.new();
h.updateStr("hello world");
let digest: [32]u8 = undefined;
h.finalize(digest[0..32]);
```

## 安装

```bash
cap add sha2
```

## 实战示例

### 基础用法：一次性 Hex 与流式 SHA-256 / SHA-224

```cp
import { Sha256, Sha224, sha256Hex, sha224Hex } from "sha2";

// 1. 一次性计算小写十六进制摘要（FIPS 180-4 "abc" 标准向量）
let hex256: [64]u8 = undefined;
const n256 = sha256Hex("abc", hex256[0..64]);
// n256 == 64 -> "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"

let hex224: [56]u8 = undefined;
const n224 = sha224Hex("abc", hex224[0..56]);
// n224 == 56 -> "23097d223405d8228642a477bda255b32aadbce4bda0b3f7e36c9da7"

// 2. 流式多分片 SHA-256（输出 32 字节原始摘要）
let h = Sha256.new();
let part1: [5]u8 = ['h', 'e', 'l', 'l', 'o'];
h.update(part1[0..5]);
h.updateStr(" world");
let raw32: [32]u8 = undefined;
h.finalize(raw32[0..32]);
```

### 进阶与边界用法：哈希器 `reset` 复用与 SHA-224

```cp
import { Sha256, Sha224 } from "sha2";

// 1. 通过 reset() 原地复用栈上 Sha256 实例
let hasher = Sha256.new();
let outHex: [64]u8 = undefined;

hasher.updateStr("chunk-A");
const lenA = hasher.finalizeHex(outHex[0..64]); // 64

hasher.reset(); // 恢复 FIPS 180-4 SHA-256 初始向量
hasher.updateStr("chunk-B");
const lenB = hasher.finalizeHex(outHex[0..64]); // 64

// 2. 流式 SHA-224（28 字节原始摘要 / 56 字节小写 Hex）
let h224 = Sha224.new();
h224.updateStr("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq");
let dig28: [28]u8 = undefined;
h224.finalize(dig28[0..28]);
```

## API 速查表

| 导出符号 | 签名 | 说明 |
| --- | --- | --- |
| `Sha256.new` | `static fn(): Sha256` | 创建 FIPS 180-4 SHA-256 哈希器。 |
| `Sha256.update` | `fn(*_, data: []u8)` | 追加原始字节切片。 |
| `Sha256.updateStr` | `fn(*_, s: string)` | 按 64 字节栈分片追加字符串。 |
| `Sha256.finalize` | `fn(*_, out32: []u8)` | 输出 32 字节摘要到 `out32`。 |
| `Sha256.finalizeHex` | `fn(*_, out64: []u8): usize` | 输出 64 字节小写 Hex 到 `out64`，返回 `64` 或 `0`。 |
| `Sha256.reset` | `fn(*_)` | 重置状态为 SHA-256 初始向量。 |
| `Sha224.new` | `static fn(): Sha224` | 创建 FIPS 180-4 SHA-224 哈希器。 |
| `Sha224.update` / `updateStr` | `fn(*_, data: []u8)` / `fn(*_, s: string)` | 追加字节切片或字符串。 |
| `Sha224.finalize` | `fn(*_, out28: []u8)` | 输出 28 字节摘要到 `out28`。 |
| `Sha224.finalizeHex` | `fn(*_, out56: []u8): usize` | 输出 56 字节小写 Hex 到 `out56`，返回 `56` 或 `0`。 |
| `sha256Hex` | `fn(s: string, out64: []u8): usize` | 一次性计算 SHA-256 64 字节小写 Hex。 |
| `sha224Hex` | `fn(s: string, out56: []u8): usize` | 一次性计算 SHA-224 56 字节小写 Hex。 |

## 安全与性能

- **零堆分配**：全程仅使用栈上定长数组，无堆内存残留。
- **FIPS 180-4 标准对齐**：内置官方 SHA-256 与 SHA-224 初始向量与填充规范。

## 协议

MIT — 对齐 RustCrypto `sha2` 的纯 CP 净室实现。
