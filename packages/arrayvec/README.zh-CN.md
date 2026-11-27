[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# arrayvec

Zero-allocation stack-backed fixed-capacity containers: ArrayString64, ArrayString256, ArrayVecI64, ArrayVecU64 with O(1) swapRemove and truncating push (Rust arrayvec parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:arrayvec` ([https://github.com/bluss/arrayvec](https://github.com/bluss/arrayvec))

| 元信息 | 值 |
| --- | --- |
| 包名 | `arrayvec` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add arrayvec
```

```cp
import { ArrayString64, ArrayString256, ArrayVecI64, ArrayVecU64 } from "arrayvec";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `ArrayString64` | `class` | 字段: `buf: [STR64_CAP]u8, count: usize` · 方法: `init, fromStr, len, capacity, remaining, isEmpty, +8 项` |
| `ArrayString256` | `class` | 字段: `buf: [STR256_CAP]u8, count: usize` · 方法: `init, fromStr, len, capacity, remaining, isEmpty, +8 项` |
| `ArrayVecI64` | `class` | 字段: `items: [VEC64_CAP]i64, count: usize` · 方法: `init, len, isEmpty, isFull, tryPush, pop, +7 项` |
| `ArrayVecU64` | `class` | 字段: `items: [VEC64_CAP]u64, count: usize` · 方法: `init, len, isEmpty, isFull, tryPush, pop, +7 项` |

## 最小调用示例

```cp
import { ArrayString64 } from "arrayvec";

pub fn main() {
    // Refer to src/index.d.cp for ArrayString64 fields and methods
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
