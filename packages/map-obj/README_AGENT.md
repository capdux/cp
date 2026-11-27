# map-obj@1.0.0 (Agent Quickref)

> Zero-heap fixed-pool object key-value map/filter/transform utility with skip sentinel and deep key path support (npm map-obj equivalent) | License: MIT
> Import: `import { ... } from "map-obj";` | Full types: `src/index.d.cp`

## Functions
- `export fn mobjStrEq(a: string, b: string): bool;`
- `export fn swapKeysAndValues(src: MapObjResult): MapObjResult;`
- `export fn pickKeys(src: MapObjResult, allow_keys: []string): MapObjResult;`
- `export fn omitKeys(src: MapObjResult, skip_keys: []string): MapObjResult;`
- `export fn filterEmptyValues(src: MapObjResult): MapObjResult;`
- `export fn mergeObjects(base: MapObjResult, overrides: MapObjResult): MapObjResult;`
