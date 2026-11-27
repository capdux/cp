[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# figures

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation Unicode terminal symbols with automatic ASCII/Windows fallbacks composing @cap/is-unicode-supported in pure CP

## 安装与导入

```bash
cap add figures
```

```cp
import { figStartsAt } from "figures";
```

## 核心导出 API

- `export enum FigureKind`
- `export fn figStartsAt(input: string, pos: usize, needle: string): bool;`
- `export fn figWriteStr(out: []u8, pos: usize, s: string): usize;`
- `export fn figureFor(kind: FigureKind, unicode_ok: bool): string;`
- `export fn figureForEnv(kind: FigureKind, env: EnvInfo): string;`
- `export fn replaceSymbols(input: string, unicode_ok: bool, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`figures@1.0.0`)，许可证：MIT
