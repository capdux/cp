[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# vary

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap RFC 9110 HTTP Vary header manipulation, deduplication, case-insensitive lookup, token validation, and wildcard collapsing.

## 安装与导入

```bash
cap add vary
```

```cp
import { isWs } from "vary";
```

## 核心导出 API

- `export fn isWs(c: u8): bool;`
- `export fn toLowerAscii(c: u8): u8;`
- `export fn eqIgnoreCase(a: &[]u8, b: &[]u8): bool;`
- `export fn isTchar(c: u8): bool;`
- `export fn isValidFieldName(field: &[]u8): bool;`
- `export fn isWildcard(tok: &[]u8): bool;`
- `export fn hasField(header: &[]u8, field: &[]u8): bool;`
- `export fn headerHasWildcard(src: &[]u8): bool;`
- `export fn validateFieldList(field_list: &[]u8): bool;`
- `export fn dstContainsToken(dst: &[]u8, written: usize, tok: &[]u8): bool;`
- `export fn appendTokens(src: &[]u8, dst: []u8, start_pos: usize): i64;`
- `export fn append(header: &[]u8, field: &[]u8, dst: []u8): i64;`
- `export fn appendMany(header: &[]u8, fields: &[]&[]u8, dst: []u8): i64;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`vary@1.0.0`)，许可证：MIT
