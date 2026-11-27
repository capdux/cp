[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# fast-json-stable-stringify

Zero-allocation RFC 8785 JSON Canonicalization Scheme (JCS) deterministic serializer and semantic equivalence checker for CP — clean-room MIT parity with npm `fast-json-stable-stringify`, `canonicalize`, and Rust `serde_jcs`.

Recursively sorts JSON object keys in RFC 8785 UTF-16 code-unit order, strips insignificant whitespace, normalizes Unicode escape sequences and numbers (`-0` / `-0.0` → `0`), and writes directly into caller-provided stack buffers — ideal for HMAC/webhook signing, digital signatures, cache keys, and content addressing.

## Provenance & Zero-Cost Migration

| Attribute | Value |
| --- | --- |
| Source Platform | **npm** |
| Original Package | `fast-json-stable-stringify` (CP `v1.0.0`) |
| Upstream Repo | `https://github.com/epoberezkin/fast-json-stable-stringify` |
| License | MIT |

Keep the exact package name when migrating from TypeScript/Node.js to CP:

```ts
// TypeScript / Node.js (npm: fast-json-stable-stringify)
import stringify from "fast-json-stable-stringify";
const canonical = stringify({ z: 1, a: [true, -0, null] });
```

```cp
// CP (cap: fast-json-stable-stringify) — zero-allocation RFC 8785 canonicalizer
import { canonicalizeJson } from "fast-json-stable-stringify";

let buf: [1024]u8 = undefined;
const res = canonicalizeJson("{\"z\": 1, \"a\": [true, -0, null]}", buf[0..1024]);
// res.ok == true, res.output == "{\"a\":[true,0,null],\"z\":1}"
```

## Installation

```bash
cap add fast-json-stable-stringify
```

## Usage

### Basic Usage: Deterministic Signing Payload & Equivalence Check

```cp
import { canonicalizeJson, areJsonEquivalent } from "fast-json-stable-stringify";

fn demoCanonical() {
    let buf: [1024]u8 = undefined;
    const raw = "{\"z\": 1, \"a\": [true, -0.0, null], \"m\": {\"y\": 2, \"x\": 3}}";
    const res = canonicalizeJson(raw, buf[0..1024]);
    if (res.ok) {
        // res.output == "{\"a\":[true,0,null],\"m\":{\"x\":3,\"y\":2},\"z\":1}"
        // res.len == res.output.len
    }

    // Semantic equivalence ignoring key order and whitespace
    const same = areJsonEquivalent("{\"b\":1, \"a\":2}", "{ \"a\" : 2 , \"b\" : 1 }"); // true
    _ = res;
    _ = same;
}
```

### Edge Cases: Strict Validation, Small Buffers & Depth/Key Limits

```cp
import { canonicalizeJson, areJsonEquivalent } from "fast-json-stable-stringify";

fn demoEdgeCases() {
    let buf: [512]u8 = undefined;

    // 1. Strict RFC 8259 / 8785 rejection: trailing comma, leading zeros, trailing garbage
    const bad1 = canonicalizeJson("{\"a\": 1,}", buf[0..512]);      // bad1.ok == false
    const bad2 = canonicalizeJson("{\"n\": 01}", buf[0..512]);      // bad2.ok == false
    const bad3 = canonicalizeJson("{\"a\": 1} extra", buf[0..512]); // bad3.ok == false

    // 2. Buffer overflow fails cleanly with ok == false
    let tiny: [4]u8 = undefined;
    const overflow = canonicalizeJson("{\"key\":\"value\"}", tiny[0..4]); // overflow.ok == false

    // 3. Invalid JSON is never equivalent to anything
    const eq_bad = areJsonEquivalent("{bad}", "{bad}"); // false
    _ = bad1;
    _ = bad2;
    _ = bad3;
    _ = overflow;
    _ = eq_bad;
}
```

## API Reference

| Symbol | Signature | Description |
| --- | --- | --- |
| `JcsResult` | `class { ok: bool, len: usize, output: string }` | Canonicalization result; `output` slices `out[0..len]` when `ok == true`. |
| `canonicalizeJson` | `fn(input: string, out: []u8): JcsResult` | Canonicalizes `input` per RFC 8785 into `out`. Returns `ok == false` on invalid JSON, overflow, or limit exceeded. |
| `areJsonEquivalent` | `fn(a: string, b: string): bool` | Checks whether two JSON documents (up to 4096B canonical form) are semantically equivalent. |
| `compareJsonKeys` | `fn(input: string, s1: usize, e1: usize, s2: usize, e2: usize): i8` | Low-level RFC 8785 UTF-16 code-unit key comparator (`-1`, `0`, `1`). |

## Performance & Safety Highlights

- **16-lane SIMD Acceleration**: Uses `cap:ascii` (`isDigit`, `hexVal`, `hexCharLower`, `startsWith`), `cap:bin` (`bytesEq`), and `cap:fmt` (`writeStr`) for fast scanning and equality comparison.
- **Zero Heap Allocation & Fixed Stack Pools**: Sorts up to 64 keys per object on the stack (`[64]ObjEntry`) with a hard nesting limit of `MAX_DEPTH = 16` to prevent stack exhaustion.
- **Zero-Clear Stack Buffers (`undefined`)**: Internal entry arrays and equivalence buffers (`[4096]u8 = undefined`) skip redundant zeroing.
- **Strict RFC Compliance**: Rejects malformed syntax, unpaired surrogates, trailing commas, and buffer overflows deterministically without panicking.

## License

MIT — Clean-room CP implementation inspired by npm `fast-json-stable-stringify`.
