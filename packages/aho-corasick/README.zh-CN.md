[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# aho-corasick

Zero-allocation Aho-Corasick multi-pattern O(N + Z) automaton with LCRS trie, BFS failure links, and dictionary suffix links (Rust aho-corasick parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:aho-corasick` ([https://github.com/BurntSushi/aho-corasick](https://github.com/BurntSushi/aho-corasick))

| 元信息 | 值 |
| --- | --- |
| 包名 | `aho-corasick` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add aho-corasick
```

```cp
import { AcMatch, AhoCorasick, noMatch } from "aho-corasick";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `AcMatch` | `class` | 字段: `pattern_id: u16, start: usize, end: usize, found: bool` |
| `AhoCorasick` | `class` | 字段: `edge_char: [MAX_NODES]u8, first_child: [MAX_NODES]u16, next_sibling: [MAX_NODES]u16, fail: [MAX_NODES]u16, dict_link: [MAX_NODES]u16, out_pat: [MAX_NODES]i16, +4 项` · 方法: `init, findChild, addPattern, build, patternCount, stepState, +4 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `noMatch` | `(): AcMatch` | 调用 `noMatch` 并返回 `AcMatch`。 |

## 最小调用示例

```cp
import { noMatch } from "aho-corasick";

pub fn main() {
    let result = noMatch();
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
