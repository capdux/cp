[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# http-range-header

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

CP 官方生态包 http-range-header

## 安装与导入

```bash
cap add http-range-header
```

```cp
import { hrhIsSpace } from "http-range-header";
```

## 核心导出 API

- `export enum RangePosition`
- `export fn hrhIsSpace(b: u8): bool;`
- `export fn hrhParseU64(s: string, start: usize, end: usize): u64?;`
- `export fn parseRangeHeader(header: string, out: []SyntacticByteRange): usize;`
- `export fn validateRanges(ranges: &[]SyntacticByteRange, total_size: u64, out: []ValidatedByteRange): i64;`
- `export fn formatContentRange(start: u64, end: u64, total: u64, out: []u8): usize;`
- `export fn formatUnsatisfiableContentRange(total: u64, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`http-range-header@1.0.0`)，许可证：MIT
