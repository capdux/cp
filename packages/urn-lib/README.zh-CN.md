[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# urn-lib

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation RFC 8141 Uniform Resource Name (URN) parser, equivalence comparator, and canonical formatter in pure CP

## 安装与导入

```bash
cap add urn-lib
```

```cp
import { invalidUrn } from "urn-lib";
```

## 核心导出 API

- `export fn invalidUrn(): Urn;`
- `export fn urnIsSpace(b: u8): bool;`
- `export fn isValidNid(nid: string): bool;`
- `export fn parseUrn(input: string): Urn;`
- `export fn urnEquivalent(a: Urn, b: Urn): bool;`
- `export fn pushSlice(dst: []u8, pos: usize, src: string): i64;`
- `export fn pushLowerSlice(dst: []u8, pos: usize, src: string): i64;`
- `export fn formatUrn(u: Urn, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`urn-lib@1.0.0`)，许可证：MIT
- **上游生态**：`crates.io` (`urn@1.0.0`)，许可证：MIT
