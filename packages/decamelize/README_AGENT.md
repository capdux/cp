# decamelize@1.0.0 (Agent Quickref)

> Convert a camelized string into a lowercased one with a custom separator (clean-room CP implementation of npm decamelize) | License: MIT
> Import: `import { ... } from "decamelize";` | Full types: `src/index.d.cp`

## Functions
- `export fn decamelizeWithOptions(input: string, separator: string, preserve_consecutive_uppercase: bool, out: []u8): usize;`
- `export fn decamelize(input: string, separator: string, out: []u8): usize;`
- `export fn toSnakeCase(input: string, out: []u8): usize;`
- `export fn toKebabCase(input: string, out: []u8): usize;`
