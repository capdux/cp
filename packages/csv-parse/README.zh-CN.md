[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# csv-parse

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap RFC 4180 CSV/TSV record parser and serializer with quoted fields, escaped quotes, embedded newlines, custom delimiters, and spreadsheet formula guard.

## 安装与导入

```bash
cap add csv-parse
```

```cp
import { parseRecord } from "csv-parse";
```

## 核心导出 API

- `export fn parseRecord(src: &[]u8, start_pos: usize, delim: u8, out_buf: []u8, out_spans: []CsvFieldSpan): CsvParseResult;`
- `export fn formatField(field: &[]u8, delim: u8, dst: []u8): i64;`
- `export fn formatFieldSafe(field: &[]u8, delim: u8, dst: []u8): i64;`
- `export fn formatRecord(fields: &[]&[]u8, delim: u8, dst: []u8): i64;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`csv-parse@1.0.0`)，许可证：MIT
