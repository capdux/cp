[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# memmem

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation byte-substring search engine with precompiled BMH + KMP Finder, rfind, and non-overlapping count (Rust memchr::memmem parity) for CP

## 安装与导入

```bash
cap add memmem
```

```cp
import { find } from "memmem";
```

## 核心导出 API

- `export fn find(haystack: &[]u8, needle: &[]u8): i64;`
- `export fn rfind(haystack: &[]u8, needle: &[]u8): i64;`
- `export fn contains(haystack: &[]u8, needle: &[]u8): bool;`
- `export fn count(haystack: &[]u8, needle: &[]u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`memmem@1.0.0`)，许可证：MIT
