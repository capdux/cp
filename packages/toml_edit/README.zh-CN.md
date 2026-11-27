[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# toml_edit

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation TOML v1.0 document parser, typed query/mutation engine, and canonical formatter (Rust toml_edit v0.22 parity) for CP

## 安装与导入

```bash
cap add toml_edit
```

```cp
import { tmeStripComment } from "toml_edit";
```

## 核心导出 API

- `export fn tmeStripComment(s: string): string;`
- `export fn tmeParseInt(raw: string): i64?;`
- `export fn tmeWriteByte(out: []u8, pos: usize, b: u8): usize;`
- `export fn tmeWriteQuotedStr(out: []u8, start_pos: usize, s: string): usize;`
- `export fn tmeWriteEntry(out: []u8, start_pos: usize, e: TomlEntry): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`toml_edit@1.0.0`)，许可证：MIT
