# iri-string@1.0.0 (Agent Quickref)

> Zero-allocation RFC 3986 / RFC 3987 URI/IRI parser, dot-segment remover, and relative reference resolver for CP | License: MIT
> Import: `import { ... } from "iri-string";` | Full types: `src/index.d.cp`

## Functions
- `export fn invalidIri(): IriParts;`
- `export fn isAsciiAlpha(c: u8): bool;`
- `export fn isSchemeChar(c: u8): bool;`
- `export fn parseIri(s: string): IriParts;`
- `export fn remLen(path: string, cur: RemCursor): usize;`
- `export fn remAt(path: string, cur: RemCursor, idx: usize): u8;`
- `export fn remAdvance(cur: &mut RemCursor, n: usize);`
- `export fn popLastSegment(out: []u8, out_len: usize): usize;`
- `export fn removeDotSegments(path: string, out: []u8): usize;`
- `export fn obPush(ob: &mut OutBuf, out: []u8, ch: u8);`
- `export fn obWrite(ob: &mut OutBuf, out: []u8, src: string);`
- `export fn resolveReference(base: string, rel: string, out: []u8): usize;`
