# kebabcase-keys@1.0.0 (Agent Quickref)

> Zero-heap object key converter to kebab-case with exclude list, batch conversion, and nested dot-path support, composing decamelize-keys and map-obj (clean-room port of npm kebabcase-keys) | License: MIT
> Import: `import { ... } from "kebabcase-keys";` | Full types: `src/index.d.cp`
> Deps: `decamelize-keys`, `map-obj`

## Functions
- `export fn kckCopySlice(src_buf: [1024]u8, off: usize, len: usize, out: []u8, dst_off: usize): usize;`
- `export fn kebabcaseKeys(src: MapObjResult, exclude: []string): DecamelizeKeysResult;`
- `export fn kebabcaseKeysAll(src: MapObjResult): DecamelizeKeysResult;`
- `export fn kebabcaseKeyInto(key: string, out: []u8): usize;`
- `export fn kebabcasePathInto(dot_path: string, out: []u8): usize;`
- `export fn kebabcaseKeysBatch(items: []MapObjResult, exclude: []string, out: []DecamelizeKeysResult): usize;`
