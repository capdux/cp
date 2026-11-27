# miniz_oxide@3.0.2 (Agent Quickref)

> Single-file lossless compression & ZIP archive creation/extraction engine (miniz 3.0.2) for CP | License: MIT
> Import: `import { ... } from "miniz_oxide";` | Full types: `src/index.d.cp`

## Functions
- `export fn cp_miniz_version(): [*:0]&u8!;`
- `export fn cp_miniz_zip_create_single(entry_name: [*:0]&u8!, data: [*]&u8!, data_len: usize, out_zip_len: *usize!): [*]u8!?;`
- `export fn cp_miniz_zip_extract_entry(zip_bytes: [*]&u8!, zip_len: usize, entry_name: [*:0]&u8!, out_len: *usize!): [*]u8!?;`
- `export fn cp_miniz_free(ptr: *auto!?);`
- `export fn versionString(): string;`
