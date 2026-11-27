[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# highlight.js

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation multi-language lexer and syntax highlighter for Code, JSON, and SQL with ANSI and XSS-safe HTML output (highlight.js / prismjs parity) for CP

## 安装与导入

```bash
cap add highlight.js
```

```cp
import { tokenize } from "highlight.js";
```

## 核心导出 API

- `export fn tokenize(src: &[]u8, lang: u8, out: []Token): usize;`
- `export fn highlightAnsi(src: &[]u8, lang: u8, dst: []u8): i64;`
- `export fn highlightHtml(src: &[]u8, lang: u8, dst: []u8): i64;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`highlight.js@1.0.0`)，许可证：BSD-3-Clause
