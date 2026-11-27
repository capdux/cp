# space-separated-tokens@2.0.2 (Agent Quickref)

> Zero-allocation HTML/SVG space-separated and comma-separated token list parser and serializer | License: MIT
> Import: `import { ... } from "space-separated-tokens";` | Full types: `src/index.d.cp`

## Functions
- `export fn parseSpaceTokens(input: string, out: []string): usize;`
- `export fn hasSpaceToken(input: string, target: string): bool;`
- `export fn stringifySpaceTokens(tokens: &[]string, out: []u8): usize;`
- `export fn parseCommaTokens(input: string, out: []string): usize;`
- `export fn stringifyCommaTokens(tokens: &[]string, out: []u8): usize;`
