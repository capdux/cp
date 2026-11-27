# fast-json-stable-stringify@1.0.0 — AI Agent Quick Ref

> Zero-alloc RFC 8785 JSON Canonicalization Scheme (JCS) serializer & equivalence checker | Source: `npm:fast-json-stable-stringify` | License: MIT
> Import: `import { JcsResult, canonicalizeJson, areJsonEquivalent } from "fast-json-stable-stringify";` | Types: `src/index.d.cp`

## Core Types

```cp
export class JcsResult {
    ok: bool,
    len: usize,
    output: string,
    fn fail(out: []u8): JcsResult;
}
```

## Core Functions

- `canonicalizeJson(input: string, out: []u8): JcsResult` — Canonicalize JSON per RFC 8785 (UTF-16 key order, `-0` -> `0`, stripped whitespace) into `out`.
- `areJsonEquivalent(a: string, b: string): bool` — Compare semantic equivalence of two JSON strings (internal 4096B stack buffers).
- `compareJsonKeys(input: string, s1: usize, e1: usize, s2: usize, e2: usize): i8` — Low-level UTF-16 code-unit comparator (`-1`, `0`, `1`).
- `emitCanonicalValue(input: string, start: usize, out: []u8, out_pos: usize, depth: u8): StepRes`

## Minimal Usage

```cp
import { canonicalizeJson, areJsonEquivalent } from "fast-json-stable-stringify";

let buf: [1024]u8 = undefined;
const res = canonicalizeJson("{\"b\":2,\"a\":1}", buf[0..1024]);
if (res.ok) {
    // res.output == "{\"a\":1,\"b\":2}"
}
const eq = areJsonEquivalent("{\"b\":2,\"a\":1}", "{ \"a\": 1, \"b\": 2 }"); // true
```

## Pitfalls & Rules

1. **Always check `res.ok`**: Returns `ok == false` on invalid JSON (trailing commas, leading zeros, trailing data), buffer overflow, or exceeded limits.
2. **Hard stack limits**: Max nesting depth is `16`; max keys per JSON object is `64`. Exceeding either yields `ok == false`.
3. **`areJsonEquivalent` size cap**: Uses internal `[4096]u8` buffers; inputs whose canonical form exceeds 4096B return `false`. For larger payloads, call `canonicalizeJson` on both with larger buffers and compare via `cap:bin` `bytesEq`.
4. **Slice lifetime**: `res.output` borrows `out[0..res.len]`; do not mutate `out` while holding `res.output`.
5. **Array order preserved**: Only object keys are sorted (UTF-16 code-unit order); JSON array element order is never altered.
