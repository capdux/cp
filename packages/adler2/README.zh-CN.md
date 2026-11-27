[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent 速查](README_AGENT.md)

# adler2

面向 CP 的零堆分配 RFC 1950 Adler-32 校验和引擎，内置 16-lane 硬件 SIMD 加速、流式更新、O(1) 滑动窗口滚动哈希与 O(1) 校验和合并（对齐 Rust `adler2`）。

主循环采用 `@Vector(16, u8)` / `@Vector(16, u32)` 加权向量归约，每轮并行处理 16 字节，并以 `NMAX = 5552` 延迟取模最大化吞吐量。

## 源包溯源与零成本迁移

| 属性 | 说明 |
| --- | --- |
| 源生态平台 | **crates.io** |
| 原包名与版本 | `adler2`（CP 生态版 `v1.0.0`） |
| 上游仓库 | `https://github.com/oyvindln/adler2` |
| 开源协议 | MIT |

从 Rust 迁移至 CP 保持原包名，零成本切换：

```rust
// Rust (crates.io: adler2)
use adler2::{Adler32, adler32_slice};
let sum = adler32_slice(b"Wikipedia");
```

```cp
// CP (cap: adler2) — 16-lane SIMD + 滚动窗口 + 合并
import { Adler32, adler32, combineAdler32 } from "adler2";
const sum = adler32(&"Wikipedia"); // 0x11E60398
```

## 安装

```bash
cap add adler2
```

## 实战示例

### 基础用法：一次性与流式 16-lane SIMD 校验

```cp
import { Adler32, adler32 } from "adler2";

// RFC 1950 标准测试向量（"Wikipedia" -> 0x11E60398）
const data: []const u8 = "Wikipedia";
const sum = adler32(&data); // 0x11E60398

// 跨任意分片流式累加
let h = Adler32.init();
const p1: []const u8 = "Wiki";
const p2: []const u8 = "pedia";
h.update(&p1);
h.update(&p2);
const streamed = h.finalize(); // 0x11E60398

h.reset(); // 零分配原地重置复用
```

### 进阶用法：O(1) 滚动哈希（rsync / 数据去重）与并行合并

```cp
import { Adler32, adler32, combineAdler32 } from "adler2";

// 1. 4 字节滑动窗口 O(1) 滚动更新（"abcd" -> "bcde"）
let roll = Adler32.init();
const win0: []const u8 = "abcd";
roll.update(&win0);
roll.roll(4, 'a' as u8, 'e' as u8);
// roll.finalize() == adler32(&"bcde")

// 2. 合并并行分片独立算出的校验和
const chunkA: []const u8 = "hello ";
const chunkB: []const u8 = "world!";
const cA = adler32(&chunkA);
const cB = adler32(&chunkB);
const merged = combineAdler32(cA, cB, chunkB.len as u64);
// merged == adler32(&"hello world!")

// 3. 从已有 u32 校验值断点续算
let resumed = Adler32.fromValue(cA);
resumed.update(&chunkB);
// resumed.finalize() == merged
```

## API 速查表

| 导出符号 | 签名 | 说明 |
| --- | --- | --- |
| `Adler32.init` | `static fn(): Adler32` | 初始化状态 `s1 = 1, s2 = 0`（空输入校验值为 `1`）。 |
| `Adler32.fromValue` | `static fn(checksum: u32): Adler32` | 从已有 Adler-32 值恢复哈希器状态。 |
| `Adler32.update` | `fn(*_, data: &[]u8)` | 16-lane SIMD 流式追加字节切片（`NMAX = 5552` 延迟取模）。 |
| `Adler32.updateByte` | `fn(*_, b: u8)` | 单字节增量更新。 |
| `Adler32.roll` | `fn(*_, window_len: usize, out_byte: u8, in_byte: u8)` | O(1) 滑动窗口移出 `out_byte`、移入 `in_byte`。 |
| `Adler32.finalize` | `fn(): u32` | 返回最终校验值 `(s2 << 16) \| s1`。 |
| `Adler32.reset` | `fn(*_)` | 重置状态为 `s1 = 1, s2 = 0`。 |
| `adler32` | `fn(data: &[]u8): u32` | 一次性计算 RFC 1950 Adler-32 校验和。 |
| `combineAdler32` | `fn(adler1: u32, adler2: u32, len2: u64): u32` | O(1) 合并两段校验和为 `adler32(A \|\| B)`。 |

## 性能特性

- **16-lane 硬件 SIMD**：将 16 字节提升为 `@Vector(16, u32)` 并乘以权重向量 `[16..1]` 一次性归约。
- **延迟取模**：每 5552 字节才做一次 `% 65521`，彻底消除热循环除法开销。

## 协议

MIT — 对齐 Rust `adler2` 的纯 CP 净室实现。
