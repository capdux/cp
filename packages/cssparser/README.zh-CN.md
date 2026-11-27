[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# cssparser

> 版本：`0.34.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap CSS Syntax Level 3 tokenizer and lexer in pure CP

## 安装与导入

```bash
cap add cssparser
```

```cp
import { csspIsWs } from "cssparser";
```

## 核心导出 API

- `export enum CssTokKind`
- `export fn csspIsWs(c: u8): bool;`
- `export fn csspIsDigit(c: u8): bool;`
- `export fn csspIsIdentStart(c: u8): bool;`
- `export fn csspIsIdentChar(c: u8): bool;`
- `export fn csspStartsIdent(s: string, pos: usize): bool;`
- `export fn csspStartsNumber(s: string, pos: usize): bool;`
- `export fn csspScanIdent(s: string, start: usize): usize;`
- `export fn csspScanNumber(s: string, start: usize): usize;`
- `export fn csspSkipWsAndComments(s: string, start: usize): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`cssparser@0.34.0`)（https://github.com/servo/rust-cssparser），许可证：MIT
