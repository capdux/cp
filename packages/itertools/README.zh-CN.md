[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# itertools

Pure CP zero-allocation iterator combinators and sequence algorithms (Rust itertools parity)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:itertools` ([https://github.com/rust-itertools/itertools](https://github.com/rust-itertools/itertools))

| 元信息 | 值 |
| --- | --- |
| 包名 | `itertools` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add itertools
```

```cp
import {
    MinMaxResult,
    PartitionResult,
    CartesianPair,
    itCopyBytes,
    minmax,
    isSorted,
    isSortedDesc,
    allEqual,
} from "itertools";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `MinMaxResult` | `class` | 字段: `empty: bool, min: i64, max: i64` |
| `PartitionResult` | `class` | 字段: `true_count: usize, false_count: usize` |
| `CartesianPair` | `class` | 字段: `found: bool, first: i64, second: i64` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `itCopyBytes` | `(dst: []u8, pos: &mut usize, src: &[]u8): bool` | 调用 `itCopyBytes` 并返回 `bool`。 |
| `minmax` | `(slice: &[]i64): MinMaxResult` | 调用 `minmax` 并返回 `MinMaxResult`。 |
| `isSorted` | `(slice: &[]i64): bool` | 执行 `isSorted` 条件判定，返回 `bool`。 |
| `isSortedDesc` | `(slice: &[]i64): bool` | 执行 `isSortedDesc` 条件判定，返回 `bool`。 |
| `allEqual` | `(slice: &[]i64): bool` | 调用 `allEqual` 并返回 `bool`。 |
| `uniqueI64` | `(src: &[]i64, out: []i64): usize` | 调用 `uniqueI64` 并返回 `usize`。 |
| `dedupConsecutive` | `(src: &[]i64, out: []i64): usize` | 调用 `dedupConsecutive` 并返回 `usize`。 |
| `interleave` | `(a: &[]i64, b: &[]i64, out: []i64): usize` | 调用 `interleave` 并返回 `usize`。 |
| `intersperse` | `(src: &[]i64, sep: i64, out: []i64): usize` | 调用 `intersperse` 并返回 `usize`。 |
| `mergeSorted` | `(a: &[]i64, b: &[]i64, out: []i64): usize` | 调用 `mergeSorted` 并返回 `usize`。 |
| `joinStrings` | `(items: &[]string, sep: string, out: []u8): usize` | 调用 `joinStrings` 并返回 `usize`。 |
| `partitionByThreshold` | `(src: &[]i64, threshold: i64, out_true: []i64, out_false: []i64): PartitionResult` | 调用 `partitionByThreshold` 并返回 `PartitionResult`。 |
| `cartesianProductCount` | `(a_len: usize, b_len: usize): usize` | 调用 `cartesianProductCount` 并返回 `usize`。 |
| `nthCartesianPair` | `(a: &[]i64, b: &[]i64, idx: usize): CartesianPair` | 调用 `nthCartesianPair` 并返回 `CartesianPair`。 |
| `chunkCount` | `(total_len: usize, chunk_size: usize): usize` | 调用 `chunkCount` 并返回 `usize`。 |
| `windowCount` | `(total_len: usize, win_size: usize): usize` | 调用 `windowCount` 并返回 `usize`。 |

## 最小调用示例

```cp
import { joinStrings } from "itertools";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = joinStrings(/* items: &[]string */, "example", out_buf[0..256]);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
