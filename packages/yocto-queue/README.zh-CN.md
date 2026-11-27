[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# yocto-queue

Tiny O(1) FIFO queue data structure (clean-room zero-heap CP implementation of npm yocto-queue)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:yocto-queue` ([https://github.com/sindresorhus/yocto-queue](https://github.com/sindresorhus/yocto-queue))

| 元信息 | 值 |
| --- | --- |
| 包名 | `yocto-queue` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add yocto-queue
```

```cp
import { PopU64Result, PopI64Result, YoctoQueueU64, YoctoQueueI64 } from "yocto-queue";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `PopU64Result` | `class` | 字段: `ok: bool, value: u64` |
| `PopI64Result` | `class` | 字段: `ok: bool, value: i64` |
| `YoctoQueueU64` | `class` | 字段: `buf: [256]u64, head: usize, tail: usize, count: usize` · 方法: `new, size, capacity, isEmpty, isFull, enqueue, +5 项` |
| `YoctoQueueI64` | `class` | 字段: `buf: [256]i64, head: usize, tail: usize, count: usize` · 方法: `new, size, isEmpty, isFull, enqueue, dequeue, +2 项` |

## 最小调用示例

```cp
import { PopU64Result } from "yocto-queue";

pub fn main() {
    // Refer to src/index.d.cp for PopU64Result fields and methods
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
