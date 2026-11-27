# iri-string (AI Agent Quick Ref)

- **Import**: `import { IriParts, parseIri, removeDotSegments, resolveReference } from "iri-string";`
- **Source**: `crates.io:iri-string` (MIT, zero-alloc RFC 3986/3987 parser & resolver, `undefined` internal path buffers).

## Core Signatures

```cp
export class IriParts {
    scheme_start: usize, scheme_len: usize,
    auth_start: usize, auth_len: usize,
    path_start: usize, path_len: usize,
    query_start: usize, query_len: usize,
    frag_start: usize, frag_len: usize,
    has_scheme: bool, has_auth: bool,
    has_query: bool, has_frag: bool,
    valid: bool,
}

export fn parseIri(s: string): IriParts;
export fn removeDotSegments(path: string, out: []u8): usize;
export fn resolveReference(base: string, rel: string, out: []u8): usize;
```

## Minimal Pattern

```cp
let p = parseIri("https://example.com/a/b?k=1#top");
if (p.valid && p.has_auth) {
    let auth = s[p.auth_start..(p.auth_start + p.auth_len)];
}

let mut buf: [512]u8 = undefined;
let len = resolveReference("https://example.com/a/b", "../c", buf[0..512]);
let resolved = buf[0..len]; // "https://example.com/c"
```

## Pitfalls

1. **Always check `p.valid` and `p.has_*`**: For example, an empty query (`?`) has `has_query == true, query_len == 0`, while no query has `has_query == false`.
2. **Delimiter exclusion**: `auth` excludes `//`, `query` excludes `?`, `frag` excludes `#`, and `scheme` excludes `:`.
3. **Internal 512-byte path scratch**: `resolveReference` uses internal `[512]u8` merge/path buffers; very long paths exceeding 512 bytes will be truncated/fail.
4. **Overflow return value**: `resolveReference` returns `0` if `base` lacks a valid scheme or if `out` is too small to hold the resolved IRI.
5. **Relative-only input**: `parseIri("/foo/bar")` succeeds with `valid: true, has_scheme: false` — check `p.has_scheme` when requiring an absolute URI/IRI.
