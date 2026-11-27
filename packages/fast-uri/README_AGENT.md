# fast-uri — AI Agent Quick Ref

- **Package**: `fast-uri` (`1.0.0`) | **Source**: npm `fast-uri` v3 (RFC 3986 URI parser/resolver/comparator)
- **Import**: `import { UriComponents, parseUri, serializeUri, resolveUri, equalUri } from "fast-uri";`
- **Allocation**: Zero-heap; `parseUri` slices `input` in-place, `serializeUri`/`resolveUri` write into caller `[]u8`.

## Core Signatures

```typescript
export class UriComponents {
    valid: bool,
    scheme: string,
    userinfo: string,
    host: string,
    port: i32,         // -1 if no port present
    path: string,
    query: string,
    has_query: bool,
    fragment: string,
    has_fragment: bool,
    fn invalid(): UriComponents;
    fn empty(): UriComponents;
}

export fn parseUri(input: string): UriComponents;
export fn serializeUri(c: UriComponents, out: []u8): usize;
export fn resolveUri(base: string, rel: string, out: []u8): usize;
export fn equalUri(a: string, b: string): bool;
export fn furiRemoveDotSegments(input: string, out: []u8): usize;
export fn furiDefaultPort(scheme: string): i32;
```

## Minimal Pattern

```typescript
import { parseUri, resolveUri, equalUri } from "fast-uri";

const c = parseUri("https://example.com:8443/v1/items?a=1#top");
if (c.valid) {
    let out: [256]u8 = undefined;
    const n = resolveUri("https://example.com/v1/items", "../health", out[0..]);
    const eq = equalUri("HTTPS://EXAMPLE.COM:443/v1", "https://example.com/v1");
}
```

## Pitfalls

1. **Always check `c.valid`**: Invalid IPv6 brackets, bad ports (`> 65535`), or malformed schemes return `UriComponents.invalid()` with `valid == false`.
2. **Absent port is `-1`**: Check `c.port >= 0` before using `port`.
3. **Empty query vs absent query**: Check `c.has_query` / `c.has_fragment` to distinguish `?` / `#` from absent components.
4. **Slice lifetime**: Strings inside `UriComponents` borrow directly from `input`; do not outlive `input`.
