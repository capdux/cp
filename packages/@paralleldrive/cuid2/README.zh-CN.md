[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# @paralleldrive/cuid2

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap collision-resistant, URL-safe, lowercase CUID2 ID generator and validator (clean-room port of npm @paralleldrive/cuid2)

## 安装与导入

```bash
cap add @paralleldrive/cuid2
```

```cp
import { c2Mix } from "@paralleldrive/cuid2";
```

## 核心导出 API

- `export fn c2Mix(x: u64): u64;`
- `export fn c2Base36Char(v: u64): u8;`
- `export fn c2LetterChar(v: u64): u8;`
- `export fn isCuid2(id: string): bool;`
- `export fn isCuid2WithBounds(id: string, min_len: usize, max_len: usize): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`@paralleldrive/cuid2@1.0.0`)，许可证：MIT
