# `@cap/helmet-headers`

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:helmet` ([https://github.com/helmetjs/helmet](https://github.com/helmetjs/helmet))（原库名 `helmet` → CP 包名 `@cap/helmet-headers`）

Zero-heap HTTP security response headers policy generator in pure CP (clean-room MIT counterpart to npm `helmet` v8).

## Features

- **100% Zero Heap Allocation**: Formats full CRLF-delimited HTTP security header blocks directly into caller-provided `out: []u8`.
- **Comprehensive Security Header Coverage**:
  - `Content-Security-Policy` (CSP)
  - `Strict-Transport-Security` (HSTS with `max-age`, `includeSubDomains`, `preload`)
  - `X-Content-Type-Options: nosniff`
  - `X-Frame-Options` (`DENY` / `SAMEORIGIN`)
  - `Referrer-Policy`
  - `Cross-Origin-Opener-Policy` (COOP)
  - `Cross-Origin-Resource-Policy` (CORP)
  - `X-DNS-Prefetch-Control`
  - `X-Permitted-Cross-Domain-Policies`
  - `Origin-Agent-Cluster: ?1`
- **CRLF / Header-Splitting Defense**: `isSafeHeaderValue(val)` rejects `\r`, `\n`, `\0`, DEL, and control characters; unsafe values are automatically dropped during formatting.
- **Presets**: `HelmetOptions.defaultStrict()` and `HelmetOptions.minimal()`.

## Quick Start

```cp
import { HelmetOptions, formatHelmetHeaders, formatHstsHeader, formatCspHeader } from "@cap/helmet-headers";

let out: [1024]u8 = undefined;
const opts = HelmetOptions.defaultStrict();
const n = formatHelmetHeaders(opts, &out);
```

## API

- `HelmetOptions` (`HelmetOptions.defaultStrict()`, `HelmetOptions.minimal()`)
- `formatHelmetHeaders(opts: HelmetOptions, out: []u8): usize`
- `formatHstsHeader(max_age: u64, include_subdomains: bool, preload: bool, out: []u8): usize`
- `formatCspHeader(default_src, script_src, style_src, img_src, connect_src, out: []u8): usize`
- `isSafeHeaderValue(val: string): bool`

## License

MIT
