[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# humansize

> 版本：`1.0.0` | 许可证：`MIT OR Apache-2.0` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap human-readable byte size formatter (Decimal, Binary, Windows) in pure CP (modeled after Rust humansize v2)

## 安装与导入

```bash
cap add humansize
```

```cp
import { hszWriteSlice } from "humansize";
```

## 核心导出 API

- `export enum BaseUnit`
- `export fn hszWriteSlice(out: []u8, pos: usize, s: string): usize;`
- `export fn hszWriteU64(out: []u8, pos: usize, val: u64): usize;`
- `export fn hszWriteFixedFrac(out: []u8, pos: usize, frac: u64, places: u8): usize;`
- `export fn hszPow10(places: u8): u64;`
- `export fn hszUnitStr(base: BaseUnit, idx: usize, long_units: bool, is_one: bool): string;`
- `export fn formatSize(bytes: u64, opts: FormatSizeOptions, out: []u8): usize;`
- `export fn formatDecimal(bytes: u64, out: []u8): usize;`
- `export fn formatBinary(bytes: u64, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`humansize@1.0.0`)，许可证：MIT OR Apache-2.0
