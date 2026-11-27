# cssesc@1.0.0 (Agent Quickref)

> Zero-allocation CSSOM-compliant CSS identifier and string escaper in pure CP | License: MIT
> Import: `import { ... } from "cssesc";` | Full types: `src/index.d.cp`

## Types & Constants
- `export enum CssQuoteStyle`

## Functions
- `export fn defaultCssEscOptions(): CssEscOptions;`
- `export fn cescDecodeUtf8(s: string, start: usize): CescUtf8Res;`
- `export fn cescIsAsciiAlnumOrDash(cp: u32): bool;`
- `export fn cssescBuf(input: string, opts: CssEscOptions, out: []u8): usize;`
- `export fn cssesc(input: string, opts: CssEscOptions): string;`
