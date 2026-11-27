[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# pep440_rs

> 版本：`1.0.0` | 许可证：`MIT OR Apache-2.0` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap PEP 440 Python version parser, total-order comparator, and specifier matcher in pure CP (modeled after Astral pep440_rs)

## 安装与导入

```bash
cap add pep440_rs
```

```cp
import { pepInvalid } from "pep440_rs";
```

## 核心导出 API

- `export enum PreKind`
- `export fn pepInvalid(): Pep440Version;`
- `export fn pepIsDigit(c: u8): bool;`
- `export fn pepIsSpace(c: u8): bool;`
- `export fn pepToLower(c: u8): u8;`
- `export fn pepIsSep(c: u8): bool;`
- `export fn pepMatchWord(s: string, pos: usize, end: usize, word: string): bool;`
- `export fn pepReadU32(s: string, pos: usize, end: usize): PepNumRes;`
- `export fn parsePep440(s: string): Pep440Version;`
- `export fn pepCmpU32(a: u32, b: u32): i32;`
- `export fn pepPreRank(k: PreKind): i32;`
- `export fn pepStageRank(v: Pep440Version): i32;`
- `export fn comparePep440(a: Pep440Version, b: Pep440Version): i32;`
- `export fn pepPrefixMatch(v: Pep440Version, target: Pep440Version, prefix_len: u8): bool;`
- `export fn pepMatchSingle(v: Pep440Version, raw: string): bool;`
- `export fn matchesSpecifier(ver: string, spec: string): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`pep440_rs@1.0.0`)，许可证：MIT OR Apache-2.0
