[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# skiplist

Zero-allocation static-pool deterministic skip list (O(log N) ordered map, lowerBound, rangeCount, freelist reuse) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:skiplist` ([https://github.com/JP-Ellis/rust-skiplist](https://github.com/JP-Ellis/rust-skiplist))

| 元信息 | 值 |
| --- | --- |
| 包名 | `skiplist` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add skiplist
```

```cp
import { SkipEntry, SkipNode, SkipList, emptyNode } from "skiplist";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `SkipEntry` | `class` | 字段: `key: i64, value: i64, found: bool` · 方法: `none` |
| `SkipNode` | `class` | 字段: `key: i64, value: i64, level: u8, next: [8]u16` |
| `SkipList` | `class` | 字段: `nodes: [129]SkipNode, free_stack: [128]u16, free_top: usize, cur_level: u8, count: usize, rng_state: u64` · 方法: `init, len, isEmpty, clear, randomLevel, insert, +8 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `emptyNode` | `(): SkipNode` | 调用 `emptyNode` 并返回 `SkipNode`。 |

## 最小调用示例

```cp
import { emptyNode } from "skiplist";

pub fn main() {
    let result = emptyNode();
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
