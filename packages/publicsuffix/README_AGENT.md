# publicsuffix@1.0.0 (Agent Quickref)

> Zero-heap pure-CP Public Suffix List (PSL) parser for eTLD and registrable domain extraction with custom wildcard/exception rules (crates.io publicsuffix equivalent) | License: MIT
> Import: `import { ... } from "publicsuffix";` | Full types: `src/index.d.cp`

## Functions
- `export fn pslToLower(b: u8): u8;`
- `export fn pslIsLabelChar(b: u8): bool;`
- `export fn pslBufEqLowerStr(buf: [256]u8, start: usize, len: usize, s: string): bool;`
- `export fn pslCountPatternLabels(pat: string): usize;`
- `export fn pslMatchSuffixPattern(norm: [256]u8, norm_len: usize, label_starts: [128]usize, label_count: usize, pat: string): usize;`
- `export fn pslMatchBuiltin3(norm: [256]u8, norm_len: usize, label_starts: [128]usize, label_count: usize): bool;`
- `export fn pslMatchBuiltin2(norm: [256]u8, norm_len: usize, label_starts: [128]usize, label_count: usize): bool;`
- `export fn pslMatchBuiltin1(norm: [256]u8, norm_len: usize, label_starts: [128]usize, label_count: usize): bool;`
- `export fn parseDomain(host: string): DomainInfo?;`
- `export fn suffix(host: string): string?;`
- `export fn registrableDomain(host: string): string?;`
- `export fn isPublicSuffix(host: string): bool;`
