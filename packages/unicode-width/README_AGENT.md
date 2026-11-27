# unicode-width@1.0.0 (Agent Quickref)

> Zero-allocation Unicode 15.1 East Asian Width & terminal display column width calculator with UTF-8 safe truncation for CP. | License: MIT
> Import: `import { ... } from "unicode-width";` | Full types: `src/index.d.cp`

## Functions
- `export fn charWidth(cp: u32): usize;`
- `export fn charWidthCjk(cp: u32): usize;`
- `export fn strWidth(s: string): usize;`
- `export fn strWidthCjk(s: string): usize;`
- `export fn truncateToWidth(s: string, max_cols: usize): usize;`
