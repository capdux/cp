# negotiator@1.0.0 (Agent Quickref)

> Zero-heap RFC 7231 HTTP content negotiation for Accept, Accept-Encoding, Accept-Language, and Accept-Charset with q-value ranking (clean-room port of npm negotiator) | License: MIT
> Import: `import { ... } from "negotiator";` | Full types: `src/index.d.cp`
> Deps: `unicase`

## Functions
- `export fn parseQValue(raw: string): u32;`
- `export fn parseWeightedList(header: string, out: []QualitySpec): usize;`
- `export fn negotiateEncoding(accept_encoding: string, available: &[]string): string?;`
- `export fn negotiateLanguage(accept_language: string, available: &[]string): string?;`
- `export fn negotiateCharset(accept_charset: string, available: &[]string): string?;`
- `export fn parseAcceptList(header: string, out: []NegSpec): usize;`
- `export fn bestEncoding(header: string, available: &[]string): i64;`
- `export fn bestMediaType(header: string, available: &[]string): i64;`
- `export fn bestLanguage(header: string, available: &[]string): i64;`
