[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# num-iter

Zero-heap stepped numeric range iterators for signed and unsigned 64-bit integers with overflow-safe inclusive/exclusive bounds (clean-room port of Rust num-iter)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:num-iter` ([https://github.com/rust-num/num-iter](https://github.com/rust-num/num-iter))

| 元信息 | 值 |
| --- | --- |
| 包名 | `num-iter` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add num-iter
```

```cp
import {
    RangeStep,
    RangeStepInclusive,
    RangeStepU64,
    nitrAbsI64,
    nitrCheckedAddI64,
    nitrDiffU64,
    nitrAdvanceSigned,
    rangeStep,
} from "num-iter";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `RangeStep` | `class` | 字段: `state: i64, stop: i64, step: i64, done: bool` · 方法: `isDone, remaining, next, nth` |
| `RangeStepInclusive` | `class` | 字段: `state: i64, stop: i64, step: i64, done: bool` · 方法: `isDone, remaining, next` |
| `RangeStepU64` | `class` | 字段: `state: u64, stop: u64, step: u64, done: bool` · 方法: `isDone, remaining, next, nth` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `nitrAbsI64` | `(v: i64): u64` | 调用 `nitrAbsI64` 并返回 `u64`。 |
| `nitrCheckedAddI64` | `(a: i64, b: i64): i64?` | 调用 `nitrCheckedAddI64` 并返回 `i64?`。 |
| `nitrDiffU64` | `(hi: i64, lo: i64): u64` | 调用 `nitrDiffU64` 并返回 `u64`。 |
| `nitrAdvanceSigned` | `(state: i64, step: i64, n: usize): i64` | 调用 `nitrAdvanceSigned` 并返回 `i64`。 |
| `rangeStep` | `(start: i64, stop: i64, step: i64): RangeStep` | 调用 `rangeStep` 并返回 `RangeStep`。 |
| `rangeStepInclusive` | `(start: i64, stop: i64, step: i64): RangeStepInclusive` | 调用 `rangeStepInclusive` 并返回 `RangeStepInclusive`。 |
| `rangeStepU64` | `(start: u64, stop: u64, step: u64): RangeStepU64` | 调用 `rangeStepU64` 并返回 `RangeStepU64`。 |
| `collectRangeStep` | `(r: &mut RangeStep, out: []i64): usize` | 调用 `collectRangeStep` 并返回 `usize`。 |

## 最小调用示例

```cp
import { nitrAbsI64 } from "num-iter";

pub fn main() {
    let result = nitrAbsI64(0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
