[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# es-module-lexer

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:es-module-lexer` ([https://github.com/guybedford/es-module-lexer](https://github.com/guybedford/es-module-lexer))

Zero-allocation ES module static/dynamic import and export lexer for CP — clean-room MIT parity with npm `es-module-lexer` (308M+/wk).

纯 CP 零堆分配 ES Module 词法分析器，精准跳过注释与字符串字面量，提取静态 `import`、副作用 `import`、动态 `import()`、`import.meta`、`export const/let/var/function/class/async function`、`export default`、`export { a as b } from` 及 `export * from`，并支持 `isFacade` 重导出门面模块判定。

## Install / 安装

```bash
cap add es-module-lexer
```

## API

| Symbol | Signature | Description |
| --- | --- | --- |
| `ImportKind` | `enum` | `Static`, `Dynamic`, `ImportMeta` |
| `ImportSpecifier` | `struct` | Import span (`kind`, `spec_start`, `spec_end`, `stmt_start`, `stmt_end`, `.specifier(src)`) |
| `ExportSpecifier` | `struct` | Export span (`name_start`, `name_end`, `local_start`, `local_end`, `.name(src)`, `.localName(src)`) |
| `EsmParseResult` | `struct` | Parse summary (`import_count`, `export_count`, `facade`, `has_module_syntax`) |
| `parseEsm` | `(src: string, imports_out: []ImportSpecifier, exports_out: []ExportSpecifier): EsmParseResult` | Lex ESM imports and exports into caller buffers |
| `isFacade` | `(src: string): bool` | Returns `true` if module contains only import/re-export statements |

## Usage / 使用示例

```cp
import { parseEsm, isFacade, ImportSpecifier, ExportSpecifier } from "es-module-lexer";

const code = "import { a } from './a.js'; export { b as c } from './b.js';";
let imps: [8]ImportSpecifier = undefined;
let exps: [8]ExportSpecifier = undefined;
const res = parseEsm(code, imps[0..8], exps[0..8]); // import_count=2, export_count=1, facade=true
```
