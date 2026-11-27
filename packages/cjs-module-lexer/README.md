[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/cjs-module-lexer

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:cjs-module-lexer` ([https://github.com/nodejs/cjs-module-lexer](https://github.com/nodejs/cjs-module-lexer))（原库名 `cjs-module-lexer` → CP 包名 `@cap/cjs-module-lexer`）

Zero-allocation CommonJS static named export and re-export lexer for CP — clean-room MIT implementation inspired by Node.js `cjs-module-lexer`.

纯 CP 零堆分配 CommonJS 静态导出与重导出词法扫描器（对标 Node.js 官方 `cjs-module-lexer`），跳过注释与字符串字面量，精准提取 `exports.foo`、`module.exports.bar`、`exports['baz']`、`Object.defineProperty(exports, 'qux', ...)`、`module.exports = { a, b: 1 }` 对象字面量导出，以及 `module.exports = require('./sub')` / `__exportStar(require('./sub'), exports)` 重导出并自动去重。

## Install / 安装

```bash
cap add @cap/cjs-module-lexer
```

## API

### Structs & Functions / 核心类型与函数

| Symbol | Signature / Fields | Description |
| --- | --- | --- |
| `CjsParseResult` | `struct { exports: [32]string, export_count: usize, reexports: [16]string, reexport_count: usize }` | Deduplicated static export names and re-export specifiers |
| `parseCjs` | `(src: string): CjsParseResult` | Lex CommonJS source and extract named exports & re-exports |

## Usage / 使用示例

```cp
import { parseCjs } from "@cap/cjs-module-lexer";

const src = "exports.alpha = 1;\nmodule.exports.beta = 2;\n__exportStar(require('./sub'), exports);";
const res = parseCjs(src);
// res.export_count == 2 ("alpha", "beta")
// res.reexport_count == 1 ("./sub")
```
