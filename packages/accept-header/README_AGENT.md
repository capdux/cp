# accept-header@1.0.0 (Agent Quickref)

> Zero-heap RFC 7231 HTTP Accept header parser and media-type negotiator with quality-factor & specificity ranking (clean-room port of Rust accept-header) | License: MIT
> Import: `import { ... } from "accept-header";` | Full types: `src/index.d.cp`
> Deps: `negotiator`, `unicase`

## Functions
- `export fn acphFindSlash(s: string): usize;`
- `export fn acphIsStar(s: string): bool;`
- `export fn acphMediaSpecificity(range: string): u8;`
- `export fn acphMatchesRange(range: string, mime: string): bool;`
- `export fn acphEmptyEntry(): AcceptEntry;`
- `export fn acphEmptySpec(): QualitySpec;`
- `export fn negotiateMediaType(header: string, available: []string): string?;`
