[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# `crc32c`

面向 CP 语言的零堆分配纯 CP CRC-32C（Castagnoli，`0x82F63B78`）校验和库——完整对标 **crates.io `crc32c`**，支持流式增量追加（`crc32cAppend` / `Crc32cHasher`）、`O(log N)` GF(2) 并行合并，以及 LevelDB / RocksDB / Google Cloud Storage 标准掩码编解码（`maskCrc32c` / `unmaskCrc32c`）。

## 源包溯源与零成本迁移

| 项目 | 说明 |
| :--- | :--- |
| **源包平台与对标** | `crates.io:crc32c` ([zowens/crc32c](https://github.com/zowens/crc32c), v0.6 对齐) |
| **CP 包名与版本** | `crc32c@1.0.0` (`MIT`) |
| **外部依赖** | 无（纯 CP，零外部依赖） |

保持原包名导入，零改名成本直接迁移：

```rust
// Rust (crates.io crc32c)
let sum = crc32c::crc32c(b"123456789"); // 0xE3069283
let chained = crc32c::crc32c_append(sum, b"more");
```

```cp
// CP — 保持原包名，零堆分配
import { crc32cStr, crc32cAppend, Crc32cHasher } from "crc32c";

const sum = crc32cStr("123456789"); // 0xE3069283
let h = Crc32cHasher.new();
h.updateStr("123456789");
const s2 = h.finalize();            // 0xE3069283
```

## ReleaseFast 实测性能亮点

在 Intel Xeon Platinum 8175M 平台下使用 `cpc 1.0.0` + `zig 0.17.0 -OReleaseFast -fllvm` 实测（详见 [`bench/ECOSYSTEM_PERF_REPORT.md`](../../bench/ECOSYSTEM_PERF_REPORT.md)）：

| 操作 | 测试载荷 | 延迟 | 吞吐量 | 带宽 | 核心优化架构 |
| :--- | :--- | --: | --: | --: | :--- |
| `crc32c` | 64 KB 缓冲区 | **138,707.48 ns/op** | 7,209 ops/s | **450.59 MB/s** | 256 项 Castagnoli LUT、零堆分配 |

- **纯 CP 跨平台确定性**：基于 Castagnoli 多项式（`CRC32C_POLY_REVERSED = 0x82F63B78`）与编译期 256 项查找表，跨架构结果完全一致。
- **存储引擎开箱即用**：内置 LevelDB / RocksDB / Snappy / GCS 标准掩码算法（`CRC32C_MASK_DELTA = 0xA282EAD8`）。

## 实战示例

### 1. 一次性计算与增量追加（`crc32c`、`crc32cStr`、`crc32cAppend`）

```cp
import { crc32c, crc32cStr, crc32cAppend } from "crc32c";

fn demoBasic() {
    // Castagnoli 标准测试向量："123456789" -> 0xE3069283
    const s1 = crc32cStr("123456789");

    let p1 = [_]u8{ '1', '2', '3', '4' };
    let p2 = [_]u8{ '5', '6', '7', '8', '9' };
    const partial = crc32c(p1[0..]);
    const total = crc32cAppend(partial, p2[0..]); // 0xE3069283
    _ = s1; _ = total;
}
```

### 2. 流式 `Crc32cHasher` 与并行 GF(2) 合并

```cp
import { Crc32cHasher, crc32cCombine, crc32cStr } from "crc32c";

fn demoStreamingAndCombine() {
    let h1 = Crc32cHasher.new();
    h1.updateStr("block-A:");

    let h2 = Crc32cHasher.new();
    h2.updateStr("block-B");

    h1.combine(h2);
    const merged = h1.finalize();

    // 直接合并两个 u32 校验和（传入第二段字节长度）
    const c1 = crc32cStr("block-A:");
    const c2 = crc32cStr("block-B");
    const merged2 = crc32cCombine(c1, c2, 7); // == crc32cStr("block-A:block-B")
    _ = merged; _ = merged2;
}
```

### 3. 存储引擎掩码（`maskCrc32c` / `unmaskCrc32c`）与状态复位

```cp
import { Crc32cHasher, maskCrc32c, unmaskCrc32c } from "crc32c";

fn demoMasking() {
    // LevelDB / RocksDB / Snappy / GCS 掩码，避免嵌套 CRC 校验抵消
    let h = Crc32cHasher.new();
    h.updateStr("wal-record-payload");
    const raw_crc = h.finalize();

    const stored = maskCrc32c(raw_crc);
    const restored = unmaskCrc32c(stored); // restored == raw_crc

    // 复位并复用到下一条 WAL 记录
    h.reset();
    h.updateStr("next-wal-record");
    const next_crc = h.finalize();
    _ = restored; _ = next_crc;
}
```

## 核心 API 速览

- **一次性/追加**：`crc32c(data: []u8): u32`、`crc32cStr(s: string): u32`、`crc32cAppend(crc: u32, data: []u8): u32`
- **GF(2) 合并**：`crc32cCombine(crc1: u32, crc2: u32, len2: u64): u32`
- **存储掩码**：`maskCrc32c(crc: u32): u32`、`unmaskCrc32c(masked: u32): u32`
- **流式计算**：`Crc32cHasher`（`new`、`newWithInitial`、`update`、`updateStr`、`finalize`、`reset`、`combine`）
- **常量**：`CRC32C_POLY_REVERSED`（`0x82F63B78`）、`CRC32C_MASK_DELTA`（`0xA282EAD8`）

完整类型签名请查阅 [`src/index.d.cp`](src/index.d.cp)。
