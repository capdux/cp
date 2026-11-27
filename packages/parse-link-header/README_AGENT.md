# parse-link-header@1.0.0 (Agent Quickref)

> Zero-allocation RFC 8288 HTTP Link header parser and builder with GitHub/REST pagination helpers for CP | License: MIT
> Import: `import { ... } from "parse-link-header";` | Full types: `src/index.d.cp`

## Functions
- `export fn toLowerAscii(c: u8): u8;`
- `export fn isWs(c: u8): bool;`
- `export fn eqIgnoreCase(a: string, b: string): bool;`
- `export fn relMatches(rel_attr: string, target: string): bool;`
- `export fn skipToNextEntry(s: string, start: usize): usize;`
- `export fn parseLinkHeader(header: string): LinkHeader;`
- `export fn appendByte(out: []u8, pos: usize, c: u8): usize;`
- `export fn appendStr(out: []u8, pos: usize, s: string): usize;`
- `export fn appendUsize(out: []u8, pos: usize, val: usize): usize;`
- `export fn appendParam(out: []u8, pos: usize, name: string, val: string): usize;`
- `export fn formatLinkHeader(entries: []LinkEntry, out: []u8): string;`
- `export fn containsChar(s: string, target: u8): bool;`
- `export fn appendPageLink(out: []u8, start_pos: usize, base_url: string, sep: u8, target_page: usize, rel: string, emitted: usize): usize;`
- `export fn formatPaginationLinks(base_url: string, page: usize, total_pages: usize, out: []u8): string;`
