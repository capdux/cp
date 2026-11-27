[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# calver

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation Calendar Versioning (CalVer) parser, validator, comparator, bumper, and formatter for CP

## 安装与导入

```bash
cap add calver
```

```cp
import { cmpIgnoreCase } from "calver";
```

## 核心导出 API

- `export enum CalverFormat`
- `export fn cmpIgnoreCase(a: string, b: string): i32;`
- `export fn isLeapYear(year: u16): bool;`
- `export fn daysInMonth(year: u16, month: u8): u8;`
- `export fn parseU32Strict(s: string): u32?;`
- `export fn parseModifierTail(s: string): ModifierParts;`
- `export fn parseCalverCore(raw: string, force_day_on_3: bool): Calver;`
- `export fn parseCalver(raw: string): Calver;`
- `export fn parseCalverWith(raw: string, fmt: CalverFormat): Calver;`
- `export fn modifierRank(name: string): u8;`
- `export fn compareCalver(a: Calver, b: Calver): i32;`
- `export fn bumpCalver(cur: Calver, year: u16, month: u8, day: u8, use_day: bool): Calver;`
- `export fn appendByte(out: []u8, pos: usize, c: u8): usize;`
- `export fn append2Digits(out: []u8, pos: usize, val: u32): usize;`
- `export fn formatCalver(v: Calver, fmt: CalverFormat, out: []u8): string;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`calver@1.0.0`)，许可证：MIT
