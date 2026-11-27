# @atproto/syntax@1.0.0 (Agent Quickref)

> Zero-allocation AT Protocol / Bluesky at:// URI, NSID, and Record Key (rkey) validator, parser, and formatter in pure CP | License: MIT
> Import: `import { ... } from "@atproto/syntax";` | Full types: `src/index.d.cp`

## Functions
- `export fn invalidAtUri(): AtUri;`
- `export fn isLowerAlpha(b: u8): bool;`
- `export fn isAlpha(b: u8): bool;`
- `export fn isDigit(b: u8): bool;`
- `export fn isAlphaNum(b: u8): bool;`
- `export fn isValidNsid(nsid: string): bool;`
- `export fn isValidRkey(rkey: string): bool;`
- `export fn isValidDid(s: string): bool;`
- `export fn isValidHandle(s: string): bool;`
- `export fn parseAtUri(uri: string): AtUri;`
- `export fn pushSlice(dst: []u8, pos: usize, src: string): i64;`
- `export fn formatAtUri(authority: string, collection: string, rkey: string, out: []u8): usize;`
