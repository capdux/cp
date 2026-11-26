# `@cap/merge-descriptors`

Zero-allocation property descriptor table and safe descriptor merge engine for CP. Clean-room MIT implementation inspired by npm `merge-descriptors` v2.

## Features

- **Zero heap allocation**: Fixed 32-slot `DescriptorTable` storing `PropDescriptor` entries with insertion-order preservation.
- **Full `merge-descriptors` semantics**: Supports `redefine = true` (overwrite configurable existing keys) and `redefine = false` (only add missing keys).
- **`configurable` guard enforcement**: Non-configurable properties are protected from both redefinition during `define`/`mergeDescriptors` and deletion via `deleteKey`.
- **Enumerable tracking**: Includes `countEnumerable` to count visible descriptors.

## API

### `PropDescriptor`

```cp
pub struct PropDescriptor {
    pub key: string,
    pub value: string,
    pub writable: bool,
    pub enumerable: bool,
    pub configurable: bool,

    pub static fn data(key: string, value: string): PropDescriptor
    pub static fn readonly(key: string, value: string): PropDescriptor
}
```

### `DescriptorTable`

- `DescriptorTable.new(): DescriptorTable`
- `len(): usize`
- `define(*_, desc: PropDescriptor): bool`
- `get(key: string): PropDescriptor?`
- `has(key: string): bool`
- `deleteKey(*_, key: string): bool`

### Functions

- `mergeDescriptors(dst: *DescriptorTable, src: DescriptorTable, redefine: bool): usize`
- `countEnumerable(tbl: DescriptorTable): usize`

## Example

```cp
import {
    PropDescriptor,
    DescriptorTable,
    mergeDescriptors,
    countEnumerable
} from "@cap/merge-descriptors";

let dst = DescriptorTable.new();
dst.define(PropDescriptor.data("name", "app"));
dst.define(PropDescriptor.readonly("version", "1.0.0"));

let src = DescriptorTable.new();
src.define(PropDescriptor.data("name", "express-app"));
src.define(PropDescriptor.data("version", "2.0.0"));
src.define(PropDescriptor.data("env", "production"));

let merged = mergeDescriptors(&dst, src, true);
// merged == 2 ("name" updated, "env" added; non-configurable "version" protected)
// countEnumerable(dst) == 3
```

## License

MIT
