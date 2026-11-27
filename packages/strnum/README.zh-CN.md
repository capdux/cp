[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# strnum

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap configurable string-to-number parser for CP (clean-room MIT implementation of npm strnum).

## 安装与导入

```bash
cap add strnum
```

```cp
import { snumInvalid } from "strnum";
```

## 核心导出 API

- `export fn snumInvalid(): StrNumResult;`
- `export fn snumIsSpace(ch: u8): bool;`
- `export fn snumHexDigit(ch: u8): i32;`
- `export fn snumScalePow10(val: f64, exp10: i32): f64;`
- `export fn parseStrNum(input: string, opts: StrNumOptions): StrNumResult;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`strnum@1.0.0`)，许可证：MIT
