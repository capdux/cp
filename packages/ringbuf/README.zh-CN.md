[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# ringbuf

Zero-heap O(1) circular ring buffers (RingBufU8 & RingBufI64) with slice batching and overwrite mode (clean-room port of Rust ringbuf)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:ringbuf` ([https://github.com/agerasev/ringbuf](https://github.com/agerasev/ringbuf))

| 元信息 | 值 |
| --- | --- |
| 包名 | `ringbuf` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add ringbuf
```

```cp
import { RingBufU8, RingBufI64 } from "ringbuf";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `RingBufU8` | `class` | 字段: `buf: [256]u8, head: usize, tail: usize, count: usize` · 方法: `new, len, capacity, freeLen, isEmpty, isFull, +7 项` |
| `RingBufI64` | `class` | 字段: `buf: [64]i64, head: usize, tail: usize, count: usize` · 方法: `new, len, capacity, isEmpty, isFull, push, +4 项` |

## 最小调用示例

```cp
import { RingBufU8 } from "ringbuf";

pub fn main() {
    // Refer to src/index.d.cp for RingBufU8 fields and methods
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
