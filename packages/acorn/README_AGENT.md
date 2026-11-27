# acorn — AI Agent Quick Ref

- **Package**: `acorn` (`1.0.0`) | **Source**: npm `acorn` v8 (zero-alloc ES2024 lexer + ESM scanner)
- **Import**: `import { JsLexer, JsToken, JsModuleSummary, analyzeJsModule, extractImportSpecifiers, TOK_EOF, TOK_IDENT, TOK_KEYWORD, TOK_NUMBER, TOK_STRING, TOK_TEMPLATE, TOK_PUNCT, TOK_COMMENT } from "acorn";`
- **Allocation**: Zero-heap; tokens store byte offsets `[start..end]` into original `src`.

## Constants & Types

```typescript
TOK_EOF=0; TOK_IDENT=1; TOK_KEYWORD=2; TOK_NUMBER=3;
TOK_STRING=4; TOK_TEMPLATE=5; TOK_PUNCT=6; TOK_COMMENT=7;

export class JsToken { kind: u8, start: usize, end: usize, line: usize, col: usize }
export class JsModuleSummary {
    ok: bool, token_count: usize, import_count: usize, export_count: usize,
    function_count: usize, class_count: usize, unmatched_bracket_pos: usize,
}
```

## Core Signatures

```typescript
export class JsLexer {
    src: string, pos: usize, line: usize, col: usize,
    fn new(src: string): JsLexer;
    fn advance(_: auto);
    fn skipWhitespace(_: auto);
    fn nextToken(_: auto): JsToken;
}

export fn analyzeJsModule(src: string): JsModuleSummary;
export fn extractImportSpecifiers(src: string, out_specs: []string): usize;
export fn acnIsKeyword(s: string): bool;
```

## Minimal Pattern

```typescript
import { analyzeJsModule, extractImportSpecifiers } from "acorn";

const summary = analyzeJsModule(src);
let specs: [32]string = undefined;
const n = extractImportSpecifiers(src, specs[0..]);
```

## Pitfalls

1. **Lexer + ESM scanner (not full ESTree AST)**: Designed for ultra-fast tokenization, bracket checking, and dependency extraction — does not allocate full AST nodes.
2. **Strip quotes manually if using `JsLexer`**: `TOK_STRING` spans include surrounding quotes in `src[tok.start..tok.end]`, whereas `extractImportSpecifiers` already strips quotes.
3. **Output slice capacity**: `extractImportSpecifiers` stops once `out_specs.len` is reached and returns the count written.
4. **Comments are emitted as tokens**: Check `tok.kind == TOK_COMMENT` if skipping comments in a manual `JsLexer` loop.
