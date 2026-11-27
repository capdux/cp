# depd@1.0.0 (Agent Quickref)

> Zero-heap deprecation notice formatter and deduplicated warning tracker matching npm depd (150M+/wk) in pure CP | License: MIT
> Import: `import { ... } from "depd";` | Full types: `src/index.d.cp`
> Deps: `@sindresorhus/fnv1a`

## Functions
- `export fn depdHashKey(ns: string, msg: string): u64;`
- `export fn copySlice(dst: []u8, pos: usize, src: string): usize;`
- `export fn formatDeprecation(namespace: string, msg: string, site: string, out: []u8): usize;`
