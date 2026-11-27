[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# sieve-cache

Zero-allocation NSDI'24 SIEVE cache eviction engine with static doubly-linked pool and open-addressing index for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:sieve-cache` ([https://github.com/jedisct1/rust-sieve-cache](https://github.com/jedisct1/rust-sieve-cache))

| 元信息 | 值 |
| --- | --- |
| 包名 | `sieve-cache` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add sieve-cache
```

```cp
import {
    SieveNode,
    SieveCache,
    emptyNode,
    mulWrap64,
    hashSlot,
} from "sieve-cache";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `SieveNode` | `class` | 字段: `key: u64, value: i64, visited: bool, in_use: bool, prev: u16, next: u16` |
| `SieveCache` | `class` | 字段: `nodes: [128]SieveNode, buckets: [256]u16, free_stack: [128]u16, free_top: usize, head: u16, tail: u16, +7 项` · 方法: `init, len, capacity, isEmpty, hits, misses, +14 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `emptyNode` | `(): SieveNode` | 调用 `emptyNode` 并返回 `SieveNode`。 |
| `mulWrap64` | `(a: u64, b: u64): u64` | 调用 `mulWrap64` 并返回 `u64`。 |
| `hashSlot` | `(key: u64): usize` | 执行 `hashSlot` 条件判定，返回 `bool`。 |

## 最小调用示例

```cp
import { hashSlot } from "sieve-cache";

pub fn main() {
    let result = hashSlot(0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
