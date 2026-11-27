# lodash — AI Agent Quick Reference

- **Package**: `lodash` (`v1.0.0`) | **Source**: npm `lodash` / `dot-prop` | **Import**: `import { ... } from "lodash";`
- **Role**: Zero-allocation dot-path parser & deep JSON path reader (no DOM allocation).

## Core Exports (`src/index.d.cp`)

```cp
class PathSegment {
    is_index: bool,
    key: string,
    index: usize,
}

class ParsedDotPath {
    segs: [32]PathSegment,
    count: usize,
    valid: bool,
    fn get(idx: usize): PathSegment?;
}

fn parseDotPath(path: string, key_buf: []u8): ParsedDotPath;
fn getJsonPath(json: string, path: string): string?;
fn getJsonString(json: string, path: string): string?;
fn hasJsonPath(json: string, path: string): bool;
fn escapePathSegment(seg: string, out: []u8): string;
```

## Minimal Usage

```cp
import { getJsonPath, getJsonString, hasJsonPath, escapePathSegment } from "lodash";

const raw = "{\"a\":{\"b\":[10,\"ok\"]}}";
const num = getJsonPath(raw, "a.b[0]");      // "10"
const str = getJsonString(raw, "a.b[1]");    // "ok" (unquoted)
const ok  = hasJsonPath(raw, "a.b[1]");      // true

let buf: [64]u8 = undefined;
const esc = escapePathSegment("k.1[0]", buf[0..64]); // "k\\.1\\[0\\]"
```

## Pitfalls & Constraints

1. **String vs raw JSON**: `getJsonString` strips outer `"` quotes and returns `null` for non-string values (numbers/bools/objects); use `getJsonPath` for non-string JSON values.
2. **Escaped keys**: Dots/brackets inside literal keys must be escaped with `\` (e.g. `"a\\.b"`).
3. **Segment/buffer limits**: Max `32` path segments (`MAX_SEGMENTS = 32`); `getJsonPath` uses an internal `512`-byte key buffer. Exceeding limits returns `valid: false` / `null`.
4. **Slice lifetime**: `parseDotPath` stores slices pointing into `key_buf` — keep `key_buf` alive while using `ParsedDotPath`.
5. **Scope**: This package implements deep path parsing & JSON path lookup (`lodash.get` / `dot-prop`), not the full JS utility collection.
