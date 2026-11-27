[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# hmac

面向 CP 语言的零堆分配 RFC 2104 流式 HMAC-SHA256 与 HMAC-SHA512 消息认证码库，内置常量时间校验与小写十六进制编码（复用 `cookie-signature` 的 SHA-256 基础实现）。

对齐 Rust crates.io 源包 [`hmac`](https://github.com/RustCrypto/MACs)（MIT OR Apache-2.0）。

## 安装与导入

```bash
cap add hmac
```

```cp
import {
    HMAC_SHA256_LEN,
    HMAC_BLOCK_SIZE,
    HMAC_SHA512_LEN,
    HMAC_SHA512_BLOCK_SIZE,
    HmacSha256,
    HmacSha512,
    hmacSha256Bytes,
    hmacSha256Hex,
    hmacSha512,
    hmacSha512Hex,
    ctEqualBytes,
    encodeHex32,
    encodeHex64,
} from "hmac";
```

## 核心 API

### 常量与辅助函数

| 符号 | 类型 / 签名 | 说明 |
| --- | --- | --- |
| `HMAC_SHA256_LEN` / `HMAC_BLOCK_SIZE` | `usize = 32` / `64` | SHA-256 摘要长度与分组长度（字节）。 |
| `HMAC_SHA512_LEN` / `HMAC_SHA512_BLOCK_SIZE` | `usize = 64` / `128` | SHA-512 摘要长度与分组长度（字节）。 |
| `ctEqualBytes` | `(a: []const u8, b: []const u8): bool` | 常量时间字节切片比对，防御时序侧信道泄露。 |
| `encodeHex32` | `(digest: []u8, out64: []u8): usize` | 将 32 字节摘要编码为 64 字节小写十六进制。 |
| `encodeHex64` | `(digest: []const u8, out128: []u8): usize` | 将 64 字节摘要编码为 128 字节小写十六进制。 |

### 流式 MAC 类

| 类名 | 方法列表 | 说明 |
| --- | --- | --- |
| `HmacSha256` | `newFromSlice(key: []u8)`, `newFromConstSlice(key: []const u8)`, `newFromKey(key: string)`, `update(*self, data: []u8)`, `updateConst(*self, data: []const u8)`, `updateStr(*self, msg: string)`, `finalize(*self, out32: []u8)`, `verifySlice(*self, expected: []u8): bool`, `verifyConstSlice(*self, expected: []const u8): bool` | 流式 RFC 2104 HMAC-SHA256 计算器，支持常量时间 `verify*` 校验。 |
| `HmacSha512` | `newFromSlice(key: []const u8)`, `newFromKey(key: string)`, `update(*self, data: []const u8)`, `updateStr(*self, msg: string)`, `finalize(*self, out64: []u8)`, `verifySlice(*self, expected: []const u8): bool` | 流式 RFC 2104 HMAC-SHA512 计算器，支持常量时间 `verifySlice` 校验。 |

### 一次性快捷函数

| 函数 | 签名 | 说明 |
| --- | --- | --- |
| `hmacSha256Bytes` | `(key: []const u8, msg: []const u8, out32: []u8)` | 一次性计算 32 字节原始 HMAC-SHA256 并写入 `out32`。 |
| `hmacSha256Hex` | `(key: string, msg: string, out64: []u8): usize` | 计算 64 字节小写十六进制 HMAC-SHA256 写入 `out64`，返回 `64`。 |
| `hmacSha512` | `(key: []const u8, msg: []const u8, out64: []u8)` | 一次性计算 64 字节原始 HMAC-SHA512 并写入 `out64`。 |
| `hmacSha512Hex` | `(key: string, msg: string, out128: []u8): usize` | 计算 128 字节小写十六进制 HMAC-SHA512 写入 `out128`，返回 `128`。 |

## 使用示例

### 1. 一次性计算 HMAC-SHA256 与 HMAC-SHA512 十六进制签名

```cp
import { print } from "cap:io";
import { hmacSha256Hex, hmacSha512Hex } from "hmac";

pub fn main() {
    let hex256: [64]u8 = undefined;
    let n256 = hmacSha256Hex("secret-key", "hello world", hex256[0..64]);
    print("HMAC-SHA256: {}\n", hex256[0..n256]);

    let hex512: [128]u8 = undefined;
    let n512 = hmacSha512Hex("secret-key", "hello world", hex512[0..128]);
    print("HMAC-SHA512: {}\n", hex512[0..n512]);
}
```

### 2. 流式分块更新与常量时间验签

```cp
import { print } from "cap:io";
import { HmacSha256 } from "hmac";

pub fn main() {
    let mac = HmacSha256.newFromKey("webhook-secret");
    mac.updateStr("chunk-1:");
    mac.updateStr("chunk-2");
    let tag: [32]u8 = undefined;
    mac.finalize(tag[0..32]);

    let verifier = HmacSha256.newFromKey("webhook-secret");
    verifier.updateStr("chunk-1:chunk-2");
    let ok = verifier.verifyConstSlice(tag[0..32]);
    print("verified={}\n", ok);
}
```

## 与源包差异与零拷贝安全亮点

- **零堆分配**：全部哈希状态、`o_key_pad` 密钥填充及摘要输出均位于栈缓冲区，无堆分配开销。
- **抗时序侧信道验签**：`verifySlice`、`verifyConstSlice` 与 `ctEqualBytes` 均采用常量时间比对，避免签名验证泄露时序信息。
- **单态化直出**：以零抽象开销的 `HmacSha256` / `HmacSha512` 具象类替代 Rust 泛型 `Hmac<D>`，并附带一键十六进制输出接口。
