[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# acorn

Pure CP, zero-allocation **ES2024 JavaScript lexer** and **ES Module (`import` / `export`) static analyzer** with full line/column tracking and bracket-balance validation.

## Provenance & Zero-Cost Migration

| Field | Value |
| :--- | :--- |
| **Source Registry** | [npm](https://www.npmjs.com/package/acorn) |
| **Upstream Package** | `acorn` (v8 parity) |
| **Upstream Repo** | https://github.com/acornjs/acorn |
| **CP Package** | `acorn` (`1.0.0`) |
| **License** | MIT |

Keep the exact same package name in CP — no renaming required:

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

## Installation

```bash
cap add acorn
```

## Key Features & Performance Highlights

- **Zero Heap Allocation**: `JsLexer` scans UTF-8 JavaScript source in a single pass returning lightweight `JsToken { kind, start, end, line, col }` slices pointing directly into the input string.
- **Full ES2024 Lexical Coverage**: Handles single/double-quoted strings with escapes, template literals, hex/binary/octal/BigInt/underscore numeric literals, line & block comments, ES2024 keywords, and 1–4 character operators (`===`, `!==`, `>>>=`, `??=`, `?.`, `...`, `=>`).
- **Fast ESM Static Analysis**: `analyzeJsModule(src)` counts tokens, `import`/`export`/`function`/`class` declarations, and validates balanced `()`, `[]`, `{}` brackets in one pass.
- **Zero-Alloc Import Specifier Extraction**: `extractImportSpecifiers(src, out_specs)` collects module specifiers from static `import ... from "pkg"` and side-effect `import "pkg"` statements directly into a caller slice.

## Quick Start

### Analyze an ES Module & Extract Imports

```typescript
import { analyzeJsModule, extractImportSpecifiers } from "acorn";

fn main(): void {
    const src =
        "import { readFile } from 'fs';\n" +
        "import 'dotenv/config';\n" +
        "export class Server { start() { return 42; } }\n";

    const summary = analyzeJsModule(src);
    if (!summary.ok) {
        // Unmatched bracket at byte offset summary.unmatched_bracket_pos
        return;
    }

    let specs: [16]string = undefined;
    const count = extractImportSpecifiers(src, specs[0..]);
    // specs[0] == "fs", specs[1] == "dotenv/config", count == 2
    _ = count;
}
```

### Streaming Tokenization with `JsLexer`

```typescript
import { JsLexer, TOK_EOF } from "acorn";

fn countTokens(src: string): usize {
    let lexer = JsLexer.new(src);
    let total: usize = 0;
    while (true) {
        const tok = lexer.nextToken();
        if (tok.kind == TOK_EOF) {
            break;
        }
        const lexeme = src[tok.start..tok.end];
        _ = lexeme;
        total += 1;
    }
    return total;
}
```

## API Overview

| Symbol | Signature / Value | Description |
| :--- | :--- | :--- |
| `TOK_EOF` .. `TOK_COMMENT` | `0` .. `7` | Token kind constants (`EOF=0`, `IDENT=1`, `KEYWORD=2`, `NUMBER=3`, `STRING=4`, `TEMPLATE=5`, `PUNCT=6`, `COMMENT=7`) |
| `JsToken` | `{ kind, start, end, line, col }` | Zero-copy token span with 1-based line & column |
| `JsLexer.new` | `(src: string) -> JsLexer` | Create streaming ES2024 lexer |
| `JsLexer.nextToken` | `() -> JsToken` | Advance and return next token (`TOK_EOF` at end) |
| `JsModuleSummary` | `{ ok, token_count, import_count, export_count, function_count, class_count, unmatched_bracket_pos }` | Static module analysis result |
| `analyzeJsModule` | `(src: string) -> JsModuleSummary` | Single-pass ESM summary and bracket-balance check |
| `extractImportSpecifiers` | `(src: string, out_specs: []string) -> usize` | Extract static `import` specifier strings into `out_specs` |

## License

MIT
