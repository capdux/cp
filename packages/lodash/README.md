[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# lodash

Zero-allocation dot-path parser and deep JSON path reader for CP (`lodash.get` / `dot-prop` / `dlv` parity).

Extracts deeply nested object fields and array elements directly from raw JSON strings without building a DOM tree or allocating heap memory. Supports bracket array indexing (`a.b[0].c`), backslash-escaped dots/brackets (`a\.b.c`), and path segment escaping.

## Provenance & Zero-Cost Migration

| Attribute | Value |
| --- | --- |
| Source Platform | **npm** |
| Original Package | `lodash` (`lodash.get`) / `dot-prop` (CP `v1.0.0`) |
| Upstream Repo | `https://github.com/lodash/lodash` · `https://github.com/sindresorhus/dot-prop` |
| License | MIT |

Keep the exact package name when migrating from TypeScript/JavaScript to CP:

```ts
// TypeScript / Node.js (npm: lodash / dot-prop)
import get from "lodash/get";
const host = get(JSON.parse(raw), "servers[0].host");
```

```cp
// CP (cap: lodash) — zero-allocation, no JSON DOM parse needed
import { getJsonString, getJsonPath, hasJsonPath } from "lodash";
const host = getJsonString(raw, "servers[0].host");
```

## Installation

```bash
cap add lodash
```

## Usage

### Basic Usage: Deep JSON Reading & Existence Checks

```cp
import { getJsonPath, getJsonString, hasJsonPath } from "lodash";

const payload = "{\"user\":{\"name\":\"Alice\",\"roles\":[\"admin\",\"dev\"],\"age\":30}}";

// Unquoted string value
if (name := getJsonString(payload, "user.name")) {
    // name == "Alice"
}

// Raw JSON value slice (works for numbers, booleans, objects, arrays, strings)
if (age := getJsonPath(payload, "user.age")) {
    // age == "30"
}

// Bracket array indexing & dot-notation indexing
if (role := getJsonString(payload, "user.roles[1]")) {
    // role == "dev"
}

const exists = hasJsonPath(payload, "user.roles[0]"); // true
const missing = hasJsonPath(payload, "user.email");   // false
```

### Advanced & Edge Cases: Escaped Keys & Pre-Parsed Dot Paths

```cp
import { parseDotPath, escapePathSegment, getJsonString } from "lodash";

// 1. Escape literal dots or brackets in dynamic keys into a stack buffer
let escBuf: [128]u8 = undefined;
const escaped = escapePathSegment("api.v1[beta]", escBuf[0..128]);
// escaped == "api\\.v1\\[beta\\]"

// 2. Read keys containing literal dots via backslash escaping
const cfg = "{\"api.v1\":{\"timeout\":5000}}";
const rawTimeout = getJsonPath(cfg, "api\\.v1.timeout"); // "5000"

// 3. Parse a path once into a stack-allocated ParsedDotPath (max 32 segments)
let keyBuf: [512]u8 = undefined;
const parsed = parseDotPath("items[2].meta\\.id", keyBuf[0..512]);
if (parsed.valid) {
    let i: usize = 0;
    while (i < parsed.count) : (i += 1) {
        if (seg := parsed.get(i)) {
            // seg.is_index, seg.key, seg.index
        }
    }
}
```

## API Reference

| Symbol | Signature | Description |
| --- | --- | --- |
| `PathSegment` | `class { is_index: bool, key: string, index: usize }` | Single parsed path segment (object key or array index). |
| `ParsedDotPath` | `class { segs: [32]PathSegment, count: usize, valid: bool, fn get(idx: usize): PathSegment? }` | Stack-allocated parsed path (up to 32 segments). |
| `parseDotPath` | `fn(path: string, key_buf: []u8): ParsedDotPath` | Parses dot/bracket path into segments using caller's `key_buf` for unescaped keys. |
| `getJsonPath` | `fn(json: string, path: string): string?` | Returns raw JSON value slice at `path`, or `null` if missing/invalid. |
| `getJsonString` | `fn(json: string, path: string): string?` | Returns unquoted string value at `path`, or `null` if missing or not a JSON string. |
| `hasJsonPath` | `fn(json: string, path: string): bool` | Returns `true` if `path` resolves to a valid JSON value. |
| `escapePathSegment` | `fn(seg: string, out: []u8): string` | Escapes `.`, `[`, `]`, `\` in `seg` into `out` and returns the written slice. |

## Performance & Limits

- **Zero heap allocation**: Operates directly on input slices and stack buffers.
- **Fixed bounds**: Up to `32` path segments and `512` bytes of internal key buffer in `getJsonPath`.
- **Single-pass scanner**: Skips sibling JSON values without allocating intermediate AST nodes.

## License

MIT — Clean-room CP implementation inspired by `lodash` and `dot-prop`.
