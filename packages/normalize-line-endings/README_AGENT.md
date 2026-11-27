# normalize-line-endings@0.3.0 (Agent Quickref)

> Zero-heap line ending detector and normalizer (CRLF/CR <-> LF) | License: Apache-2.0
> Import: `import { ... } from "normalize-line-endings";` | Full types: `src/index.d.cp`

## Types & Constants
- `export enum LineEnding`

## Functions
- `export fn detectLineEnding(input: string): LineEnding;`
- `export fn normalizeToLf(input: string, out: []u8): usize;`
- `export fn normalizeToCrlf(input: string, out: []u8): usize;`
