[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# crossbeam-queue

Zero-heap bounded ring queue (RingQueueU64) and wrap-around byte ring buffer (ByteRing) in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:crossbeam-queue` ([https://github.com/crossbeam-rs/crossbeam](https://github.com/crossbeam-rs/crossbeam)) / `npm:denque` ([https://github.com/invertase/denque](https://github.com/invertase/denque))（原库名 `crossbeam-queue` → CP 包名 `crossbeam-queue`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `crossbeam-queue` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add crossbeam-queue
```

```cp
import { RingQueueU64, ByteRing } from "crossbeam-queue";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `RingQueueU64` | `class` | 字段: `head: usize, tail: usize, count: usize, capacity: usize` · 方法: `new, len, isEmpty, isFull, remaining, clear, +4 项` |
| `ByteRing` | `class` | 字段: `head: usize, tail: usize, count: usize, capacity: usize` · 方法: `new, len, isEmpty, isFull, remaining, clear, +2 项` |

## 最小调用示例

```cp
import { RingQueueU64 } from "crossbeam-queue";

pub fn main() {
    // Refer to src/index.d.cp for RingQueueU64 fields and methods
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
