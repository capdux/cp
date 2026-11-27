# path-key@1.0.0 (Agent Quickref)

> Zero-heap cross-platform PATH environment variable key lookup and PATH list splitting matching npm path-key in pure CP | License: MIT
> Import: `import { ... } from "path-key";` | Full types: `src/index.d.cp`

## Functions
- `export fn isPathKeyName(key: string): bool;`
- `export fn findPathKeyIndex(keys: []string, is_windows: bool): i64;`
- `export fn defaultPathKey(is_windows: bool): string;`
- `export fn pathDelimiter(is_windows: bool): u8;`
- `export fn splitPathEntries(path_val: string, is_windows: bool, out: []PathSpan): usize;`
