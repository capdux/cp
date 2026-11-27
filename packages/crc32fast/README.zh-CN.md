[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# `crc32fast`

面向 CP 语言的零堆分配 IEEE 802.3 CRC-32（`0xEDB88320`）校验和引擎——完整对标 **crates.io `crc32fast`**，内置 Slice-by-4 查表加速、流式 `Crc32Hasher` 以及 `O(log N)` GF(2) 矩阵并行合并能力。

## 源包溯源与零成本迁移

| 项目 | 说明 |
| :--- | :--- |
| **源包平台与对标** | `crates.io:crc32fast` ([srijs/rust-crc32fast](https://github.com/srijs/rust-crc32fast), v1 对齐) |
| **CP 包名与版本** | `crc32fast@1.0.0` (`MIT`) |
| **外部依赖** | 无（纯 CP，零外部依赖） |

保持原包名导入，零改名成本直接迁移：

```rust
// Rust (crates.io crc32fast)
use crc32fast::Hasher;
let mut h = Hasher::new();
h.update(b"123456789");
let sum = h.finalize(); // 0xCBF43926
```

```cp
// CP — 保持原包名，Slice-by-4 LUT 加速
import { Crc32Hasher, hashStr } from "crc32fast";

let h = Crc32Hasher.new();
h.updateStr("123456789");
const sum = h.finalize();              // 0xCBF43926
const one_shot = hashStr("123456789"); // 0xCBF43926
```

## ReleaseFast 实测性能亮点

在 Intel Xeon Platinum 8175M 平台下使用 `cpc 1.0.0` + `zig 0.17.0 -OReleaseFast -fllvm` 实测（详见 [`bench/ECOSYSTEM_PERF_REPORT.md`](../../bench/ECOSYSTEM_PERF_REPORT.md)）：

| 操作 | 测试载荷 | 延迟 | 吞吐量 | 带宽 | 核心优化架构 |
| :--- | :--- | --: | --: | --: | :--- |
| `hashBytes` | 64 KB 缓冲区 | **54,965.76 ns/op** | 18,193 ops/s | **1,137.07 MB/s (1.14 GB/s)** | Slice-by-4 LUT（`[4][256]u32`）、零堆分配 |

- **Slice-by-4 查表加速**：通过编译期生成的 `[4][256]u32` 查找表每轮并行处理 4 字节（`CRC32_POLY_REVERSED = 0xEDB88320`），吞吐量达单字节 256 项查表实现的 **2.52 倍**。
- **GF(2) 矩阵合并**：支持在 `O(log len2)` 时间内将两段独立计算的 CRC-32 直接合并，无需重新扫描原始数据。

## 实战示例

### 1. 一次性计算与流式 `Crc32Hasher`

```cp
import { hashBytes, hashStr, Crc32Hasher } from "crc32fast";

fn demoBasic() {
    // IEEE 802.3 标准测试向量："123456789" -> 0xCBF43926
    const s1 = hashStr("123456789");
    const raw = [_]u8{ '1', '2', '3', '4', '5', '6', '7', '8', '9' };
    const s2 = hashBytes(raw[0..]);

    let h = Crc32Hasher.new();
    h.updateStr("1234");
    h.updateStr("56789");
    const s3 = h.finalize(); // 0xCBF43926
    _ = s1; _ = s2; _ = s3;
}
```

### 2. 并行分块校验和合并（`combine` 与 `combineCrc32`）

```cp
import { Crc32Hasher, combineCrc32, hashStr } from "crc32fast";

fn demoCombine() {
    // 两段数据可跨 Actor / 线程独立计算
    let h1 = Crc32Hasher.new();
    h1.updateStr("hello ");

    let h2 = Crc32Hasher.new();
    h2.updateStr("world");

    // 通过 Hasher.combine() 自动使用 h2.amount 合并
    h1.combine(h2);
    const merged = h1.finalize();

    // 也可直接传入两个原始 u32 校验和及第二段字节长度
    const c1 = hashStr("hello ");
    const c2 = hashStr("world");
    const merged2 = combineCrc32(c1, c2, 5); // == hashStr("hello world")
    _ = merged; _ = merged2;
}
```

### 3. 进阶边界场景：断点续算（`newWithInitial`）与状态复用（`reset`）

```cp
import { Crc32Hasher } from "crc32fast";

fn demoResumeAndReset() {
    // 从已持久化的中间 CRC 状态恢复续算
    let part1 = Crc32Hasher.new();
    part1.updateStr("header:");
    const checkpoint = part1.finalize();

    let resumed = Crc32Hasher.newWithInitial(checkpoint);
    resumed.updateStr("payload");
    const total = resumed.finalize();

    // 调用 reset() 零开销复用到下一帧
    resumed.reset();
    resumed.updateStr("next-frame");
    const next_crc = resumed.finalize();
    _ = total; _ = next_crc;
}
```

## 核心 API 速览

- **一次性哈希**：`hashBytes(buf: &[]u8): u32`、`hashStr(s: string): u32`
- **GF(2) 合并**：`combineCrc32(crc1: u32, crc2: u32, len2: u64): u32`
- **流式计算**：`Crc32Hasher`（`new`、`newWithInitial`、`update`、`updateStr`、`finalize`、`reset`、`combine`）
- **常量**：`CRC32_POLY_REVERSED`（`0xEDB88320`）

完整类型签名请查阅 [`src/index.d.cp`](src/index.d.cp)。
