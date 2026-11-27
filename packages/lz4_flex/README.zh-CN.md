[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# `lz4_flex`

面向 CP 语言的零分配 LZ4 原始块无损压缩与解压库——完整对标 **crates.io `lz4_flex`** 与官方 LZ4 Block Format，内置固定 4096 槽位栈哈希表、4 字节快速匹配、重叠 RLE 拷贝以及 4 字节小端长度前缀帧格式。

## 源包溯源与零成本迁移

| 项目 | 说明 |
| :--- | :--- |
| **源包平台与对标** | `crates.io:lz4_flex` ([PSeitz/lz4_flex](https://github.com/PSeitz/lz4_flex), v0.11 对齐) |
| **CP 包名与版本** | `lz4_flex@1.0.0` (`MIT`) |
| **外部依赖** | 无（仅依赖内置 `cap:bin`、`cap:fmt`） |

保持原包名导入，零改名成本直接迁移：

```rust
// Rust (crates.io lz4_flex)
use lz4_flex::block::{compress_prepend_size, decompress_size_prepended};
let compressed = compress_prepend_size(input);
let restored = decompress_size_prepended(&compressed)?;
```

```cp
// CP — 保持原包名，零堆分配调用方缓冲
import { compressWithSize, decompressWithSize, maxCompressedLen } from "lz4_flex";

let comp_buf: [4352]u8 = undefined;
const comp_len = compressWithSize(input, comp_buf[0..]);
let out_buf: [4096]u8 = undefined;
const orig_len = decompressWithSize(comp_buf[0..@intCast(comp_len)], out_buf[0..]);
```

## ReleaseFast 实测性能亮点

在 Intel Xeon Platinum 8175M 平台下使用 `cpc 1.0.0` + `zig 0.17.0 -OReleaseFast -fllvm` 实测（详见 [`bench/ECOSYSTEM_PERF_REPORT.md`](../../bench/ECOSYSTEM_PERF_REPORT.md)）：

| 操作 | 测试载荷 | 延迟 | 吞吐量 | 带宽 | 核心优化架构 |
| :--- | :--- | --: | --: | --: | :--- |
| `compressBlock` | 4 KB JSON 日志（27.7× 压缩率） | **1,862.81 ns/op** | 536,822 ops/s | **2,096.96 MB/s (2.10 GB/s)** | 4096 槽位栈哈希表 + 4B 匹配 |
| `decompressBlock` | 4 KB（148 B 压缩块） | **1,251.47 ns/op** | 799,060 ops/s | **3,121.33 MB/s (3.12 GB/s)** | `@memcpy` 字面量/匹配块高速拷贝 |

- **零堆分配**：哈希表常驻栈空间，压缩与解压均直接写入调用方传入的切片，无任何堆申请。
- **原生 `@memcpy` 解压加速**：非重叠字面量与匹配段直通 LLVM `@memcpy`，重叠 RLE 匹配（`offset < match_len`）自动回退至安全逐字节拷贝。

## 实战示例

### 1. 原始块压缩与解压（`compressBlock` / `decompressBlock`）

```cp
import { compressBlock, decompressBlock, maxCompressedLen } from "lz4_flex";

fn demoRawBlock() {
    const src = "hello lz4_flex! hello lz4_flex! hello lz4_flex! hello lz4_flex!";
    const worst = maxCompressedLen(src.len);

    let comp_buf: [256]u8 = undefined;
    if (comp_buf.len < worst) { return; }

    const comp_n = compressBlock(src, comp_buf[0..]);
    if (comp_n < 0) { return; }

    let decomp_buf: [256]u8 = undefined;
    const orig_n = decompressBlock(comp_buf[0..@intCast(comp_n)], decomp_buf[0..]);
    if (orig_n < 0) { return; }
    // decomp_buf[0..@intCast(orig_n)] == src
}
```

### 2. 带 4 字节小端长度前缀帧（`compressWithSize` / `decompressWithSize`）

```cp
import { compressWithSize, decompressWithSize } from "lz4_flex";

fn demoSizePrefixed() {
    const payload = "{\"event\":\"metric\",\"service\":\"gateway\",\"status\":200}";

    // 头部写入 4 字节小端原始长度（与 Rust lz4_flex::compress_prepend_size 字节级互通）
    let wire: [256]u8 = undefined;
    const wire_len = compressWithSize(payload, wire[0..]);
    if (wire_len < 0) { return; }

    let restored: [256]u8 = undefined;
    const out_len = decompressWithSize(wire[0..@intCast(wire_len)], restored[0..]);
    if (out_len < 0) { return; }
    // restored[0..@intCast(out_len)] == payload
}
```

### 3. 进阶边界场景：缓冲区尺寸计算与损坏流防护

```cp
import { compressBlock, decompressBlock, decompressWithSize, maxCompressedLen } from "lz4_flex";

fn demoEdgeCases() {
    // 1. 压缩缓冲区至少预留 maxCompressedLen(src.len) 字节
    //    使用 compressWithSize() 时需额外预留 +4 字节长度头
    const needed = maxCompressedLen(4096) + 4;
    _ = needed;

    // 2. 目标缓冲区不足时安全返回 -1，绝不越界
    let tiny: [4]u8 = undefined;
    const fail_comp = compressBlock("incompressible-sample-bytes-0123456789", tiny[0..]); // -1

    // 3. 截断或畸形压缩帧安全返回 -1
    const bad_frame = [_]u8{ 0xFF, 0x00, 0x00, 0x00, 0xF0 };
    let dst: [64]u8 = undefined;
    const fail_dec = decompressWithSize(bad_frame[0..], dst[0..]); // -1
    const fail_raw = decompressBlock(bad_frame[0..], dst[0..]);    // -1
    _ = fail_comp; _ = fail_dec; _ = fail_raw;
}
```

## 核心 API 速览

- `maxCompressedLen(input_len: usize): usize` — 计算不可压缩输入的最坏输出字节数
- `compressBlock(src: &[]u8, dst: []u8): i64` — LZ4 原始块压缩（失败返回 `-1`）
- `decompressBlock(src: &[]u8, dst: []u8): i64` — LZ4 原始块解压（失败返回 `-1`）
- `compressWithSize(src: &[]u8, dst: []u8): i64` — 4 字节 LE 长度前缀 + 原始块压缩（失败返回 `-1`）
- `decompressWithSize(src: &[]u8, dst: []u8): i64` — 校验 4 字节 LE 长度头并解压（失败返回 `-1`）

完整类型签名请查阅 [`src/index.d.cp`](src/index.d.cp)。
