# `@cap/parseurl`

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:parseurl` ([https://github.com/pillarjs/parseurl](https://github.com/pillarjs/parseurl))

Zero-allocation high-speed HTTP request URL pathname/query zero-copy splitter and path safety validator for CP. Clean-room MIT implementation inspired by Express `parseurl`.

## Features

- **Single-pass O(N) zero-copy parsing**: Splits origin-form (`/api/v1/users?page=2&sort=asc#top`) and absolute-form (`http://host:8080/a/b?x=1`) request URLs into `pathname`, `search`, `query`, `hash`, and `path` slices without heap allocation.
- **RFC-accurate pathname decoding**: `decodePathname` decodes `%XX` sequences while preserving literal `+` characters (unlike form query decoding).
- **Path traversal & null-byte protection**: `isSafeRequestPath` rejects `\0` (`%00`), `\` (`%5C`), and `..` (`%2E%2E`) traversal segments while permitting benign dots like `/files/a..b.txt`.
- **On-demand query parameter extraction**: Delegates to `@cap/querystring` (`pickQueryParam` / `unescapeQuery`) with zero duplication.

## API

### `ParsedRequestUrl`

```cp
pub struct ParsedRequestUrl {
    pub href: string,
    pub path: string,
    pub pathname: string,
    pub search: string,
    pub query: string,
    pub hash: string,
    pub has_query: bool,

    pub static fn empty(): ParsedRequestUrl
}
```

### Functions

- `parseRequestUrl(raw_url: string): ParsedRequestUrl` — Single-pass zero-copy splitter for origin-form and absolute-form HTTP request URLs.
- `decodePathname(pathname: string, out: []u8): usize` — Percent-decodes a pathname into `out`, preserving literal `+`.
- `isSafeRequestPath(pathname: string): bool` — Validates that a pathname has no null bytes, backslashes, or `..` traversal segments (both raw and percent-encoded).
- `getRequestQueryParam(raw_url: string, key: string, out: []u8): usize?` — Extracts and decodes a query parameter by key via `@cap/querystring`.
- `decodeQueryValue(raw: string, out: []u8): usize` — Decodes a query value (`+` -> space and `%XX`) via `@cap/querystring`.

## Example

```cp
import {
    parseRequestUrl,
    decodePathname,
    isSafeRequestPath,
    getRequestQueryParam
} from "@cap/parseurl";

let u = parseRequestUrl("/api/v1/users?page=2&sort=asc#top");
// u.pathname == "/api/v1/users"
// u.search   == "?page=2&sort=asc"
// u.query    == "page=2&sort=asc"
// u.hash     == "#top"
// u.path     == "/api/v1/users?page=2&sort=asc"

let buf: [64]u8 = undefined;
let page_len = getRequestQueryParam(u.href, "page", buf[0..64]);
// page_len == 1, buf[0..1] == "2"
```

## License

MIT
