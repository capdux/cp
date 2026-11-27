[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# normpath

> 版本：`1.0.0` | 许可证：`MIT OR Apache-2.0` | 类型声明：[src/index.d.cp](./src/index.d.cp)

CP 官方生态包 normpath

## 安装与导入

```bash
cap add normpath
```

```cp
import { npIsAsciiAlpha } from "normpath";
```

## 核心导出 API

- `export fn npIsAsciiAlpha(b: u8): bool;`
- `export fn npToUpper(b: u8): u8;`
- `export fn npIsWinSep(b: u8): bool;`
- `export fn npWriteByte(out: []u8, pos: usize, b: u8): usize;`
- `export fn npWriteSlice(out: []u8, pos: usize, src: string): usize;`
- `export fn npSegEqDot(src: string, start: usize, len: usize): bool;`
- `export fn npSegEqDotDot(src: string, start: usize, len: usize): bool;`
- `export fn npNormalizePosixTwo(p1: string, p2: string, hasP2: bool, out: []u8): usize;`
- `export fn normalizePosix(path: string, out: []u8): usize;`
- `export fn joinNormalized(base: string, rel: string, out: []u8): usize;`
- `export fn isVerbatimWindows(path: string): bool;`
- `export fn stripVerbatimPrefix(path: string): string;`
- `export fn npHasUncTag(path: string, off: usize): bool;`
- `export fn normalizeWindows(path: string, out: []u8): usize;`
- `export fn npWinIsAbsolute(path: string): bool;`
- `export fn joinNormalizedWindows(base: string, rel: string, out: []u8): usize;`
- `export fn hasParentTraversal(path: string): bool;`
- `export fn escapesRoot(path: string): bool;`
- `export fn isNormalized(path: string): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`normpath@1.0.0`)，许可证：MIT OR Apache-2.0
