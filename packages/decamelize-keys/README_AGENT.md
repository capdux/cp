# decamelize-keys@1.0.0 (Agent Quickref)

> Zero-heap object key decamelizer with custom separator, exclude list, and fixed key buffer pool, composing @cap/map-obj and @cap/decamelize (clean-room port of npm decamelize-keys) | License: MIT
> Import: `import { ... } from "decamelize-keys";` | Full types: `src/index.d.cp`
> Deps: `decamelize`, `map-obj`

## Functions
- `export fn dckStrEq(a: string, b: string): bool;`
- `export fn dckIsExcluded(k: string, exclude: []string): bool;`
- `export fn decamelizeKeys(src: MapObjResult, sep: string, exclude: []string): DecamelizeKeysResult;`
