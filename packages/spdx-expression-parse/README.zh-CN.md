[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# spdx-expression-parse

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation SPDX license expression recursive-descent AST parser and evaluator for CP.

## 安装与导入

```bash
cap add spdx-expression-parse
```

```cp
import { spdxpEmptyNode } from "spdx-expression-parse";
```

## 核心导出 API

- `export fn spdxpEmptyNode(): SpdxAstNode;`
- `export fn spdxpEmptyAst(): SpdxAst;`
- `export fn spdxpIsIdChar(ch: u8): bool;`
- `export fn spdxpIsKeyword(tok: string): bool;`
- `export fn spdxpIsAllIdChars(s: string): bool;`
- `export fn spdxpIsValidLicenseRef(id: string): bool;`
- `export fn spdxpIsValidLicenseId(id: string): bool;`
- `export fn spdxpIsValidExceptionId(exc: string): bool;`
- `export fn spdxpSkipSpaces(st: &mut SpdxpState);`
- `export fn spdxpAllocNode(st: &mut SpdxpState, node: SpdxAstNode): i32;`
- `export fn spdxpConsumeKeyword(st: &mut SpdxpState, kw: string): bool;`
- `export fn spdxpParseSimple(st: &mut SpdxpState): i32;`
- `export fn spdxpParsePrimary(st: &mut SpdxpState): i32;`
- `export fn spdxpParseAnd(st: &mut SpdxpState): i32;`
- `export fn spdxpParseOr(st: &mut SpdxpState): i32;`
- `export fn parseSpdxExpression(expr: string): SpdxAst;`
- `export fn isValidSpdxExpression(expr: string): bool;`
- `export fn spdxpMatchLicenseNode(node: SpdxAstNode, allowed: string): bool;`
- `export fn spdxpEvalAst(ast: SpdxAst, idx: i32, allowed: string, depth: usize): bool;`
- `export fn satisfiesSpdx(ast: SpdxAst, allowed_license: string): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`spdx-expression-parse@1.0.0`)，许可证：MIT
