[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# ansi-align

> 版本：`1.0.0` | 许可证：`ISC` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation ANSI-aware multiline text alignment (npm ansi-align parity) composing @cap/string-width and @cap/widest-line for CP

## 安装与导入

```bash
cap add ansi-align
```

```cp
import { aaWriteSpaces } from "ansi-align";
```

## 核心导出 API

- `export enum AlignMode`
- `export fn aaWriteSpaces(out: []u8, pos: usize, count: usize): usize;`
- `export fn aaCopyRange(out: []u8, pos: usize, s: string, start: usize, end: usize): usize;`
- `export fn alignAnsi(text: string, mode: AlignMode, out: []u8): usize;`
- `export fn alignCenter(text: string, out: []u8): usize;`
- `export fn alignRight(text: string, out: []u8): usize;`
- `export fn alignLeft(text: string, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`ansi-align@1.0.0`)，许可证：ISC
