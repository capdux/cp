# punycode@1.0.0 (Agent Quickref)

> Zero-allocation RFC 3492 Punycode / IDNA (xn--...) encoder and decoder for internationalized domain names in CP. | License: MIT
> Import: `import { ... } from "punycode";` | Full types: `src/index.d.cp`

## Functions
- `export fn isAsciiOnly(s: string): bool;`
- `export fn encodeLabel(utf8_label: string, out: []u8): i64;`
- `export fn decodeLabel(ascii_label: string, out_utf8: []u8): i64;`
- `export fn toAsciiDomain(utf8_domain: string, out: []u8): i64;`
- `export fn toUnicodeDomain(ascii_domain: string, out: []u8): i64;`
