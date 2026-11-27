# levn@0.4.1 (Agent Quickref)

> Zero-heap Light Value Notation (levn) scalar, list, and object parser | License: MIT
> Import: `import { ... } from "levn";` | Full types: `src/index.d.cp`

## Types & Constants
- `export enum LevnKind`

## Functions
- `export fn lvnMakeInvalid(): LevnValue;`
- `export fn lvnTryParseNumber(s: string): LevnValue;`
- `export fn parseLevnScalar(input: string): LevnValue;`
- `export fn lvnFindTopLevelChar(s: string, start: usize, target: u8): usize;`
- `export fn lvnUnquoteKey(s: string): string;`
- `export fn parseLevnList(input: string, out: []LevnValue): usize;`
- `export fn parseLevnObject(input: string, out: []LevnPair): usize;`
