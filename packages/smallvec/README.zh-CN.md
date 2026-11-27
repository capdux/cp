[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# smallvec

Zero-heap inline stack vector with push/pop/insert/remove/swapRemove/dedup (clean-room CP implementation of crates.io smallvec)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:smallvec` ([https://github.com/servo/rust-smallvec](https://github.com/servo/rust-smallvec))

| 元信息 | 值 |
| --- | --- |
| 包名 | `smallvec` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add smallvec
```

```cp
import { SmallPopU64, SmallVecU64, SmallVecU8 } from "smallvec";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `SmallPopU64` | `class` | 字段: `ok: bool, value: u64` |
| `SmallVecU64` | `class` | 字段: `data: [64]u64, used: usize` · 方法: `new, len, capacity, isEmpty, isFull, clear, +12 项` |
| `SmallVecU8` | `class` | 字段: `data: [256]u8, used: usize` · 方法: `new, len, capacity, isEmpty, clear, push, +2 项` |

## 最小调用示例

```cp
import { SmallPopU64 } from "smallvec";

pub fn main() {
    // Refer to src/index.d.cp for SmallPopU64 fields and methods
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
