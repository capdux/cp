[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# qs

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation query string parser and serializer with nested brackets, dot notation, arrays, strict null handling, and DoS limits for CP

## 安装与导入

```bash
cap add qs
```

```cp
import { hexVal } from "qs";
```

## 核心导出 API

- `export fn hexVal(c: u8): u8?;`
- `export fn isUnreserved(c: u8): bool;`
- `export fn needsDecode(s: &[]u8, plus_as_space: bool): bool;`
- `export fn parseDecimalIndex(s: &[]u8): usize?;`
- `export fn percentDecode(input: &[]u8, plus_as_space: bool, out: []u8): &[]u8 ! QsError;`
- `export fn percentEncode(input: &[]u8, space_as_plus: bool, out: []u8): &[]u8 ! QsError;`
- `export fn makeSegment(slice: &[]u8, base_off: u16, seg_rel_off: usize, in_pool: bool, is_empty_bracket: bool): QsPathSegment;`
- `export fn hasByte(s: &[]u8, b: u8): bool;`
- `export fn parseKeySegments(dec_key: &[]u8, key_off: u16, key_in_pool: bool, allow_dots: bool, max_depth: usize, out_segs: &mut [8]QsPathSegment): usize ! QsError;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`qs@1.0.0`)，许可证：MIT
