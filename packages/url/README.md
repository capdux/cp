[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# `url`

Pure CP zero-allocation WHATWG URL Standard parser, normalizer, origin extractor, and relative URL resolver — clean-room parity with **crates.io `url` v2** (`servo/rust-url`).

## Provenance & Zero-Cost Migration

| Field | Details |
| :--- | :--- |
| **Source Registry** | `crates.io:url` ([servo/rust-url](https://github.com/servo/rust-url), v2 parity) |
| **CP Package** | `url@1.0.0` (`MIT`) |
| **Dependencies** | `percent-encoding@1.0.0`, `idna@1.0.0` |

Keep the exact original package name — no renaming required:

```rust
// Rust (crates.io url)
use url::Url;
let base = Url::parse("https://example.com:8443/api/v1/items?page=1#top")?;
let joined = base.join("../users")?;
```

```cp
// CP — identical package name, zero-allocation stack struct
import { WhatwgUrl } from "url";

const base = WhatwgUrl.parse("https://example.com:8443/api/v1/items?page=1#top");
const joined = WhatwgUrl.join(base, "../users");
```

## ReleaseFast Performance

Measured on Intel Xeon Platinum 8175M (`cpc 1.0.0` + `zig 0.17.0 -OReleaseFast -fllvm`, see [`bench/ECOSYSTEM_PERF_REPORT.md`](../../bench/ECOSYSTEM_PERF_REPORT.md)):

| Operation | Payload | Latency | Throughput | Bandwidth | Architecture |
| :--- | :--- | --: | --: | --: | :--- |
| `WhatwgUrl.parse` | 96 B full HTTPS URL | **487.78 ns/op** | **2,050,084 ops/s** | 187.69 MB/s | 16-lane SIMD scan, zero-alloc |

- **Zero heap allocations**: All URL components (`scheme`, `username`, `password`, `host`, `path`, `query`, `fragment`) live in fixed inline stack buffers inside `WhatwgUrl`.
- **Full WHATWG normalization**: IDNA Punycode host conversion, IPv6 compression, dot-segment removal (`/a/b/../c` -> `/a/c`), default port elision (`:443` on `https`, `:80` on `http`), and percent-encoding.

## Examples

### 1. Parsing, Component Inspection & Serialization

```cp
import { WhatwgUrl, defaultPortForScheme } from "url";

fn demoParse() {
    const u = WhatwgUrl.parse("https://alice:secret@Example.COM:443/a/b/../c?k=v#sec");
    if (!u.ok) { return; }

    const scheme = u.schemeStr();      // "https"
    const host = u.hostStr();          // "example.com" (lowercased)
    const path = u.pathStr();          // "/a/c" (dot-segments normalized)
    const query = u.queryStr();        // "k=v"
    const frag = u.fragmentStr();      // "sec"
    const eff_port = u.effectivePort(); // 443 (u.port == -1 because default port is elided)
    const def_ws = defaultPortForScheme("wss"); // 443

    let out: [512]u8 = undefined;
    const n = u.toStr(out[0..]);       // "https://alice:secret@example.com/a/c?k=v#sec"
    _ = scheme; _ = host; _ = path; _ = query; _ = frag; _ = eff_port; _ = def_ws; _ = n;
}
```

### 2. Relative URL Resolution (`join`) & Origin Extraction (`origin`)

```cp
import { WhatwgUrl } from "url";

fn demoJoinAndOrigin() {
    const base = WhatwgUrl.parse("https://api.example.com:8443/v1/users/list");
    if (!base.ok) { return; }

    const next = WhatwgUrl.join(base, "../profile?full=1");
    if (!next.ok) { return; }

    let url_buf: [512]u8 = undefined;
    const url_len = next.toStr(url_buf[0..]);
    // url_buf[0..url_len] == "https://api.example.com:8443/v1/profile?full=1"

    let orig_buf: [256]u8 = undefined;
    const orig_len = next.origin(orig_buf[0..]);
    // orig_buf[0..orig_len] == "https://api.example.com:8443"
    _ = url_len; _ = orig_len;
}
```

### 3. Edge Cases: IPv6 Hosts, Invalid URLs & Buffer Sizing

```cp
import { WhatwgUrl } from "url";

fn demoEdgeCases() {
    // IPv6 literal normalization
    const v6 = WhatwgUrl.parse("http://[2001:0db8::1]:8080/status");
    if (v6.ok) {
        const host = v6.hostStr(); // "[2001:db8::1]"
        const port = v6.port;      // 8080
        _ = host; _ = port;
    }

    // Malformed URL safely sets ok == false
    const bad = WhatwgUrl.parse("http://[::1:999999");
    if (!bad.ok) {
        // reject malformed input cleanly
    }

    // Opaque origin for non-special schemes (e.g., data:, mailto:) returns "null"
    const mail = WhatwgUrl.parse("mailto:dev@example.com");
    let orig: [64]u8 = undefined;
    const n = mail.origin(orig[0..]); // "null"
    _ = n;
}
```

## API Overview

- `WhatwgUrl.parse(input)` — parse and normalize an absolute URL
- `WhatwgUrl.join(base, rel)` — resolve a relative reference against `base`
- Accessors: `schemeStr()`, `usernameStr()`, `passwordStr()`, `hostStr()`, `pathStr()`, `queryStr()`, `fragmentStr()`, `effectivePort()`
- Formatting: `toStr(out)`, `origin(out)`
- Utilities: `defaultPortForScheme(scheme)`, `wuCloneUrl(src)`

See [`src/index.d.cp`](src/index.d.cp) for full signatures.
