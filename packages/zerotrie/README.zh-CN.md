[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# zerotrie

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation compact left-child right-sibling prefix trie (ZeroTrie) for fast static/pooled key-to-u32 lookup and longest-prefix matching in CP

## 安装与导入

```bash
cap add zerotrie
```

```cp
import { findChild } from "zerotrie";
```

## 核心导出 API

- `export fn findChild(nodes: &[]TrieNode, parent: usize, ch: u8): i32;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`zerotrie@1.0.0`)，许可证：MIT
- **上游生态**：`npm` (`trie-prefix-tree@1.0.0`)，许可证：MIT
