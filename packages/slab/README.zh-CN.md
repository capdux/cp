[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# slab

Zero-heap pre-allocated O(1) slab allocator with intrusive free-list (clean-room port of Rust slab)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:slab` ([https://github.com/tokio-rs/slab](https://github.com/tokio-rs/slab))（原库名 `slab` → CP 包名 `slab`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `slab` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add slab
```

```cp
import { SlabEntryI64, SlabI64 } from "slab";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `SlabEntryI64` | `class` | 字段: `occupied: bool, val: i64, next_free: usize` |
| `SlabI64` | `class` | 字段: `entries: [128]SlabEntryI64, count: usize, next_free: usize, watermark: usize` · 方法: `new, len, capacity, isEmpty, isFull, vacantKey, +7 项` |

## 最小调用示例

```cp
import { SlabEntryI64 } from "slab";

pub fn main() {
    // Refer to src/index.d.cp for SlabEntryI64 fields and methods
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
