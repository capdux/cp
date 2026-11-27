[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# radix_trie

Zero-allocation LCRS static-pool byte prefix tree (Trie) with exact lookup, deletion, prefix count, and longest prefix match (LPM) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:radix_trie` ([https://github.com/michaelsproul/rust_radix_trie](https://github.com/michaelsproul/rust_radix_trie))（原库名 `radix_trie` → CP 包名 `radix_trie`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `radix_trie` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add radix_trie
```

```cp
import { TrieNode, LongestMatch, Trie } from "radix_trie";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `TrieNode` | `class` | 字段: `byte: u8, is_end: bool, value: i64, first_child: u16, next_sibling: u16, subtree_words: u16` · 方法: `empty` |
| `LongestMatch` | `class` | 字段: `matched_len: usize, value: i64, found: bool` · 方法: `none` |
| `Trie` | `class` | 字段: `nodes: [512]TrieNode, used_nodes: usize, word_count: usize` · 方法: `init, clear, len, nodeCount, findChild, walkPrefix, +7 项` |

## 最小调用示例

```cp
import { TrieNode } from "radix_trie";

pub fn main() {
    // Refer to src/index.d.cp for TrieNode fields and methods
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
