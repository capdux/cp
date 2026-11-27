# ini@1.0.0 — AI Agent Quick Ref

- **Source**: `npm:ini` (`npm/ini`, ISC) | **Deps**: none
- **Import**: `import { IniDoc, IniEntry, IniWriter, MAX_ENTRIES } from "ini";`
- **Full Types**: `packages/ini/src/index.d.cp`

## Constants
```cp
MAX_ENTRIES: usize = 128
```

## Core Types & Methods
```cp
class IniDoc {
    entries: [128]IniEntry, count: usize,
    fn parse(src: &[]u8): IniDoc
    fn len(): usize
    fn findIndex(src: &[]u8, section: &[]u8, key: &[]u8): i64
    fn has(src: &[]u8, section: &[]u8, key: &[]u8): bool
    fn get(src: &[]u8, section: &[]u8, key: &[]u8): &[]u8
    fn getBool(src: &[]u8, section: &[]u8, key: &[]u8, default_val: bool): bool
    fn getInt(src: &[]u8, section: &[]u8, key: &[]u8, default_val: i64): i64
}

class IniWriter {
    buf: []u8, pos: usize, overflow: bool,
    fn init(buf: []u8): IniWriter
    fn writeByte(_: auto, b: u8): bool
    fn writeSlice(_: auto, s: &[]u8): bool
    fn writeSection(_: auto, name: &[]u8): bool
    fn writeEntry(_: auto, key: &[]u8, value: &[]u8): bool
    fn written(): &[]u8
    fn len(): usize
}
```

## Minimal Pattern
```cp
import { IniDoc, IniWriter } from "ini";

const src = "[app]\nport = 8080\ndebug = yes\n";
const doc = IniDoc.parse(src);
const port = doc.getInt(src, "app", "port", 3000); // 8080

let buf: [128]u8 = undefined;
let w = IniWriter.init(buf[0..]);
* = w.writeSection("app");
* = w.writeEntry("port", "8080");
```

## Pitfalls & Notes
1. **Pass `src` on Queries**: `IniDoc` stores byte offsets only — pass the exact same `src` buffer to `get`/`has`/`findIndex`/`getBool`/`getInt`.
2. **Global Keys**: Keys before any `[section]` use `""` as `section`.
3. **Missing vs Empty**: `get()` returns an empty slice when missing; use `has()` or `findIndex() >= 0` to distinguish missing keys from empty values.
4. **Entry Cap**: Excess entries beyond `MAX_ENTRIES` (128) are silently skipped during `parse()`.
5. **Writer Overflow**: Check `w.overflow` (or boolean return values) before consuming `w.written()`.
