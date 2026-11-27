[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# yallist

Zero-allocation O(1) doubly linked list backed by a fixed 64-slot freelist pool (npm yallist v5 parity) in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：吸收自 **npm** [`yallist`](https://github.com/isaacs/yallist)（`ISC`）（包名 `yallist`）。

| 元信息 | 值 |
| --- | --- |
| 包名 | `yallist` |
| 版本 | `1.0.0` |
| 许可证 | `ISC` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add yallist
```

```cp
import { YallistU64, yalValidHandle } from "yallist";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `YallistU64` | `class` | 字段: `values: [64]u64, prev: [64]i32, next: [64]i32, alive: [64]bool, free_stack: [64]i32, free_top: usize, +3 项` · 方法: `new, len, capacity, isEmpty, push, pop, +10 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `yalValidHandle` | `(handle: i32): bool` | 调用 `yalValidHandle` 并返回 `bool`。 |

## 最小调用示例

```cp
import { yalValidHandle } from "yallist";

pub fn main() {
    let result = yalValidHandle(0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
