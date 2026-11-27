[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# zerotrie

Zero-allocation compact left-child right-sibling prefix trie (ZeroTrie) for fast static/pooled key-to-u32 lookup and longest-prefix matching in CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:zerotrie` ([https://github.com/unicode-org/icu4x](https://github.com/unicode-org/icu4x)) / `npm:trie-prefix-tree` ([https://github.com/lyndseybrowning/trie-prefix-tree](https://github.com/lyndseybrowning/trie-prefix-tree))

| 元信息 | 值 |
| --- | --- |
| 包名 | `zerotrie` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add zerotrie
```

```cp
import { TrieNode, PrefixMatch, ZeroTrie, findChild } from "zerotrie";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `TrieNode` | `class` | 字段: `ch: u8, has_value: bool, value: u32, first_child: i32, next_sibling: i32` · 方法: `empty` |
| `PrefixMatch` | `class` | 字段: `matched_len: usize, value: u32, found: bool` |
| `ZeroTrie` | `class` | 字段: `node_count: usize, key_count: usize` · 方法: `new, init, insert, get, containsKey, hasPrefix, +1 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `findChild` | `(nodes: &[]TrieNode, parent: usize, ch: u8): i32` | 查询或读取目标数据，返回 `i32`。 |

## 最小调用示例

```cp
import { findChild } from "zerotrie";

pub fn main() {
    let result = findChild(/* nodes: &[]TrieNode */, 0, 0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
