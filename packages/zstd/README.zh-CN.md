[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent 快速参考](README_AGENT.md)

# zstd

高性能 CP 原生 **Zstandard (Zstd)** 压缩与解压库，底层封装官方 C `libzstd`，提供支持 RAII 自动释放的 `Compressor` / `Decompressor` 类，兼顾零分配（`*Into`）与动态分配（`*Alloc`）两种模式。

## 源包溯源与零成本迁移

| 字段 | 说明 |
| :--- | :--- |
| **源平台** | [crates.io](https://crates.io/crates/zstd) |
| **原包名与版本** | `zstd`（v0.13 对齐，底层集成官方 `libzstd`） |
| **上游仓库** | https://github.com/gyscos/zstd-rs |
| **CP 包名与版本** | `zstd`（`1.0.0`） |
| **开源协议** | BSD-3-Clause / MIT |

在 CP 中保持原包名直接导入，无需改名：

```rust
// Rust (crates.io)
use zstd::bulk::{Compressor, Decompressor};
```

```typescript
// CP (cap)
import { Compressor, Decompressor, compressBound, versionNumber, ZstdError } from "zstd";
```

## 安装

```bash
cap add zstd
```

## 核心特性与性能亮点

- **原生 `libzstd` 性能**：内置预编译 C `zstd` 原生库（`opt: "fast"` 优化），吞吐量与压缩率媲美原生 C/Rust。
- **RAII 上下文管理**：`Compressor` 与 `Decompressor` 内置 `__cp_deinit`，离开作用域自动释放 `ZSTD_CCtx` / `ZSTD_DCtx`，杜绝内存泄漏。
- **零分配高频复用**：通过 `compressInto` / `decompressInto` 直接复用上下文与调用方缓冲区，适合高频网络包与日志流压缩。
- **动态分配便捷接口**：当目标大小未知时，可使用 `compressAlloc` / `decompressAlloc` 自动按帧大小分配输出切片。

## 快速上手

### 零分配压缩与解压（`compressInto` / `decompressInto`）

```typescript
import { Compressor, Decompressor } from "zstd";

fn roundtripZeroAlloc(input: &[]u8): !void {
    let comp: Compressor = undefined;
    try comp.init(3); // 压缩级别 3（速度与压缩率均衡）

    let compressed: [4096]u8 = undefined;
    const comp_len = try comp.compressInto(input, compressed[0..]);

    let decomp: Decompressor = undefined;
    try decomp.init();

    let restored: [4096]u8 = undefined;
    const dec_len = try decomp.decompressInto(compressed[0..comp_len], restored[0..]);
    _ = dec_len;
}
```

### 动态分配压缩与解压（`compressAlloc` / `decompressAlloc`）

```typescript
import { Compressor, Decompressor } from "zstd";
import * as std from "std";

fn roundtripAlloc(allocator: std.mem.Allocator, input: &[]u8): ![]u8 {
    let comp: Compressor = undefined;
    try comp.init(6);
    const compressed = try comp.compressAlloc(allocator, input);
    defer allocator.free(compressed);

    let decomp: Decompressor = undefined;
    try decomp.init();
    return try decomp.decompressAlloc(allocator, compressed);
}
```

## API 速览

| 导出符号 | 签名 | 说明 |
| :--- | :--- | :--- |
| `versionNumber` | `() -> u32` | 获取运行时 `libzstd` 版本号 |
| `compressBound` | `(src_size: usize) -> usize` | 计算最坏情况压缩输出字节上限 |
| `Compressor.init` | `(level: i32) -> !void` | 创建压缩上下文并指定压缩级别（1..22） |
| `Compressor.compressInto` | `(src: &[]u8, dst: []u8) -> !usize` | 压缩到调用方缓冲区，返回写入长度 |
| `Compressor.compressAlloc` | `(allocator: std.mem.Allocator, src: &[]u8) -> ![]u8` | 分配内存并压缩 |
| `Decompressor.init` | `() -> !void` | 创建解压上下文 |
| `Decompressor.decompressInto` | `(src: &[]u8, dst: []u8) -> !usize` | 解压到调用方缓冲区，返回写入长度 |
| `Decompressor.decompressAlloc` | `(allocator: std.mem.Allocator, src: &[]u8) -> ![]u8` | 按帧头声明大小分配内存并解压 |
| `ZstdError` | 错误枚举 | `AllocContextFailed`, `InvalidContext`, `CompressFailed`, `DecompressFailed`, `UnknownContentSize`, `OutOfMemory` |

## 开源协议

BSD-3-Clause / MIT
