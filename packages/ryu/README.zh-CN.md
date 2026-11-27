[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# ryu

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Pure CP zero-allocation fast float-to-string formatter (Rust ryu v1 parity, reuses @cap/itoa)

## 安装与导入

```bash
cap add ryu
```

```cp
import { isNanF64 } from "ryu";
```

## 核心导出 API

- `export fn isNanF64(val: f64): bool;`
- `export fn isInfF64(val: f64): bool;`
- `export fn isFiniteF64(val: f64): bool;`
- `export fn formatF64(val: f64, out: []u8): usize;`
- `export fn formatF64Finite(val: f64, out: []u8): usize;`
- `export fn formatF32(val: f32, out: []u8): usize;`
- `export fn formatFixedF64(val: f64, decimals: u8, out: []u8): usize;`
- `export fn formatExpF64(val: f64, precision: u8, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`ryu@1.0.0`)，许可证：MIT
