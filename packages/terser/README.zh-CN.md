[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# terser

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation lexer-safe JS/TS/CSS code minifier (npm terser parity) preserving strings, templates, legal comments, keyword spacing, and unary operator separation for CP

## 安装与导入

```bash
cap add terser
```

```cp
import { minifyJs } from "terser";
```

## 核心导出 API

- `export fn minifyJs(input: string, opts: MinifyOptions, out: []u8): MinifyStats;`
- `export fn minifyCss(input: string, out: []u8): MinifyStats;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`terser@1.0.0`)，许可证：MIT
