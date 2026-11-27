# snakecase-keys@1.0.0 (Agent Quickref)

> Zero-heap object key converter to snake_case with exclude list and fixed key buffer pool, composing map-obj and heck (clean-room port of npm snakecase-keys) | License: MIT
> Import: `import { ... } from "snakecase-keys";` | Full types: `src/index.d.cp`
> Deps: `heck`, `map-obj`

## Functions
- `export fn sckStrEq(a: string, b: string): bool;`
- `export fn sckIsExcluded(k: string, exclude: []string): bool;`
- `export fn snakecaseKeyInto(key: string, out: []u8): usize;`
- `export fn snakecaseKeys(src: MapObjResult, exclude: []string): SnakecaseKeysResult;`
