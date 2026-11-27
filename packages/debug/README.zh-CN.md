[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# debug

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation namespace debug log filter, glob exclusion engine, deterministic ANSI coloring, and diff-ms formatter for CP

## 安装与导入

```bash
cap add debug
```

```cp
import { isSep } from "debug";
```

## 核心导出 API

- `export fn isSep(c: u8): bool;`
- `export fn globMatch(pat: string, text: string): bool;`
- `export fn appendByte(out: []u8, pos: usize, c: u8): usize;`
- `export fn appendStr(out: []u8, pos: usize, s: string): usize;`
- `export fn appendU64(out: []u8, pos: usize, val: u64): usize;`
- `export fn isNamespaceEnabled(pattern: string, namespace: string): bool;`
- `export fn selectColor(namespace: string): u8;`
- `export fn formatDiffMs(diff_ms: i64, out: []u8): string;`
- `export fn formatDebugLine(ns: string, msg: string, diff_ms: i64, use_color: bool, out: []u8): string;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`debug@1.0.0`)，许可证：MIT
