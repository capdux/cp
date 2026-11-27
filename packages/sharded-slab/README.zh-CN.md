[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# sharded-slab

Zero-allocation O(1) generational slab allocator and ABA-safe handle pool (Rust slab + generational-arena parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:sharded-slab` ([https://github.com/hawkw/sharded-slab](https://github.com/hawkw/sharded-slab))（原库名 `sharded-slab` → CP 包名 `sharded-slab`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `sharded-slab` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add sharded-slab
```

```cp
import { SlabKey, SlabI64, SlabU64, nextGeneration } from "sharded-slab";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `SlabKey` | `class` | 字段: `index: u32, generation: u32` · 方法: `equals` |
| `SlabI64` | `class` | 字段: `slots: [SLAB_CAPACITY]i64, occupied: [SLAB_CAPACITY]bool, generations: [SLAB_CAPACITY]u32, next_free: [SLAB_CAPACITY]i32, free_head: i32, next_alloc: usize, +2 项` · 方法: `init, insert, contains, get, set, remove, +4 项` |
| `SlabU64` | `class` | 字段: `slots: [SLAB_CAPACITY]u64, occupied: [SLAB_CAPACITY]bool, generations: [SLAB_CAPACITY]u32, next_free: [SLAB_CAPACITY]i32, free_head: i32, next_alloc: usize, +2 项` · 方法: `init, insert, contains, get, set, remove, +4 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `nextGeneration` | `(gen: u32): u32` | 调用 `nextGeneration` 并返回 `u32`。 |

## 最小调用示例

```cp
import { nextGeneration } from "sharded-slab";

pub fn main() {
    let result = nextGeneration(0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
