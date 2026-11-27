[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent 速查](README_AGENT.md)

# crc64fast

面向 CP 的零堆分配 64 位 CRC 校验引擎，支持 **ECMA-182**、**CRC-64/XZ** 与 **CRC-64/GO-ISO**，采用编译期 **Slice-by-4**（`[4][256]u64`）查表加速与流式 `Crc64Hasher`（对齐 Rust `crc64fast` / `crc`）。

三种标准预设的 4×256 Slice-by-4 查找表均在编译期生成，主循环每次处理 4 字节，零运行期建表开销、零堆分配。

## 源包溯源与零成本迁移

| 属性 | 说明 |
| --- | --- |
| 源生态平台 | **crates.io** |
| 原包名与版本 | `crc64fast`（CP 生态版 `v1.0.0`） |
| 上游仓库 | `https://github.com/srijs/rust-crc64fast` |
| 开源协议 | MIT |

从 Rust 迁移至 CP 保持原包名，零学习成本：

```rust
// Rust (crates.io: crc64fast)
use crc64fast::Digest;
let mut d = Digest::new();
d.write(b"123456789");
let sum = d.sum64();
```

```cp
// CP (cap: crc64fast) — 编译期 Slice-by-4 查表，零堆分配
import { Crc64Hasher, crc64Ecma, crc64Xz, crc64Iso } from "crc64fast";
const sum = crc64Ecma(&"123456789"); // 0x6C40DF5F0B497347
```

## 安装

```bash
cap add crc64fast
```

## 实战示例

### 基础用法：三种标准一次性校验（官方 `"123456789"` 测试向量）

```cp
import { crc64Ecma, crc64Xz, crc64Iso } from "crc64fast";

const data: []const u8 = "123456789";

// ECMA-182（正向，多项式 0x42F0E1EBA9EA3693）-> 0x6C40DF5F0B497347
const ecma = crc64Ecma(&data);

// CRC-64/XZ（反转，多项式 0xC96C5795D7870F42）-> 0x995DC9BBDF1939FA
const xz = crc64Xz(&data);

// CRC-64/GO-ISO（反转，多项式 0xD800000000000000）-> 0xB90956C775A41001
const iso = crc64Iso(&data);
```

### 进阶用法：Slice-by-4 流式哈希器与自定义多项式

```cp
import { Crc64Hasher, makeU64 } from "crc64fast";

// 1. 流式 CRC-64/XZ（内部自动走编译期 Slice-by-4 查表）
let h = Crc64Hasher.initXz();
const c1: []const u8 = "1234";
const c2: []const u8 = "56789";
h.update(&c1);
h.update(&c2);
const digest = h.finalize(); // 0x995DC9BBDF1939FA

h.reset(); // 重置状态复用

// 2. 自定义 64 位 CRC 多项式（如 CRC-64/WE）
const poly = makeU64(0x42F0E1EB, 0xA9EA3693);
const allOnes = makeU64(0xFFFFFFFF, 0xFFFFFFFF);
let custom = Crc64Hasher.initCustom(poly, allOnes, allOnes, false);
custom.update(&c1);
custom.update(&c2);
const customSum = custom.finalize();
```

## API 速查表

| 导出符号 | 签名 | 说明 |
| --- | --- | --- |
| `crc64Ecma` | `fn(data: &[]u8): u64` | 一次性计算 CRC-64/ECMA-182（Slice-by-4）。 |
| `crc64Xz` | `fn(data: &[]u8): u64` | 一次性计算 CRC-64/XZ（Slice-by-4）。 |
| `crc64Iso` | `fn(data: &[]u8): u64` | 一次性计算 CRC-64/GO-ISO（Slice-by-4）。 |
| `Crc64Hasher.initEcma` | `static fn(): Crc64Hasher` | 初始化 ECMA-182 流式哈希器。 |
| `Crc64Hasher.initXz` | `static fn(): Crc64Hasher` | 初始化 CRC-64/XZ 流式哈希器。 |
| `Crc64Hasher.initIso` | `static fn(): Crc64Hasher` | 初始化 CRC-64/GO-ISO 流式哈希器。 |
| `Crc64Hasher.initCustom` | `static fn(poly: u64, init_val: u64, xorout: u64, reflected: bool): Crc64Hasher` | 自定义多项式流式哈希器（256 项查表）。 |
| `Crc64Hasher.update` | `fn(*_, data: &[]u8)` | 追加字节切片更新校验状态。 |
| `Crc64Hasher.finalize` | `fn(): u64` | 返回 `crc ^ xorout`（不破坏状态，可继续 `update` 或 `reset`）。 |
| `Crc64Hasher.reset` | `fn(*_)` | 重置 `crc` 为 `init_val`。 |
| `makeU64` | `fn(hi: u32, lo: u32): u64` | 将高/低 32 位组合为 `u64`。 |

## 性能特性

- **编译期 Slice-by-4 查找表**：`CRC64_ECMA_LUT` / `CRC64_XZ_LUT` / `CRC64_ISO_LUT` 在编译期展开，每轮吞吐 4 字节。
- **零堆分配**：所有中间状态位于栈或只读常量段。

## 协议

MIT — 受 Rust `crc64fast` 与 `crc` 启发的纯 CP 净室实现。
