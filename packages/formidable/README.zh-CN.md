[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# formidable

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap pure CP multipart/form-data parser for fields and file uploads (clean-room MIT implementation of npm formidable v3)

## 安装与导入

```bash
cap add formidable
```

```cp
import { frmToLower } from "formidable";
```

## 核心导出 API

- `export fn frmToLower(b: u8): u8;`
- `export fn frmIsOws(b: u8): bool;`
- `export fn frmIsTokenChar(b: u8): bool;`
- `export fn frmStrEq(a: string, b: string): bool;`
- `export fn frmEqIgnoreCase(a: string, b: string): bool;`
- `export fn frmTrimOws(s: string): string;`
- `export fn frmSkipOws(s: string, start: usize): usize;`
- `export fn frmMatchesAt(hay: string, pos: usize, needle: string): bool;`
- `export fn frmIsBoundaryTerminator(body: string, after_pos: usize): bool;`
- `export fn frmFindFirstBoundary(body: string, boundary: string): usize;`
- `export fn frmFindNextBoundary(body: string, start: usize, boundary: string): usize;`
- `export fn frmExtractDispositionName(header: string): string;`
- `export fn frmHasFilenameParam(header: string): bool;`
- `export fn parseMultipartForm(content_type: string, body: string, opts: FormidableOptions): ParsedForm;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`formidable@1.0.0`)，许可证：MIT
