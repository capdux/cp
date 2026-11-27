[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# lru-slab

Zero-heap pre-allocated O(1) intrusive LRU slab cache matching crates.io #415 lru-slab (358M+ downloads) in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:lru-slab` ([https://github.com/rustls/lru-slab](https://github.com/rustls/lru-slab))

| 元信息 | 值 |
| --- | --- |
| 包名 | `lru-slab` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add lru-slab
```

```cp
import { GetResult, EvictResult, SlabEntry, LruSlab } from "lru-slab";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `GetResult` | `class` | 字段: `ok: bool, val: u64` |
| `EvictResult` | `class` | 字段: `idx: i32, key: u64, val: u64` |
| `SlabEntry` | `class` | 字段: `occupied: bool, key: u64, val: u64, prev: i32, next: i32` |
| `LruSlab` | `class` | 字段: `head: i32, tail: i32, free_head: i32, count: usize, capacity: usize` · 方法: `new, init, len, isEmpty, isFull, unlinkLru, +8 项` |

## 最小调用示例

```cp
import { GetResult } from "lru-slab";

pub fn main() {
    // Refer to src/index.d.cp for GetResult fields and methods
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
