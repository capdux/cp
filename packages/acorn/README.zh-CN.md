[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent 快速参考](README_AGENT.md)

# acorn

纯 CP 实现的零堆分配 **ES2024 JavaScript 词法分析器**与 **ES Module（`import` / `export`）静态扫描器**，支持精准行列号追踪与括号平衡校验。

## 源包溯源与零成本迁移

| 字段 | 说明 |
| :--- | :--- |
| **源平台** | [npm](https://www.npmjs.com/package/acorn) |
| **原包名与版本** | `acorn`（v8 对齐） |
| **上游仓库** | https://github.com/acornjs/acorn |
| **CP 包名与版本** | `acorn`（`1.0.0`） |
| **开源协议** | MIT |

在 CP 中保持原包名直接导入，无需改名：

```typescript
// TypeScript (npm)
import * as acorn from "acorn";

// CP (cap)
import {
    JsLexer,
    JsToken,
    JsModuleSummary,
    analyzeJsModule,
    extractImportSpecifiers,
    TOK_EOF,
    TOK_IDENT,
    TOK_KEYWORD,
    TOK_NUMBER,
    TOK_STRING,
    TOK_TEMPLATE,
    TOK_PUNCT,
    TOK_COMMENT,
} from "acorn";
```

## 安装

```bash
cap add acorn
```

## 核心特性与性能亮点

- **零堆分配**：`JsLexer` 单遍扫描 JS 源码，输出仅含偏移量与行列号的轻量 `JsToken`，切片零拷贝直指原字符串。
- **完整 ES2024 词法支持**：覆盖单双引号字符串、模板字符串、十六进制/二进制/八进制/BigInt 数字字面量、行注释与块注释、ES2024 关键字及多字符运算符（`===`、`>>>=`、`??=`、`?.`、`...`、`=>`）。
- **单遍 ESM 静态分析**：`analyzeJsModule(src)` 一次遍历统计 Token 数、`import`/`export`/`function`/`class` 声明数并校验 `()`/`[]`/`{}` 括号匹配。
- **零分配依赖提取**：`extractImportSpecifiers(src, out_specs)` 直接提取 `import ... from "pkg"` 与副作用 `import "pkg"` 依赖路径。

## 快速上手

```typescript
import { analyzeJsModule, extractImportSpecifiers, JsLexer, TOK_EOF } from "acorn";

fn main(): void {
    const src =
        "import { readFile } from 'fs';\n" +
        "import 'dotenv/config';\n" +
        "export class Server { start() { return 42; } }\n";

    const summary = analyzeJsModule(src);
    if (!summary.ok) {
        return;
    }

    let specs: [16]string = undefined;
    const count = extractImportSpecifiers(src, specs[0..]); // count == 2
    _ = count;

    // 流式分词
    let lexer = JsLexer.new(src);
    while (true) {
        const tok = lexer.nextToken();
        if (tok.kind == TOK_EOF) {
            break;
        }
        const lexeme = src[tok.start..tok.end];
        _ = lexeme;
    }
}
```

## API 速览

| 导出符号 | 签名 / 值 | 说明 |
| :--- | :--- | :--- |
| `TOK_EOF` .. `TOK_COMMENT` | `0` .. `7` | Token 类型常量（`EOF=0`, `IDENT=1`, `KEYWORD=2`, `NUMBER=3`, `STRING=4`, `TEMPLATE=5`, `PUNCT=6`, `COMMENT=7`） |
| `JsToken` | `{ kind, start, end, line, col }` | 零拷贝 Token 区间与 1-based 行列号 |
| `JsLexer.new` / `nextToken` | `(src: string) -> JsLexer` | 流式 ES2024 词法分析器 |
| `analyzeJsModule` | `(src: string) -> JsModuleSummary` | 单遍统计 ESM 结构并校验括号平衡 |
| `extractImportSpecifiers` | `(src: string, out_specs: []string) -> usize` | 提取静态 `import` 模块路径到 `out_specs` |

## 开源协议

MIT
