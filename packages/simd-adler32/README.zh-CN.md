[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent 速查](README_AGENT.md)

# simd-adler32

面向 CP 的零堆分配 16-lane 硬件 SIMD Adler-32、IEEE CRC-32、Castagnoli CRC-32C 与 O(log N) GF(2) CRC-32 合并引擎（对齐 Rust `simd-adler32` / `crc32fast` / `crc-catalog`）。

内置 `@Vector(16, u8)` / `@Vector(16, u32)` 16 通道 SIMD Adler-32 核心（`NMAX = 5552` 延迟取模），同时提供查表流式 `Crc32Hasher`（IEEE `0xEDB88320`）、`Crc32cHasher`（Castagnoli `0x82F63B78`）以及与 `zlib` 等价的 `combineCrc32Ieee`。

## 源包溯源与零成本迁移

| 属性 | 说明 |
| --- | --- |
| 源生态平台 | **crates.io** |
| 原包名与版本 | `simd-adler32` / `crc-catalog`（CP 生态版 `v1.0.0`） |
| 上游仓库 | `https://github.com/mcountryman/simd-adler32` · `https://github.com/akhilles/crc-catalog` |
| 开源协议 | MIT |

从 Rust 迁移至 CP 保持原包名，零学习成本：

```rust
// Rust (crates.io: simd-adler32 / crc32fast)
let a = simd_adler32::adler32(&b"Wikipedia"[..]);
let c = crc32fast::hash(b"123456789");
```

```cp
// CP (cap: simd-adler32) — 16-lane SIMD Adler-32 + IEEE/Castagnoli CRC-32 + O(log N) 合并
import { adler32, crc32Ieee, crc32c, combineCrc32Ieee } from "simd-adler32";
const a = adler32("Wikipedia");   // 0x11E60398
const c = crc32Ieee("123456789"); // 0xCBF43926
```

## 安装

```bash
cap add simd-adler32
```

## 实战示例

### 基础用法：16-lane SIMD Adler-32、IEEE CRC-32 与 Castagnoli CRC-32C

```cp
import { adler32, crc32Ieee, crc32c } from "simd-adler32";

// 16-lane 硬件 SIMD Adler-32（RFC 1950）
const a = adler32("Wikipedia");      // 0x11E60398

// IEEE 802.3 CRC-32（gzip / PNG / ZIP）
const ieee = crc32Ieee("123456789"); // 0xCBF43926

// Castagnoli CRC-32C（iSCSI / RocksDB / LevelDB / gRPC）
const cast = crc32c("123456789");    // 0xE3069283
```

### 进阶用法：流式哈希器与 O(log N) 并行 CRC-32 合并

```cp
import { Crc32Hasher, Crc32cHasher, crc32Ieee, combineCrc32Ieee } from "simd-adler32";

// 1. 流式 IEEE CRC-32（支持 reset 复用 256 项查找表）
let h = Crc32Hasher.init();
h.update("1234");
h.update("56789");
const sum = h.finalize(); // 0xCBF43926
h.reset();

// 2. 流式 Castagnoli CRC-32C
let hc = Crc32cHasher.init();
hc.update("header:");
hc.update("payload");
const tag = hc.finalize();

// 3. O(log N) GF(2) 矩阵快速幂合并并行分片 CRC-32
const c1 = crc32Ieee("1234");
const c2 = crc32Ieee("56789");
const combined = combineCrc32Ieee(c1, c2, 5); // 0xCBF43926 == crc32Ieee("123456789")
```

## API 速查表

| 导出符号 | 签名 | 说明 |
| --- | --- | --- |
| `adler32` | `fn(data: string): u32` | 16-lane SIMD RFC 1950 Adler-32 校验和。 |
| `crc32Ieee` | `fn(data: string): u32` | 一次性计算 IEEE 802.3 CRC-32（`0xEDB88320`）。 |
| `crc32c` | `fn(data: string): u32` | 一次性计算 Castagnoli CRC-32C（`0x82F63B78`）。 |
| `Crc32Hasher` | `class { init(), update(*_, data: string), finalize(): u32, reset(*_) }` | 流式 IEEE CRC-32 哈希器（256 项查表）。 |
| `Crc32cHasher` | `class { init(), update(*_, data: string), finalize(): u32, reset(*_) }` | 流式 Castagnoli CRC-32C 哈希器（256 项查表）。 |
| `combineCrc32Ieee` | `fn(crc1: u32, crc2: u32, len2: usize): u32` | O(log N) GF(2) 矩阵快速幂合并（对齐 `zlib` `crc32_combine`）。 |

## 性能特性

- **16-lane 硬件 SIMD**：`adler32` 每次将 16 字节扩展至 `@Vector(16, u32)` 加权归约，每 `5552` 字节仅取模一次。
- **O(log N) 合并**：`combineCrc32Ieee` 零堆分配合并任意长度分片的 IEEE CRC-32。

## 协议

MIT — 受 `simd-adler32`、`crc-catalog` 与 `crc32fast` 启发的纯 CP 净室实现。
