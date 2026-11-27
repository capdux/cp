[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# to-regex-range

Zero-allocation numeric range to compact regex pattern compiler and numeric range matcher for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:to-regex-range` ([https://github.com/micromatch/to-regex-range](https://github.com/micromatch/to-regex-range))

| 元信息 | 值 |
| --- | --- |
| 包名 | `to-regex-range` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add to-regex-range
```

```cp
import {
    TrrBufWriter,
    bwPush,
    countDigits,
    absI64ToU64,
    trrWriteU64,
    isSinglePattern,
    emitSubRange,
    insertStop,
} from "to-regex-range";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `TrrBufWriter` | `class` | 字段: `pos: usize, overflow: bool` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `bwPush` | `(w: &mut TrrBufWriter, out: []u8, ch: u8)` | 就地执行 `bwPush` 操作。 |
| `countDigits` | `(n: u64): usize` | 调用 `countDigits` 并返回 `usize`。 |
| `absI64ToU64` | `(v: i64): u64` | 调用 `absI64ToU64` 并返回 `u64`。 |
| `trrWriteU64` | `(w: &mut TrrBufWriter, out: []u8, v: u64)` | 就地执行 `trrWriteU64` 操作。 |
| `isSinglePattern` | `(a: u64, b: u64): bool` | 执行 `isSinglePattern` 条件判定，返回 `bool`。 |
| `emitSubRange` | `(w: &mut TrrBufWriter, out: []u8, a: u64, b: u64)` | 就地执行 `emitSubRange` 操作。 |
| `insertStop` | `(stops: []u64, count: &mut usize, val: u64)` | 就地执行 `insertStop` 操作。 |
| `sortStops` | `(stops: []u64, count: usize)` | 就地执行 `sortStops` 操作。 |
| `splitPositiveStops` | `(a: u64, b: u64, stops: []u64): usize` | 调用 `splitPositiveStops` 并返回 `usize`。 |
| `emitPositiveRange` | `(w: &mut TrrBufWriter, out: []u8, a: u64, b: u64)` | 就地执行 `emitPositiveRange` 操作。 |
| `emitNegativeRange` | `(w: &mut TrrBufWriter, out: []u8, mag_lo: u64, mag_hi: u64)` | 就地执行 `emitNegativeRange` 操作。 |
| `toRegexRange` | `(min_val: i64, max_val: i64, out: []u8): usize` | 调用 `toRegexRange` 并返回 `usize`。 |
| `matchNumericRange` | `(s: string, min_val: i64, max_val: i64): bool` | 调用 `matchNumericRange` 并返回 `bool`。 |

## 最小调用示例

```cp
import { matchNumericRange } from "to-regex-range";

pub fn main() {
    let result = matchNumericRange("example", 0, 0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
