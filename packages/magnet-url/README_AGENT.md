# magnet-url@1.0.0 (Agent Quickref)

> Zero-heap BitTorrent Magnet URI (BEP-9 / BEP-53 btih & btmh) parser, tracker extractor, and builder for CP (compatible with npm magnet-uri & Rust magnet-url) | License: MIT
> Import: `import { ... } from "magnet-url";` | Full types: `src/index.d.cp`

## Functions
- `export fn startsWithIgnoreCase(s: string, prefix: string): bool;`
- `export fn isHexStr(s: string): bool;`
- `export fn isBase32Str(s: string): bool;`
- `export fn parsePositiveI64(s: string): i64;`
- `export fn classifyExactTopic(xt: string): TopicMatch;`
- `export fn parseMagnet(uri: string): MagnetLink;`
- `export fn appendBytes(dst: []u8, pos: usize, src: string): usize;`
- `export fn formatMagnetV1(info_hash: string, display_name: string, trackers: &[]string, out: []u8): usize;`
