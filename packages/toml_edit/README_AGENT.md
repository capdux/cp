# toml_edit@1.0.0 — AI Agent Quick Ref

- **Source**: `crates.io:toml_edit` (`toml-rs/toml`, MIT, v0.22 parity) | **Deps**: none
- **Import**: `import { TomlDoc, TomlEntry, TOML_STR, TOML_INT, TOML_BOOL, TOML_MAX_ENTRIES } from "toml_edit";`
- **Full Types**: `packages/toml_edit/src/index.d.cp`

## Constants
```cp
TOML_STR:  u8 = 1
TOML_INT:  u8 = 2
TOML_BOOL: u8 = 3
TOML_MAX_ENTRIES: usize = 64
```

## Core Types & Methods
```cp
class TomlEntry {
    section: string, key: string, kind: u8,
    str_val: string, int_val: i64, bool_val: bool,
    fn empty(): TomlEntry
}
class TomlDoc {
    entries: [64]TomlEntry, count: usize,
    fn new(): TomlDoc
    fn parse(src: string): TomlDoc
    fn len(): usize
    fn hasKey(section: string, key: string): bool
    fn getStr(section: string, key: string): string?
    fn getInt(section: string, key: string): i64?
    fn getBool(section: string, key: string): bool?
    fn setStr(_: auto, section: string, key: string, val: string): bool
    fn setInt(_: auto, section: string, key: string, val: i64): bool
    fn setBool(_: auto, section: string, key: string, val: bool): bool
    fn format(out: []u8): usize
}
```

## Minimal Pattern
```cp
import { TomlDoc } from "toml_edit";

let doc = TomlDoc.parse("[pkg]\nname = \"demo\"\nver = 2\n");
* = doc.setInt("pkg", "ver", 3);
let out: [256]u8 = undefined;
const n = doc.format(out[0..]);
```

## Pitfalls & Notes
1. **Root Table Section**: Pass `""` as `section` for top-level keys outside any `[table]`.
2. **Capacity Limit**: Maximum `TOML_MAX_ENTRIES` (64) entries; `set*` updates existing `(section, key)` in place and returns `false` when full.
3. **Borrowed Strings**: `section`, `key`, and `str_val` borrow slices from `src` (or string literals passed to `setStr`); keep source strings alive until `format()` completes.
4. **Scalar Scope**: Supports strings, `i64` (with `_` separators), and `bool`; arrays/inline tables/floats are outside the lightweight v1.0 scalar model.
5. **Internal Helpers**: `tmeStripComment`, `tmeParseInt`, `tmeWriteByte`, `tmeWriteQuotedStr`, `tmeWriteEntry` are internal — use `TomlDoc` methods.
