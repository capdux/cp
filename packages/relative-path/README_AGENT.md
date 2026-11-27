# relative-path@1.0.0 (Agent Quickref)

> Portable platform-independent relative path manipulation, normalization, joining, and resolution (zero-heap pure CP) | License: MIT
> Import: `import { ... } from "relative-path";` | Full types: `src/index.d.cp`

## Functions
- `export fn rpIsSep(b: u8): bool;`
- `export fn normalizeRelPath(path: string): RelPath;`
- `export fn joinRelPath(base: string, rel: string): RelPath;`
- `export fn relativeTo(from_path: string, to_path: string): RelPath;`
