# negotiator@1.0.0 (Agent Quickref)

> Zero-heap RFC 7231 HTTP content negotiation for Accept, Accept-Encoding, Accept-Language, and Accept-Charset with q-value ranking (clean-room port of npm negotiator) | License: MIT
> Import: `import { ... } from "negotiator";` | Full types: `src/index.d.cp`
> Deps: `unicase`

## Functions
- `export fn negIsWs(b: u8): bool;`
- `export fn negTrim(s: string): string;`
- `export fn parseQValue(raw: string): u32;`
- `export fn parseWeightedList(header: string, out: []QualitySpec): usize;`
- `export fn negTokenSpecificity(spec: string, candidate: string, is_lang: bool): u32;`
- `export fn negNegotiateToken(header: string, available: &[]string, is_lang: bool): string?;`
- `export fn negotiateEncoding(accept_encoding: string, available: &[]string): string?;`
- `export fn negotiateLanguage(accept_language: string, available: &[]string): string?;`
- `export fn negotiateCharset(accept_charset: string, available: &[]string): string?;`
- `export fn negTrimStart(s: string, lo: usize, hi: usize): usize;`
- `export fn negTrimEnd(s: string, lo: usize, hi: usize): usize;`
- `export fn negParseQMilli(val: string): u16;`
- `export fn negClassifyToken(tok: string): u8;`
- `export fn parseAcceptList(header: string, out: []NegSpec): usize;`
- `export fn negIsBetterCandidate(q: u16, spec: u8, order: usize, best_q: u16, best_spec: u8, best_order: usize, has_best: bool): bool;`
- `export fn bestEncoding(header: string, available: &[]string): i64;`
- `export fn bestMediaType(header: string, available: &[]string): i64;`
- `export fn negLangPrefixMatch(longer: string, shorter: string): bool;`
- `export fn bestLanguage(header: string, available: &[]string): i64;`
