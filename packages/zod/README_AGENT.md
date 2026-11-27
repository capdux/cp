# zod@1.0.0 (Agent Quickref)

> Zero-heap type-safe schema validation framework for CP (clean-room MIT implementation of npm zod v3). | License: MIT
> Import: `import { ... } from "zod";` | Full types: `src/index.d.cp`
> Deps: `kind-of`, `url`

## Functions
- `export fn zString(raw: string, min_len: usize, max_len: usize): bool;`
- `export fn zNumber(raw: string, min_v: f64, max_v: f64): bool;`
- `export fn zEmail(s: string): bool;`
- `export fn zUrl(s: string): bool;`
- `export fn zUuid(s: string): bool;`
- `export fn zEnum(s: string, csv: string): bool;`
