[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# bitflags

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap 64-bit bitflags set algebra (contains, intersects, insert, remove, toggle, difference, popcount, trailing/leading zeros) for CP (compatible with Rust bitflags & npm bitfield)

## 安装与导入

```bash
cap add bitflags
```

```cp
import { empty } from "bitflags";
```

## 核心导出 API

- `export fn empty(): Flags64;`
- `export fn fromBits(raw: u64): Flags64;`
- `export fn fromBitIndex(idx: u8): Flags64;`
- `export fn isEmpty(f: Flags64): bool;`
- `export fn contains(f: Flags64, other: Flags64): bool;`
- `export fn intersects(f: Flags64, other: Flags64): bool;`
- `export fn unionFlags(a: Flags64, b: Flags64): Flags64;`
- `export fn intersection(a: Flags64, b: Flags64): Flags64;`
- `export fn difference(a: Flags64, b: Flags64): Flags64;`
- `export fn symmetricDifference(a: Flags64, b: Flags64): Flags64;`
- `export fn insert(f: &mut Flags64, other: Flags64);`
- `export fn remove(f: &mut Flags64, other: Flags64);`
- `export fn toggle(f: &mut Flags64, other: Flags64);`
- `export fn setCond(f: &mut Flags64, other: Flags64, enabled: bool);`
- `export fn truncate(raw: u64, valid_mask: u64): Flags64;`
- `export fn countBits(f: Flags64): u32;`
- `export fn lowestBitIndex(f: Flags64): i32;`
- `export fn formatHex(f: Flags64, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`bitflags@1.0.0`)，许可证：MIT
- **上游生态**：`npm` (`bitfield@1.0.0`)，许可证：BSD-2-Clause
