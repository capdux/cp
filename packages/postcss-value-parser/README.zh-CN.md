[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# postcss-value-parser

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation CSS dimension, length, angle, time, and resolution parser and unit converter for CP

## 安装与导入

```bash
cap add postcss-value-parser
```

```cp
import { parseCssDimension } from "postcss-value-parser";
```

## 核心导出 API

- `export enum CssUnitKind`
- `export fn parseCssDimension(raw: string): CssDimension;`
- `export fn toPixels(dim: CssDimension, ctx: CssContext): f64?;`
- `export fn toDegrees(dim: CssDimension): f64?;`
- `export fn toMilliseconds(dim: CssDimension): f64?;`
- `export fn unitCategory(u: CssUnitKind): string;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`postcss-value-parser@1.0.0`)，许可证：MIT
