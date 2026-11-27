# escape-string-regexp@1.0.0 (Agent Quickref)

> Escape RegExp special characters in a string (clean-room CP implementation of npm escape-string-regexp) | License: MIT
> Import: `import { ... } from "escape-string-regexp";` | Full types: `src/index.d.cp`

## Functions
- `export fn esrIsSpecial(c: u8): bool;`
- `export fn hasRegexpMeta(input: string): bool;`
- `export fn escapedRegexpLen(input: string): usize;`
- `export fn escapeStringRegexp(input: string, out: []u8): usize;`
- `export fn unescapeStringRegexp(escaped: string, out: []u8): usize;`
