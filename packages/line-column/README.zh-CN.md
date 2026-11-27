[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# line-column

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation byte offset <-> 1-based (line, col) locator, zero-copy line extractor, and LF/CRLF line counter in pure CP

## 安装与导入

```bash
cap add line-column
```

```cp
import { offsetToLineCol } from "line-column";
```

## 核心导出 API

- `export fn offsetToLineCol(text: string, offset: usize): LineCol;`
- `export fn lineColToOffset(text: string, line: usize, col: usize): i64;`
- `export fn getLine(text: string, line_1based: usize): string;`
- `export fn countLines(text: string): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`line-column@1.0.0`)，许可证：MIT
- **上游生态**：`crates.io` (`line-index@1.0.0`)，许可证：MIT
