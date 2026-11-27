# type-is@1.0.0 (Agent Quickref)

> Zero-heap HTTP Content-Type matcher supporting shorthands, wildcards (text/*), and structured suffixes (+json) (clean-room port of npm type-is) | License: MIT
> Import: `import { ... } from "type-is";` | Full types: `src/index.d.cp`
> Deps: `mime-types`, `unicase`

## Functions
- `export fn normalizeTypePattern(pattern: string): string;`
- `export fn typeMatches(content_type: string, pattern: string): bool;`
- `export fn typeIs(content_type: string, candidates_csv: string): string?;`
- `export fn hasBody(transfer_encoding: string, content_length: string): bool;`
