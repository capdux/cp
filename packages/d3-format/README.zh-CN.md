[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# d3-format

> 版本：`1.0.0` | 许可证：`ISC` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap D3/Python-style number format specifier parser and formatter for CP (clean-room ISC implementation of npm d3-format).

## 安装与导入

```bash
cap add d3-format
```

```cp
import { d3fIsAlign } from "d3-format";
```

## 核心导出 API

- `export fn d3fIsAlign(ch: u8): bool;`
- `export fn d3fIsType(ch: u8): bool;`
- `export fn parseFormatSpec(spec: string): FormatSpec;`
- `export fn d3fPow10F64(exp: i32): f64;`
- `export fn d3fPow10U64(exp: i32): u64;`
- `export fn d3fWriteU64Base(val: u64, base: u64, upper: bool, comma: bool, out: []u8): usize;`
- `export fn d3fWriteFixed(abs_val: f64, prec_in: i32, comma: bool, trim: bool, out: []u8): usize;`
- `export fn d3fAppendSiSuffix(exp3: i32, out: []u8, pos: usize): usize;`
- `export fn d3fDisplayLen(slice: &[]u8): usize;`
- `export fn d3fCopySlice(dst: []u8, pos: usize, src: &[]u8): usize;`
- `export fn formatNumber(spec: string, value: f64, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`d3-format@1.0.0`)，许可证：ISC
