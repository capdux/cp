[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# to-regex-range

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation numeric range to compact regex pattern compiler and numeric range matcher for CP

## 安装与导入

```bash
cap add to-regex-range
```

```cp
import { bwPush } from "to-regex-range";
```

## 核心导出 API

- `export fn bwPush(w: &mut TrrBufWriter, out: []u8, ch: u8);`
- `export fn countDigits(n: u64): usize;`
- `export fn absI64ToU64(v: i64): u64;`
- `export fn trrWriteU64(w: &mut TrrBufWriter, out: []u8, v: u64);`
- `export fn isSinglePattern(a: u64, b: u64): bool;`
- `export fn emitSubRange(w: &mut TrrBufWriter, out: []u8, a: u64, b: u64);`
- `export fn insertStop(stops: []u64, count: &mut usize, val: u64);`
- `export fn sortStops(stops: []u64, count: usize);`
- `export fn splitPositiveStops(a: u64, b: u64, stops: []u64): usize;`
- `export fn emitPositiveRange(w: &mut TrrBufWriter, out: []u8, a: u64, b: u64);`
- `export fn emitNegativeRange(w: &mut TrrBufWriter, out: []u8, mag_lo: u64, mag_hi: u64);`
- `export fn toRegexRange(min_val: i64, max_val: i64, out: []u8): usize;`
- `export fn matchNumericRange(s: string, min_val: i64, max_val: i64): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`to-regex-range@1.0.0`)，许可证：MIT
