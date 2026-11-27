[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# hashring

Zero-allocation consistent hash ring with virtual nodes, O(log V) binary search routing, and deduplicated replica selection for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:hashring` ([https://github.com/jeromefroe/hashring-rs](https://github.com/jeromefroe/hashring-rs))（原库名 `hashring` → CP 包名 `hashring`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `hashring` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add hashring
```

```cp
import {
    RingPoint,
    ReplicaList,
    HashRing,
    mulWrap64,
    avalanche64,
    fnv1a64Feed,
    hashKey,
    hashVnode,
} from "hashring";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `RingPoint` | `class` | 字段: `hash: u64, node_idx: u16` |
| `ReplicaList` | `class` | 字段: `nodes: [8]string, count: usize` · 方法: `empty, nodeAt` |
| `HashRing` | `class` | 字段: `nodes: [32]string, node_active: [32]bool, active_nodes: usize, vnodes_per_node: u16, ring: [256]RingPoint, ring_len: usize` · 方法: `init, nodeCount, vnodeCount, addNode, removeNode, lowerBound, +2 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `mulWrap64` | `(a: u64, b: u64): u64` | 调用 `mulWrap64` 并返回 `u64`。 |
| `avalanche64` | `(x: u64): u64` | 调用 `avalanche64` 并返回 `u64`。 |
| `fnv1a64Feed` | `(state: u64, b: u8): u64` | 调用 `fnv1a64Feed` 并返回 `u64`。 |
| `hashKey` | `(key: string): u64` | 执行 `hashKey` 条件判定，返回 `bool`。 |
| `hashVnode` | `(name: string, v_idx: u16): u64` | 执行 `hashVnode` 条件判定，返回 `bool`。 |
| `eqStr` | `(a: string, b: string): bool` | 调用 `eqStr` 并返回 `bool`。 |

## 最小调用示例

```cp
import { hashKey } from "hashring";

pub fn main() {
    let result = hashKey("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
