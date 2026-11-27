[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# argparse

> 版本：`1.0.0` | 许可证：`Python-2.0` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation Python-style CLI argument parser supporting short/long flags, counts, bundling, and required checks for CP

## 安装与导入

```bash
cap add argparse
```

```cp
import { argpStrEq } from "argparse";
```

## 核心导出 API

- `export enum ArgAction`
- `export fn argpStrEq(a: string, b: string): bool;`
- `export fn argpStripDashes(s: string): string;`
- `export fn argpFindEq(s: string): usize?;`
- `export fn argpResolveDest(def: ArgDef): string;`
- `export fn argpFindLong(defs: &[]ArgDef, name: string): usize?;`
- `export fn argpFindShort(defs: &[]ArgDef, ch: u8): usize?;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`argparse@1.0.0`)，许可证：Python-2.0
