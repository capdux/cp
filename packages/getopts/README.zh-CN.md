[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# getopts

> 版本：`1.0.0` | 许可证：`MIT OR Apache-2.0` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation POSIX/GNU command-line option parser (short flags, clustered flags, --opt=val, -- separator, required options) in pure CP

## 安装与导入

```bash
cap add getopts
```

```cp
import { goptStrEq } from "getopts";
```

## 核心导出 API

- `export enum HasArg`
- `export enum Occur`
- `export fn goptStrEq(a: string, b: string): bool;`
- `export fn goptFindEq(s: string): usize?;`
- `export fn goptFindByName(defs: &[]OptDef, count: usize, name: string): usize?;`
- `export fn goptFindShort(defs: &[]OptDef, count: usize, ch: string): usize?;`
- `export fn goptFindLong(defs: &[]OptDef, count: usize, name: string): usize?;`
- `export fn goptCountForIdx(vals: &[]OptVal, val_count: usize, idx: usize): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`getopts@1.0.0`)，许可证：MIT OR Apache-2.0
