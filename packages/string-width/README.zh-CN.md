[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# string-width

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation visual terminal column width calculator (npm string-width v7 parity) composing @cap/ansi-strip and @cap/unicode-width for CP

## 安装与导入

```bash
cap add string-width
```

```cp
import { stringWidthWithOptions } from "string-width";
```

## 核心导出 API

- `export fn stringWidthWithOptions(input: string, opts: StringWidthOptions): usize;`
- `export fn stringWidth(input: string): usize;`
- `export fn maxLineWidthWithOptions(multiline: string, opts: StringWidthOptions): usize;`
- `export fn maxLineWidth(multiline: string): usize;`
- `export fn padEndVisible(input: string, target_cols: usize, fill_byte: u8, out: []u8): usize;`
- `export fn padStartVisible(input: string, target_cols: usize, fill_byte: u8, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`string-width@1.0.0`)，许可证：MIT
