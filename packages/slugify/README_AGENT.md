# slugify@1.0.0 (Agent Quickref)

> Zero-allocation URL slug generator with UTF-8 Latin-1 / Latin Extended-A diacritic folding and truncation for CP. | License: MIT
> Import: `import { ... } from "slugify";` | Full types: `src/index.d.cp`

## Functions
- `export fn slugifyWithSep(input: string, sep: u8, lowercase: bool, out: []u8): usize;`
- `export fn slugify(input: string, out: []u8): usize;`
- `export fn slugifyTruncate(input: string, max_len: usize, out: []u8): usize;`
- `export fn isValidSlug(s: string): bool;`
