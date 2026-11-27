[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# fill-range

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap numeric, alphabetic, stepped, and zero-padded range expansion engine (npm fill-range parity).

## 安装与导入

```bash
cap add fill-range
```

```cp
import { isAsciiAlpha } from "fill-range";
```

## 核心导出 API

- `export fn isAsciiAlpha(c: u8): bool;`
- `export fn absStepU64(step: i64): u64;`
- `export fn absDiffI64(a: i64, b: i64): u64;`
- `export fn countRange(start: i64, end: i64, step: i64): usize;`
- `export fn expandNumbers(start: i64, end: i64, step: i64, out: []i64): i64;`
- `export fn expandChars(start: u8, end: u8, step: usize, out: []u8): i64;`
- `export fn parseRangeInt(s: &[]u8): ParsedInt;`
- `export fn writeSlice(dst: []u8, pos: usize, src: &[]u8): i64;`
- `export fn writePaddedInt(dst: []u8, start_pos: usize, val: i64, pad_width: usize): i64;`
- `export fn formatRange(start: &[]u8, end: &[]u8, step: i64, sep: &[]u8, dst: []u8): i64;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`fill-range@1.0.0`)，许可证：MIT
