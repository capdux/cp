[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# codespan-reporting

> 版本：`0.11.1` | 许可证：`Apache-2.0` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap Rustc-style multi-line source code diagnostic renderer with line numbers, gutter, and caret underlines

## 安装与导入

```bash
cap add codespan-reporting
```

```cp
import { severityLabel } from "codespan-reporting";
```

## 核心导出 API

- `export enum DiagSeverity`
- `export fn severityLabel(sev: DiagSeverity): string;`
- `export fn locateOffset(source: string, offset: usize): LineCol;`
- `export fn renderDiagnostic(source: string, diag: Diagnostic, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`codespan-reporting@0.11.1`)（https://github.com/brendanzab/codespan），许可证：Apache-2.0
