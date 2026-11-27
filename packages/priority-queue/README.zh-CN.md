[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# priority-queue

Zero-allocation Indexed Binary Min-Heap with O(log N) dynamic changePriority and arbitrary remove (Rust priority-queue / heap-js parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:priority-queue` ([https://github.com/garro95/priority-queue](https://github.com/garro95/priority-queue))

| 元信息 | 值 |
| --- | --- |
| 包名 | `priority-queue` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add priority-queue
```

```cp
import { PqItem, IndexedMinHeap } from "priority-queue";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `PqItem` | `class` | 字段: `id: u16, priority: i64, valid: bool` |
| `IndexedMinHeap` | `class` | 字段: `heap_ids: [MAX_ITEMS]u16, priorities: [MAX_ITEMS]i64, pos: [MAX_ITEMS]i16, size: usize` · 方法: `init, len, isEmpty, contains, priorityOf, peek, +9 项` |

## 最小调用示例

```cp
import { PqItem } from "priority-queue";

pub fn main() {
    // Refer to src/index.d.cp for PqItem fields and methods
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
