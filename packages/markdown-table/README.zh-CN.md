[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# markdown-table

> 版本：`3.0.4` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap GFM Markdown table renderer with automatic column width calculation and alignment padding

## 安装与导入

```bash
cap add markdown-table
```

```cp
import { mdtbWriteByte } from "markdown-table";
```

## 核心导出 API

- `export enum TableAlign`
- `export fn mdtbWriteByte(out: []u8, pos: usize, ch: u8): usize;`
- `export fn mdtbWriteStr(out: []u8, pos: usize, s: string): usize;`
- `export fn mdtbWriteRepeat(out: []u8, pos: usize, ch: u8, count: usize): usize;`
- `export fn mdtbGetAlign(aligns: &[]TableAlign, col: usize): TableAlign;`
- `export fn mdtbMinWidth(al: TableAlign): usize;`
- `export fn mdtbWriteCell(out: []u8, pos: usize, text: string, width: usize, al: TableAlign): usize;`
- `export fn mdtbWriteDelim(out: []u8, pos: usize, width: usize, al: TableAlign): usize;`
- `export fn mdtbColWidth(headers: &[]string, rows: &[]string, num_cols: usize, num_rows: usize, col: usize, al: TableAlign): usize;`
- `export fn renderMarkdownTable(headers: &[]string, rows: &[]string, num_cols: usize, num_rows: usize, aligns: &[]TableAlign, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`markdown-table@3.0.4`)（https://github.com/wooorm/markdown-table），许可证：MIT
