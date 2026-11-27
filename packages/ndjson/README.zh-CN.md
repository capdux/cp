[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# ndjson

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap NDJSON / JSON Lines streaming incremental line splitter, scanner, and writer with max-line overflow protection.

## 安装与导入

```bash
cap add ndjson
```

```cp
import { isBlankSlice } from "ndjson";
```

## 核心导出 API

- `export fn isBlankSlice(s: &[]u8): bool;`
- `export fn nextLine(src: &[]u8, start_pos: usize, out_next_pos: &mut usize): NdjsonLine;`
- `export fn countLines(src: &[]u8): usize;`
- `export fn writeLine(json_record: &[]u8, dst: []u8): i64;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`ndjson@1.0.0`)，许可证：BSD-2-Clause
