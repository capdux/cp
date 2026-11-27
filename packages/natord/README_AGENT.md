# natord@1.0.0 (Agent Quickref)

> Zero-heap arbitrary-precision human natural sort comparator, case-insensitive comparator, and in-place sorter. | License: MIT
> Import: `import { ... } from "natord";` | Full types: `src/index.d.cp`

## Functions
- `export fn compareImpl(a: string, b: string, ignore_case: bool): i32;`
- `export fn compareNatural(a: string, b: string): i32;`
- `export fn compareNaturalIgnoreCase(a: string, b: string): i32;`
- `export fn isSortedNatural(items: &[]string): bool;`
- `export fn sortNatural(items: []string);`
