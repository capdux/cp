[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# convert_case

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation 14-style string case converter (crates.io convert_case v0.6 parity) composing @cap/heck-case for CP.

## 安装与导入

```bash
cap add convert_case
```

```cp
import { toFlatCase } from "convert_case";
```

## 核心导出 API

- `export fn toFlatCase(src: string, out: []u8): usize;`
- `export fn toUpperFlatCase(src: string, out: []u8): usize;`
- `export fn toLowerWords(src: string, out: []u8): usize;`
- `export fn toUpperWords(src: string, out: []u8): usize;`
- `export fn toAlternatingCase(src: string, out: []u8): usize;`
- `export fn convertCase(src: string, style: u8, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`convert_case@1.0.0`)，许可证：MIT
