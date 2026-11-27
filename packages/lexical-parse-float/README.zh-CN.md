[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# lexical-parse-float

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation pure CP strict floating-point and radix integer string parser (rust-lexical lexical-parse-float / lexical-parse-integer / lexical-util parity)

## 安装与导入

```bash
cap add lexical-parse-float
```

```cp
import { lexpToLower } from "lexical-parse-float";
```

## 核心导出 API

- `export fn lexpToLower(c: u8): u8;`
- `export fn lexpEqIgnoreCase(a: string, b: string): bool;`
- `export fn lexpDecodeDigit(c: u8): i32;`
- `export fn lexpScalePow10(val_in: f64, exp10: i32): f64;`
- `export fn parseFloat(s: string): ParseFloatResult;`
- `export fn parseIntRadix(s: string, radix: u32): ParseIntResult;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`lexical-parse-float@1.0.0`)，许可证：MIT
