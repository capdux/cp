[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# wrap-ansi

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation ANSI-preserving word/hard wrapper and visible-column slicer with CJK double-width awareness (wrap-ansi / slice-ansi parity) for CP

## 安装与导入

```bash
cap add wrap-ansi
```

```cp
import { decodeUtf8At } from "wrap-ansi";
```

## 核心导出 API

- `export fn decodeUtf8At(s: string, i: usize): Utf8Step;`
- `export fn codepointWidth(cp: u32): usize;`
- `export fn scanAnsiCsi(s: string, i: usize): usize;`
- `export fn isResetSgr(seq: string): bool;`
- `export fn startsWithZeroReset(seq: string): bool;`
- `export fn pushByte(out: []u8, pos: usize, b: u8): usize;`
- `export fn pushStr(out: []u8, pos: usize, s: string): usize;`
- `export fn visibleWidth(s: string): usize;`
- `export fn emitLineBreak(out: []u8, pos: usize, style: AnsiStyleState): usize;`
- `export fn wrapAnsi(text: string, max_cols: usize, hard_wrap: bool, out: []u8): string;`
- `export fn sliceAnsi(text: string, start_col: usize, end_col: usize, out: []u8): string;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`wrap-ansi@1.0.0`)，许可证：MIT
