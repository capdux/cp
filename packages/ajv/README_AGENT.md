# ajv — AI Agent Quick Ref

Zero-heap JSON Schema Draft-07 / 2020-12 validation engine for CP (clean-room MIT implementation of npm ajv v8). (`v1.0.0`, `MIT`)

## Import

```cp
import {
    AjvError,
    AjvResult,
    AjvSchema,
    ajvIsSpace,
    ajvTrimBounds,
    ajvParseNumber,
    ajvDecodeJsonString,
    ajvScanValue,
} from "ajv";
```

## Core Types & Constants

```cp
class AjvError { instance_path: [64]u8, instance_path_len: usize, keyword: [32]u8, keyword_len: usize, /* +1 fields */ } // methods: empty(...) +2
class AjvResult { valid: bool, error_count: usize, errors: [16]AjvError } // methods: ok(...) +3
class AjvSchema { source: string, ok: bool } // methods: compile(...) +1
```

## API Signatures

```cp
fn ajvIsSpace(c: u8): bool;
fn ajvTrimBounds(s: string, out_start: &mut usize, out_end: &mut usize);
fn ajvParseNumber(raw: string, out_val: &mut f64, out_is_int: &mut bool): bool;
fn ajvDecodeJsonString(raw: string, out: []u8, out_len: &mut usize): bool;
fn ajvScanValue(s: string, pos: usize, end: usize, out_end: &mut usize): bool;
fn ajvClassifyStrict(raw: string, depth: usize): u8;
fn ajvFindObjectProp(obj: string, target_key: string, out_start: &mut usize, out_end: &mut usize): bool;
fn ajvCountArrayItems(arr: string): usize;
fn ajvJsonValuesEqual(raw_a: string, raw_b: string): bool;
fn ajvIsIsoDate(s: string): bool;
fn validateFormat(fmt: string, value: string): bool;
fn ajvMatchSingleType(type_name: string, data_kind: u8, data_raw: string): bool;
fn ajvJoinPath(base: string, seg: string, out: []u8): usize;
fn ajvValidateNode(schema: string, data: string, path: string, depth: usize, res: &mut AjvResult);
fn validateWithSchema(schema_json: string, data_json: string): AjvResult;
```

## Minimal Usage

```cp
import { validateFormat } from "ajv";

pub fn main() {
    let result = validateFormat("example", "example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"ajv"`; install via `cap add ajv`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
