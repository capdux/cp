# url-join@1.0.0 (Agent Quickref)

> Zero-allocation smart URL joiner, slash/dot-segment path normalizer, query merger, and trailing-slash utilities (url-join / ufo parity) for CP | License: MIT
> Import: `import { ... } from "url-join";` | Full types: `src/index.d.cp`

## Functions
- `export fn pushByte(out: []u8, pos: usize, b: u8): usize;`
- `export fn pushStr(out: []u8, pos: usize, s: string): usize;`
- `export fn strEq(a: string, b: string): bool;`
- `export fn isAlpha(c: u8): bool;`
- `export fn isSchemeChar(c: u8): bool;`
- `export fn schemePrefixLen(s: string): usize;`
- `export fn splitPart(p: string): PartSplit;`
- `export fn trimLeadingSlashes(s: string): string;`
- `export fn trimTrailingSlashes(s: string): string;`
- `export fn urlJoin(parts: []string, out: []u8): string;`
- `export fn normalizeUrlPath(path: string, out: []u8): string;`
- `export fn withQuery(base_url: string, key: string, value: string, out: []u8): string;`
- `export fn findQueryOrHash(url: string): usize;`
- `export fn withTrailingSlash(url: string, out: []u8): string;`
- `export fn withoutTrailingSlash(url: string, out: []u8): string;`
