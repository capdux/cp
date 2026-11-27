[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# esutils

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation ECMAScript keyword, reserved word, identifier, whitespace, and line terminator utilities (clean-room MIT implementation of npm esutils, #201 / 215M wk)

## 安装与导入

```bash
cap add esutils
```

```cp
import { isDecimalDigit } from "esutils";
```

## 核心导出 API

- `export fn isDecimalDigit(cp: u32): bool;`
- `export fn isHexDigit(cp: u32): bool;`
- `export fn isOctalDigit(cp: u32): bool;`
- `export fn isWhiteSpace(cp: u32): bool;`
- `export fn isLineTerminator(cp: u32): bool;`
- `export fn isIdentifierStartES6(cp: u32): bool;`
- `export fn isIdentifierPartES6(cp: u32): bool;`
- `export fn isKeywordES6(id: string, strict: bool): bool;`
- `export fn isReservedWordES6(id: string, strict: bool): bool;`
- `export fn isRestrictedWord(id: string): bool;`
- `export fn isIdentifierNameES6(id: string): bool;`
- `export fn isIdentifierES6(id: string, strict: bool): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`esutils@1.0.0`)，许可证：BSD-2-Clause
