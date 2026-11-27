# html-encoding-sniffer@1.0.0 (Agent Quickref)

> Zero-allocation WHATWG HTML 13.2.3.2 byte stream encoding sniffer and label normalizer (npm html-encoding-sniffer parity) for CP | License: MIT
> Import: `import { ... } from "html-encoding-sniffer";` | Full types: `src/index.d.cp`

## Functions
- `export fn hesToLower(ch: u8): u8;`
- `export fn hesIsAsciiWs(ch: u8): bool;`
- `export fn hesTrimAsciiWs(s: string): string;`
- `export fn hesEqIgnoreCase(a: string, b: string): bool;`
- `export fn hesStartsWithIgnoreCase(bytes: &[]u8, pos: usize, needle: string): bool;`
- `export fn normalizeEncodingLabel(label: string): string;`
- `export fn hesMapMetaEncoding(label: string): string;`
- `export fn hesExtractCharsetFromContent(content: string): string;`
- `export fn hesPrescanMeta(bytes: &[]u8): string;`
- `export fn sniffHtmlEncoding(bytes: &[]u8, opts: SniffOptions): string;`
