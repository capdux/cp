[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# suffix

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap static Suffix Array + Kasai O(N) LCP array with O(M log N) binary search, occurrence counting, and longest repeated substring detection.

## 安装与导入

```bash
cap add suffix
```

```cp
import { compareSuffixes } from "suffix";
```

## 核心导出 API

- `export fn compareSuffixes(buf: &[]u8, n: usize, a: usize, b: usize): i32;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`suffix@1.0.0`)，许可证：MIT
