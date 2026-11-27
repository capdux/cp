[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# wildmatch

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap linear-time wildcard (*, ?, custom wildcards & case-insensitive) pattern matcher (clean-room port of Rust wildmatch)

## 安装与导入

```bash
cap add wildmatch
```

```cp
import { wmByteEq } from "wildmatch";
```

## 核心导出 API

- `export fn wmByteEq(a: u8, b: u8, ignore_case: bool): bool;`
- `export fn wmMatchCore(pattern: string, input: string, any_many: u8, any_one: u8, ignore_case: bool, allow_escape: bool): bool;`
- `export fn wildmatch(pattern: string, input: string): bool;`
- `export fn wildmatchIgnoreCase(pattern: string, input: string): bool;`
- `export fn wildmatchCustom(pattern: string, input: string, any_many: u8, any_one: u8): bool;`
- `export fn hasWildcards(pattern: string): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`wildmatch@1.0.0`)，许可证：MIT
