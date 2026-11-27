# bcp-47@1.0.0 (Agent Quickref)

> Zero-allocation RFC 5646 / BCP-47 language tag parser, canonicalizer, and fallback locale matcher for CP | License: MIT
> Import: `import { ... } from "bcp-47";` | Full types: `src/index.d.cp`

## Functions
- `export fn isWs(c: u8): bool;`
- `export fn isAlpha(c: u8): bool;`
- `export fn isDigit(c: u8): bool;`
- `export fn isAlphaNum(c: u8): bool;`
- `export fn toLowerAscii(c: u8): u8;`
- `export fn toUpperAscii(c: u8): u8;`
- `export fn trimWs(s: string): string;`
- `export fn eqIgnoreCase(a: string, b: string): bool;`
- `export fn eqSubtagNormalized(a: string, b: string): bool;`
- `export fn isAllAlpha(s: string): bool;`
- `export fn isAllDigit(s: string): bool;`
- `export fn isAllAlphaNum(s: string): bool;`
- `export fn isVariantSubtag(s: string): bool;`
- `export fn isSingletonSubtag(s: string): bool;`
- `export fn isPrivateIntro(s: string): bool;`
- `export fn parseBcp47(raw: string): Bcp47Tag;`
- `export fn appendByte(out: []u8, pos: usize, c: u8): usize;`
- `export fn appendLowerHyphen(out: []u8, start_pos: usize, s: string): usize;`
- `export fn appendUpper(out: []u8, start_pos: usize, s: string): usize;`
- `export fn appendTitle(out: []u8, start_pos: usize, s: string): usize;`
- `export fn canonicalizeBcp47(tag: Bcp47Tag, out: []u8): string;`
- `export fn scoreLocaleMatch(req: Bcp47Tag, cand: Bcp47Tag): u32;`
- `export fn matchLocale(requested: Bcp47Tag, supported: &[]string): string?;`
