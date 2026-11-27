# css-what@1.0.0 (Agent Quickref)

> Zero-allocation CSS selector tokenizer, parser, and serializer (Cheerio / css-select core) in pure CP | License: BSD-2-Clause
> Import: `import { ... } from "css-what";` | Full types: `src/index.d.cp`

## Types & Constants
- `export enum CssTokenKind`
- `export enum AttrAction`

## Functions
- `export fn cwhIsIdentStart(c: u8): bool;`
- `export fn cwhIsIdentChar(c: u8): bool;`
- `export fn cwhSkipSpace(s: string, start: usize): usize;`
- `export fn cwhScanIdent(s: string, start: usize): usize;`
- `export fn cwhIsCombinatorChar(c: u8): bool;`
- `export fn cwhToken(kind: CssTokenKind, name: string, value: string, action: AttrAction, ignore_case: bool): CssSelectorToken;`
- `export fn parseCssSelector(selector: string, out: []CssSelectorToken): i64;`
- `export fn stringifyCssSelector(tokens: &[]CssSelectorToken, out: []u8): usize;`
