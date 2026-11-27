# qs@1.0.0 — AI Agent Quick Ref

- **Import**: `import { QsParsed, QsParseOptions, QsBuilder, QsError, percentDecode, percentEncode } from "qs";`
- **Provenance**: npm `qs` (v6) parity | Zero-heap | License: MIT
- **Constants**: `MAX_PARAMS = 64`, `MAX_DEPTH = 8`, `POOL_CAPACITY = 4096`
- **Perf (ReleaseFast)**: `percentDecode` = **5.87M ops/s** (660.4 MB/s); `QsParsed.parseInto` = **2.72M ops/s** (367.4 ns/op, 16-lane SIMD + stack pool)
- **Types**: `src/index.d.cp`

## Core Types & Errors
```cp
error QsError { BufferTooSmall, TooManyParams, DepthExceeded, InvalidPercentEncoding, InvalidSyntax }

class QsParseOptions {
    max_params: usize = 64, max_depth: usize = 8,
    allow_dots: bool = false, comma_arrays: bool = false,
    ignore_query_prefix: bool = true, plus_as_space: bool = true,
    strict_null_handling: bool = false,
}
```

## Key Methods & Functions
- **Parse**: `QsParsed.init(): QsParsed`, `QsParsed.parse(input: &[]u8, opts: QsParseOptions): QsParsed ! QsError`, `parsed.parseInto(input: &[]u8, opts: QsParseOptions): void ! QsError`
- **Query**: `get(flat_key)`, `getNested2(k1, k2)`, `getNested3(k1, k2, k3)`, `getArrayItem(key, idx)` -> `(&[]u8)?`; `countArrayItems(key): usize`; `isNull(flat_key): bool`; `len(): usize`; `entryAt(idx): QsEntry?`; `entryKey(idx)` / `entryValue(idx): (&[]u8)?`
- **Percent Codec**: `percentDecode(input: &[]u8, plus_as_space: bool, out: []u8): &[]u8 ! QsError`, `percentEncode(input: &[]u8, space_as_plus: bool, out: []u8): &[]u8 ! QsError`
- **Build**: `QsBuilder.init(buf: []u8)`, `QsBuilder.initWithOptions(buf: []u8, add_query_prefix: bool, space_as_plus: bool)`, `add(k, v)`, `addNested2(k1, k2, v)`, `addNested3(k1, k2, k3, v)`, `addArrayItem(k, idx, v)`, `addBracketArrayItem(k, v)`, `addNull(k)`, `written(): &[]u8`, `reset()`

## Minimal Example
```cp
import { QsParsed, QsParseOptions, QsBuilder } from "qs";
let parsed = QsParsed.init();
parsed.parseInto("?a[b]=1&tags=x,y", QsParseOptions{ comma_arrays: true }) catch return;
const v = parsed.getNested2("a", "b").?; // "1"
let out: [128]u8 = undefined;
let b = QsBuilder.initWithOptions(out[0..], true, false);
b.add("k", "v") catch return;
const s = b.written(); // "?k=v"
```

## Gotchas
1. In hot loops, prefer `let parsed = QsParsed.init(); parsed.parseInto(input, opts)` to avoid copying the ~4 KiB struct by value on return.
2. `QsParsed` holds at most `MAX_PARAMS = 64` entries, `MAX_DEPTH = 8` segments, and `POOL_CAPACITY = 4096` decoded bytes; exceeding raises `QsError.TooManyParams` / `DepthExceeded` / `BufferTooSmall`.
3. Returned `&[]u8` slices from `get*`/`entry*` borrow from either `input` or `parsed.pool` — keep both alive while using them.
4. `QsBuilder` rolls back its cursor atomically on `QsError.BufferTooSmall`.
5. `strict_null_handling: true` distinguishes `foo` (`isNull("foo") == true`, `get("foo") == null`) from `foo=` (`get("foo").? == ""`).
