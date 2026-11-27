[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# widest-line

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation widest visual terminal line width calculator (npm widest-line parity) composing string-width for CP

## 安装与导入

```bash
cap add widest-line
```

```cp
import { wlTrimCr } from "widest-line";
```

## 核心导出 API

- `export fn wlTrimCr(line: string): string;`
- `export fn wlLineWidthWithTab(line: string, tab_width: usize): usize;`
- `export fn countLines(s: string): usize;`
- `export fn widestLine(s: string): usize;`
- `export fn widestLineWithOptions(s: string, opts: StringWidthOptions): usize;`
- `export fn lineWidths(s: string, out: []usize): usize;`
- `export fn lineWidthsWithOptions(s: string, opts: StringWidthOptions, out: []usize): usize;`
- `export fn widestLineWithTab(s: string, tab_width: usize): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`widest-line@1.0.0`)，许可证：MIT
