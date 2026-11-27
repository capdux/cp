[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# es-module-lexer

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation ES module static/dynamic import and export lexer for CP

## 安装与导入

```bash
cap add es-module-lexer
```

```cp
import { esmlIsWs } from "es-module-lexer";
```

## 核心导出 API

- `export enum ImportKind`
- `export fn esmlIsWs(c: u8): bool;`
- `export fn esmlIsIdentStart(c: u8): bool;`
- `export fn esmlIsIdentPart(c: u8): bool;`
- `export fn esmlStrEq(a: string, b: string): bool;`
- `export fn esmlSkipWsAndComments(src: string, start: usize): usize;`
- `export fn esmlScanString(src: string, start: usize): EsmlSpan;`
- `export fn esmlSkipTemplate(src: string, start: usize): usize;`
- `export fn esmlScanIdent(src: string, start: usize): EsmlSpan;`
- `export fn esmlPrevNonWsChar(src: string, pos: usize): u8;`
- `export fn esmlPushImport(out: []ImportSpecifier, count: usize, kind: ImportKind, spec_start: usize, spec_end: usize, stmt_start: usize, stmt_end: usize): usize;`
- `export fn esmlPushExport(out: []ExportSpecifier, count: usize, name_start: usize, name_end: usize, local_start: usize, local_end: usize): usize;`
- `export fn parseEsm(src: string, imports_out: []ImportSpecifier, exports_out: []ExportSpecifier): EsmParseResult;`
- `export fn isFacade(src: string): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`es-module-lexer@1.0.0`)，许可证：MIT
