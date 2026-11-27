# url@1.0.0 — AI Agent Quick Ref

- **Import**: `import { WhatwgUrl, defaultPortForScheme } from "url";`
- **Provenance**: crates.io `url` v2 (`servo/rust-url`) parity | Zero-heap | License: MIT
- **Deps**: `percent-encoding`, `idna`
- **Perf (ReleaseFast)**: `WhatwgUrl.parse` (96 B HTTPS URL) = **2.05M ops/s** (487.8 ns/op, 187.7 MB/s, 16-lane SIMD)
- **Types**: `src/index.d.cp`

## Core Class & Functions
```cp
class WhatwgUrl {
    scheme: [32]u8, scheme_len: usize,
    username: [128]u8, username_len: usize,
    password: [128]u8, password_len: usize,
    host: [256]u8, host_len: usize,
    port: i32, // -1 if omitted or equal to scheme default port
    path: [512]u8, path_len: usize,
    query: [512]u8, query_len: usize,
    fragment: [256]u8, fragment_len: usize,
    has_authority: bool, has_query: bool, has_fragment: bool, ok: bool,

    fn invalid(): WhatwgUrl;
    fn parse(input: string): WhatwgUrl;
    fn join(base: WhatwgUrl, rel: string): WhatwgUrl;
    fn schemeStr(): &[]u8;
    fn usernameStr(): &[]u8;
    fn passwordStr(): &[]u8;
    fn hostStr(): &[]u8;
    fn pathStr(): &[]u8;
    fn queryStr(): &[]u8;
    fn fragmentStr(): &[]u8;
    fn effectivePort(): i32; // explicit port or scheme default (-1 if none)
    fn toStr(out: []u8): usize;
    fn origin(out: []u8): usize;
}

export fn defaultPortForScheme(scheme: string): i32;
export fn wuCloneUrl(src: WhatwgUrl): WhatwgUrl;
```

## Minimal Example
```cp
import { WhatwgUrl } from "url";
const base = WhatwgUrl.parse("https://example.com:8443/v1/items?a=1");
if (base.ok) {
    const u = WhatwgUrl.join(base, "../users");
    let buf: [512]u8 = undefined;
    const n = u.toStr(buf[0..]); // "https://example.com:8443/users"
}
```

## Gotchas
1. Always check `u.ok` after `WhatwgUrl.parse()` / `WhatwgUrl.join()`.
2. `u.port` is `-1` when omitted OR when it equals the scheme's default port (e.g., `443` for `https`); use `u.effectivePort()` to get the resolved port number.
3. Inline buffer limits: `scheme` 32B, `username`/`password` 128B, `host`/`fragment` 256B, `path`/`query` 512B.
4. `toStr(out)` / `origin(out)` write to caller-provided `out: []u8` and return bytes written (`>= 512` B recommended for `toStr`).
5. `has_query` / `has_fragment` distinguish empty `?` / `#` from absent query/fragment.
