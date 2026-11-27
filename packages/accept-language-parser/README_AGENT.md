# accept-language-parser@1.0.0 (Agent Quickref)

> Zero-heap RFC 9110 / BCP-47 Accept-Language quality weight parser and language negotiation engine. | License: MIT
> Import: `import { ... } from "accept-language-parser";` | Full types: `src/index.d.cp`

## Functions
- `export fn isWs(c: u8): bool;`
- `export fn isValidTag(tag: string): bool;`
- `export fn parseQuality(s: string): i32;`
- `export fn parseAcceptLanguage(header: string, out: []LangRange): usize;`
- `export fn matchesTag(range_tag: string, supported_tag: string): bool;`
- `export fn isExcluded(ranges: &[]LangRange, count: usize, candidate: string): bool;`
- `export fn pickLanguage(header: string, supported: &[]string): string;`
