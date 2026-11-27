[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# ordered-float

Zero-heap IEEE-754 total ordering, canonical hashing, and NaN-safe min/max/clamp for f64 and f32 (clean-room port of Rust ordered-float)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:ordered-float` ([https://github.com/reem/rust-ordered-float](https://github.com/reem/rust-ordered-float))

| 元信息 | 值 |
| --- | --- |
| 包名 | `ordered-float` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add ordered-float
```

```cp
import {
    OrderedF64,
    NotNanF64,
    isNanF64,
    isInfiniteF64,
    isFiniteF64,
    cmpOrderedF64,
    eqOrderedF64,
    hashOrderedF64,
} from "ordered-float";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `OrderedF64` | `class` | 字段: `val: f64` · 方法: `new, intoInner, isNan, isFinite, cmp, eq, +4 项` |
| `NotNanF64` | `class` | 字段: `val: f64` · 方法: `new, intoInner, cmp, eq, hash` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `isNanF64` | `(x: f64): bool` | 执行 `isNanF64` 条件判定，返回 `bool`。 |
| `isInfiniteF64` | `(x: f64): bool` | 执行 `isInfiniteF64` 条件判定，返回 `bool`。 |
| `isFiniteF64` | `(x: f64): bool` | 执行 `isFiniteF64` 条件判定，返回 `bool`。 |
| `cmpOrderedF64` | `(a: f64, b: f64): i32` | 调用 `cmpOrderedF64` 并返回 `i32`。 |
| `eqOrderedF64` | `(a: f64, b: f64): bool` | 调用 `eqOrderedF64` 并返回 `bool`。 |
| `hashOrderedF64` | `(x: f64): u64` | 执行 `hashOrderedF64` 条件判定，返回 `bool`。 |
| `sortOrderedF64` | `(arr: []f64)` | 就地执行 `sortOrderedF64` 操作。 |

## 最小调用示例

```cp
import { isNanF64 } from "ordered-float";

pub fn main() {
    let result = isNanF64(0.0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
