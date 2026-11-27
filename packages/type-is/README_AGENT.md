# type-is@1.0.0 (Agent Quickref)

> Zero-heap HTTP Content-Type matcher supporting shorthands, wildcards (text/*), and structured suffixes (+json) (clean-room port of npm type-is) | License: MIT
> Import: `import { ... } from "type-is";` | Full types: `src/index.d.cp`
> Deps: `mime-types`, `unicase`

## Functions
- `export fn normalizeTypePattern(pattern: string): string;`
- `export fn normalize(pattern: string): string?;`
- `export fn extractEssence(content_type: string): string?;`
- `export fn isValidContentType(content_type: string): bool;`
- `export fn typeMatches(content_type: string, pattern: string): bool;`
- `export fn matchType(expected: string, actual: string): bool;`
- `export fn is(content_type: string, types: &[]string): string?;`
- `export fn typeIs(content_type: string, candidates_csv: string): string?;`
- `export fn hasBody(transfer_encoding: string, content_length: string): bool;`
- `export fn requestHasBody(req: RequestHeaders): bool;`
- `export fn typeis(req: RequestHeaders, types: &[]string): string?;`
- `export fn typeisCsv(req: RequestHeaders, candidates_csv: string): string?;`
- `export fn isJson(content_type: string): bool;`
- `export fn isText(content_type: string): bool;`
- `export fn isForm(content_type: string): bool;`
- `export fn isMultipart(content_type: string): bool;`
