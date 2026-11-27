# js-tokens@1.0.0 (Agent Quickref)

> Zero-allocation tiny JavaScript/TypeScript lexer tokenizer (clean-room MIT implementation of npm js-tokens, #61 / 381M wk) | License: MIT
> Import: `import { ... } from "js-tokens";` | Full types: `src/index.d.cp`

## Types & Constants
- `export enum JsTokenKind`

## Functions
- `export fn nextJsToken(src: string, pos: usize): JsToken?;`
- `export fn tokenizeJs(src: string, out: []JsToken): usize;`
- `export fn countJsTokens(src: string): usize;`
