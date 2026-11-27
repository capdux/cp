[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# lexical-write-float

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation pure CP integer and floating-point formatter (rust-lexical lexical-write-float / lexical-write-integer parity)

## 安装与导入

```bash
cap add lexical-write-float
```

```cp
import { lexwCopyStr } from "lexical-write-float";
```

## 核心导出 API

- `export fn lexwCopyStr(out: []u8, pos: usize, src: string): usize;`
- `export fn lexwWritePaddedU64(out: []u8, pos: usize, val: u64, width: usize): usize;`
- `export fn lexwWriteSpecialFloat(val: f64, out: []u8): usize;`
- `export fn writeUintRadix(val: u64, radix: u32, uppercase: bool, out: []u8): usize;`
- `export fn writeIntRadix(val: i64, radix: u32, uppercase: bool, out: []u8): usize;`
- `export fn writeFloatFixed(val: f64, decimals: u8, out: []u8): usize;`
- `export fn writeFloatSci(val: f64, precision: u8, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`lexical-write-float@1.0.0`)，许可证：MIT
