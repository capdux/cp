[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# ndarray

Zero-heap fixed-capacity 2D/N-D numerical array with stride indexing, matmul, transpose, dot, and reductions (crates.io ndarray equivalent)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:ndarray` ([https://github.com/rust-ndarray/ndarray](https://github.com/rust-ndarray/ndarray))

| 元信息 | 值 |
| --- | --- |
| 包名 | `ndarray` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add ndarray
```

```cp
import { Array2D, addArrays, matmul, dotProduct } from "ndarray";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `Array2D` | `class` | 字段: `rows: usize, cols: usize, data: [64]f64` · 方法: `zeros, filled, eye, fromSlice, elemCount, get, +8 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `addArrays` | `(a: Array2D, b: Array2D): Array2D?` | 写入或更新状态数据。 |
| `matmul` | `(a: Array2D, b: Array2D): Array2D?` | 调用 `matmul` 并返回 `Array2D?`。 |
| `dotProduct` | `(a: []f64, b: []f64): f64?` | 调用 `dotProduct` 并返回 `f64?`。 |

## 最小调用示例

```cp
import { addArrays } from "ndarray";

pub fn main() {
    let result = addArrays(/* a: Array2D */, /* b: Array2D */);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
