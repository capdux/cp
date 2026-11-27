# glob-parent@1.0.0 (Agent Quickref)

> Zero-allocation glob static parent directory extractor, glob detector, and escaper (glob-parent + is-glob parity) for CP | License: MIT
> Import: `import { ... } from "glob-parent";` | Full types: `src/index.d.cp`
> Deps: `is-extglob`

## Functions
- `export fn isGlob(pattern: string): bool;`
- `export fn globParent(pattern: string): string;`
- `export fn splitGlobBase(pattern: string): GlobSplit;`
- `export fn escapeGlob(raw: string, out: []u8): string;`
