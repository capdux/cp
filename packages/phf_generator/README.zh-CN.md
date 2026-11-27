[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# phf_generator

Zero-allocation CHD Displace-Hash-Compress perfect hash table builder and O(1) lookup for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:phf_generator` ([https://github.com/rust-phf/rust-phf](https://github.com/rust-phf/rust-phf))（原库名 `phf_generator` → CP 包名 `phf_generator`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `phf_generator` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add phf_generator
```

```cp
import {
    PhfHashes,
    PhfBuildResult,
    add64,
    mul64,
    mix64,
    hashKey,
    displace,
    buildPhfWithSeed,
} from "phf_generator";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `PhfHashes` | `class` | 字段: `g: u32, f1: u32, f2: u32` |
| `PhfBuildResult` | `class` | 字段: `ok: bool, seed: u64, table_len: usize, disp_len: usize` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `add64` | `(a: u64, b: u64): u64` | 写入或更新状态数据。 |
| `mul64` | `(a: u64, b: u64): u64` | 调用 `mul64` 并返回 `u64`。 |
| `mix64` | `(z_in: u64): u64` | 调用 `mix64` 并返回 `u64`。 |
| `hashKey` | `(key: string, seed: u64): PhfHashes` | 执行 `hashKey` 条件判定，返回 `bool`。 |
| `displace` | `(f1: u32, f2: u32, d1: u32, d2: u32, len: usize): usize` | 调用 `displace` 并返回 `usize`。 |
| `buildPhfWithSeed` | `(keys: &[]string, start_seed: u64, disps_d1: []u32, disps_d2: []u32, slot_to_key: []i32): PhfBuildResult` | 调用 `buildPhfWithSeed` 并返回 `PhfBuildResult`。 |
| `buildPhf` | `(keys: &[]string, disps_d1: []u32, disps_d2: []u32, slot_to_key: []i32): PhfBuildResult` | 调用 `buildPhf` 并返回 `PhfBuildResult`。 |
| `lookupIndex` | `(key: string, seed: u64, keys: &[]string, disps_d1: &[]u32, disps_d2: &[]u32, slot_to_key: &[]i32): i64` | 查询或读取目标数据，返回 `i64`。 |

## 最小调用示例

```cp
import { hashKey } from "phf_generator";

pub fn main() {
    let result = hashKey("example", 0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
