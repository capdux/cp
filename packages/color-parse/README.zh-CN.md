[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# color-parse

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation CSS Color Level 4 functional color parser (rgb/rgba/hsl/hsla with comma and space/slash syntax), formatter, and RGB <-> HSL converter for CP.

## 安装与导入

```bash
cap add color-parse
```

```cp
import { hslToRgb } from "color-parse";
```

## 核心导出 API

- `export fn hslToRgb(h: f64, s: f64, l: f64, a: f64): CssRgba;`
- `export fn rgbToHsl(r: u8, g: u8, b: u8, a: f64): CssHsla;`
- `export fn parseCssColor(input: string): CssRgba;`
- `export fn formatRgb(c: CssRgba, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`color-parse@1.0.0`)，许可证：MIT
- **上游生态**：`crates.io` (`csscolorparser@1.0.0`)，许可证：MIT
