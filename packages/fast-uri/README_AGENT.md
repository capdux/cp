# fast-uri@1.0.0 (Agent Quickref)

> Zero-allocation RFC 3986 URI parser, serializer, relative resolver, and equivalence comparator (clean-room implementation of npm fast-uri, #171 / 229M wk) | License: BSD-3-Clause
> Import: `import { ... } from "fast-uri";` | Full types: `src/index.d.cp`

## Functions
- `export fn furiIsSchemeChar(b: u8): bool;`
- `export fn furiPush(out: []u8, pos: usize, b: u8): usize;`
- `export fn furiPushSlice(out: []u8, pos: usize, s: string): usize;`
- `export fn furiPushLower(out: []u8, pos: usize, s: string): usize;`
- `export fn furiPushPercentUpper(out: []u8, pos: usize, s: string): usize;`
- `export fn furiPushPort(out: []u8, pos: usize, port: i32): usize;`
- `export fn furiDefaultPort(scheme: string): i32;`
- `export fn furiHasColon(s: string): bool;`
- `export fn furiValidateIpv6Inner(s: string): bool;`
- `export fn furiParsePortStr(s: string): i32;`
- `export fn parseUri(input: string): UriComponents;`
- `export fn serializeUri(c: UriComponents, out: []u8): usize;`
- `export fn furiPopSegment(out: []u8, out_len: usize): usize;`
- `export fn furiRemoveDotSegments(input: string, out: []u8): usize;`
- `export fn resolveUri(base: string, rel: string, out: []u8): usize;`
- `export fn furiNormalizeForCompare(input: string, out: []u8): usize;`
- `export fn equalUri(a: string, b: string): bool;`
