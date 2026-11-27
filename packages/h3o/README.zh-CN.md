[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# h3o

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap Uber H3 64-bit hexagonal hierarchical spatial index cell parser, constructor, hierarchy traversal, and area table (clean-room port of Rust h3o)

## 安装与导入

```bash
cap add h3o
```

```cp
import { h3AllLow63 } from "h3o";
```

## 核心导出 API

- `export fn h3AllLow63(): u64;`
- `export fn h3DigitShift(r: u8): u6;`
- `export fn h3IsPentagonBaseCell(bc: u8): bool;`
- `export fn h3HexDigitVal(c: u8): u8;`
- `export fn h3HexChar(nibble: u8): u8;`
- `export fn h3ClearBits(val: u64, mask: u64): u64;`
- `export fn parseH3Hex(hex: string): H3Cell?;`
- `export fn avgHexAreaKm2(res: u8): f64;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`h3o@1.0.0`)，许可证：BSD-3-Clause
