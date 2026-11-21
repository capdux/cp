# `cap:blake3` — BLAKE3 密码学哈希、MAC、KDF 与 XOF 标准库

## 概述

`cap:blake3`（亦可通过 `cap.blake3` 动态挂载）是纯 CP 实现的工业级 BLAKE3 密码学原语库，严格遵循 BLAKE3 官方规范（1024 字节 Chunk、7 轮 G 函数与消息置换、Merkle 树 CV 栈合并）。全程采用固定 `[54][8]u32` 栈状态机，支持高达 $2^{54}$ 块数据的零堆分配流式计算，提供标准 256-bit 哈希、Keyed MAC、上下文隔离 KDF 密钥派生以及任意长度 XOF（`finalSeek`）可扩展输出。

## 核心类型

| 类型 | 说明 |
| --- | --- |
| `Blake3Hasher` | 零堆分配流式哈希器，内置 `ChunkState` 与 `[54][8]u32` CV 栈 |
| `ChunkState` | 单个 1024 字节 Chunk 的压缩状态管理 |
| `Output` | 根输出节点，支持 `chainingValue()` 与任意偏移 `rootOutputBytes(seek, out)` |

### 核心常量

- `OUT_LEN: usize = 32` — 标准输出字节数
- `KEY_LEN: usize = 32` — Keyed MAC 密钥字节数
- `BLOCK_LEN: usize = 64` — 压缩块字节数
- `CHUNK_LEN: usize = 1024` — Merkle 树叶子 Chunk 字节数

## API 速查

### `Blake3Hasher` 方法

- `Blake3Hasher.init(): Blake3Hasher` — 初始化标准哈希器
- `Blake3Hasher.initKeyed(key: [32]u8): Blake3Hasher` — 初始化 Keyed MAC 哈希器
- `Blake3Hasher.initDeriveKey(context: []const u8): Blake3Hasher` — 初始化 KDF 密钥派生器
- `hasher.update(input: []const u8)` — 流式写入任意长度输入
- `hasher.final(out: []u8)` — 输出任意长度 XOF 摘要（`seek = 0`）
- `hasher.finalSeek(seek_pos: u64, out: []u8)` — 从指定字节偏移输出 XOF 摘要
- `hasher.reset()` — 重置状态以便复用

### 便捷函数

- `hash(input: []const u8): [32]u8` — 单次计算 32 字节 BLAKE3 摘要
- `hashInto(input: []const u8, out: []u8)` — 单次计算并写入任意长度 XOF 输出缓冲区
- `hashHex(input: []const u8): [64]u8` — 计算小写 64 字符十六进制摘要
- `keyedHash(key: [32]u8, input: []const u8): [32]u8` — 计算 32 字节 Keyed MAC
- `verifyKeyedMac(key: [32]u8, input: []const u8, expected_mac: []const u8): bool` — 常量时间校验 Keyed MAC
- `deriveKey(context: []const u8, key_material: []const u8, out: []u8)` — 上下文隔离密钥派生
- `constantTimeEqual(a: []const u8, b: []const u8): bool` — 常量时间字节切片比较

## 示例

```cp
import { Blake3Hasher, hash, hashHex, keyedHash, verifyKeyedMac, deriveKey } from "cap:blake3";

// 1. 单次哈希与 Hex
const digest = hash("hello blake3");
const hex = hashHex(""); // "af1349b9f5f9a1a6a0404dea36dcc9499bcb25c9adc112b7cc9a93cae41f3262"

// 2. 流式哈希与 XOF 扩展输出
let hasher = Blake3Hasher.init();
hasher.update("hello ");
hasher.update("blake3");
let xof_out: [64]u8 = [0; 64];
hasher.final(xof_out[0..]);

// 3. Keyed MAC 与常量时间校验
const key: [32]u8 = [0x11; 32];
const mac = keyedHash(key, "payload");
const ok = verifyKeyedMac(key, "payload", mac[0..]);

// 4. KDF 密钥派生
let derived: [32]u8 = [0; 32];
deriveKey("my-app v1 encryption-key", "master-secret", derived[0..]);
```
