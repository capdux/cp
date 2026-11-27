[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# ron

> 版本：`0.8.1` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap Rusty Object Notation (RON) lexer and top-level field extractor in pure CP

## 安装与导入

```bash
cap add ron
```

```cp
import { ronpIsWs } from "ron";
```

## 核心导出 API

- `export enum RonTokenKind`
- `export fn ronpIsWs(c: u8): bool;`
- `export fn ronpIsDigit(c: u8): bool;`
- `export fn ronpIsHexDigit(c: u8): bool;`
- `export fn ronpIsIdentStart(c: u8): bool;`
- `export fn ronpIsIdentCont(c: u8): bool;`
- `export fn ronpEq(a: string, b: string): bool;`
- `export fn ronpSkipTrivia(src: string, start_pos: usize): usize;`
- `export fn ronpIsOpenDelim(k: RonTokenKind): bool;`
- `export fn ronpIsCloseDelim(k: RonTokenKind): bool;`
- `export fn ronGetField(ron_src: string, field_name: string): string?;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`ron@0.8.1`)（https://github.com/ron-rs/ron），许可证：MIT
