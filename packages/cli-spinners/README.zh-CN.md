[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# cli-spinners

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation terminal spinner frame presets and Unicode/ASCII status symbols (cli-spinners / ora / log-symbols parity) for CP

## 安装与导入

```bash
cap add cli-spinners
```

```cp
import { intervalMs } from "cli-spinners";
```

## 核心导出 API

- `export enum SpinnerKind`
- `export enum SymbolKind`
- `export fn intervalMs(kind: SpinnerKind): u32;`
- `export fn frameCount(kind: SpinnerKind): usize;`
- `export fn frameAt(kind: SpinnerKind, tick: usize): string;`
- `export fn frameAtTime(kind: SpinnerKind, elapsed_ms: u64): string;`
- `export fn findSpinnerByName(name: string): SpinnerKind?;`
- `export fn logSymbol(kind: SymbolKind, ascii_only: bool): string;`
- `export fn renderSpinnerLine(kind: SpinnerKind, tick: usize, text: string, out: []u8): string;`
- `export fn renderStatusLine(sym: SymbolKind, ascii_only: bool, text: string, out: []u8): string;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`cli-spinners@1.0.0`)，许可证：MIT
