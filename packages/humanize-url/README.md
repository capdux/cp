[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# humanize-url

Zero-allocation, zero-copy URL humanizer for CP — strips protocol (`http://`, `https://`, `//`), leading `www.`, and trailing `/`, matching npm [`humanize-url`](https://github.com/sindresorhus/humanize-url) v3.

## Source Alignment

- **Upstream**: npm `humanize-url` v3 (`sindresorhus/humanize-url`, MIT)
- **CP Package**: `humanize-url` v1.0.0 (clean-room MIT in pure CP)
- **Design**: Pure slice-based zero-copy API (`humanizeUrlSlice`) plus buffer-writing API (`humanizeUrl`) with exact length pre-calculation (`humanizedLen`).

## Installation & Import

```bash
cap add humanize-url
```

```cp
import {
    isProtocolRelative,
    extractScheme,
    stripProtocol,
    stripWww,
    stripTrailingSlash,
    humanizeUrlSlice,
    humanizedLen,
    humanizeUrl,
} from "humanize-url";
```

## Core API

| Function | Signature | Description |
| :--- | :--- | :--- |
| `humanizeUrlSlice` | `fn humanizeUrlSlice(url: string): string` | Zero-copy humanize: trims whitespace, strips `http://`/`https://`/`//`, strips `www.` (preserving bare domains like `www.com`), and strips a single trailing `/` |
| `humanizedLen` | `fn humanizedLen(url: string): usize` | Exact byte length of the humanized URL |
| `humanizeUrl` | `fn humanizeUrl(url: string, out: []u8): usize` | Writes humanized URL into `out` and returns bytes written |
| `isProtocolRelative` | `fn isProtocolRelative(url: string): bool` | Returns `true` if trimmed `url` begins with `//` |
| `extractScheme` | `fn extractScheme(url: string): string` | Returns the scheme slice before `://` (e.g. `"https"`, `"ftp"`), or `""` if absent |
| `stripProtocol` | `fn stripProtocol(url: string): string` | Zero-copy strip of leading `https://`, `http://` (case-insensitive), or `//` |
| `stripWww` | `fn stripWww(host_and_rest: string): string` | Zero-copy strip of case-insensitive leading `www.` only when the remainder still has a dot (keeps `www.com` intact) |
| `stripTrailingSlash` | `fn stripTrailingSlash(url: string): string` | Zero-copy strip of a single trailing `/` if present |

## Examples

### 1. Zero-Copy URL Humanization

```cp
import { humanizeUrlSlice } from "humanize-url";
import { print } from "cap:io";

pub fn main() {
    print(humanizeUrlSlice("https://www.sindresorhus.com/"));       // "sindresorhus.com"
    print(humanizeUrlSlice("http://example.com:8080/docs/?a=1"));   // "example.com:8080/docs/?a=1"
    print(humanizeUrlSlice("//www.cp-lang.org/guide/"));            // "cp-lang.org/guide"
    print(humanizeUrlSlice("https://www.com/"));                    // "www.com"
}
```

### 2. Buffer Output with Exact Length Check

```cp
import { humanizedLen, humanizeUrl } from "humanize-url";

pub fn formatDisplayUrl(raw: string, out: []u8): usize {
    if (humanizedLen(raw) > out.len) {
        return 0;
    }
    return humanizeUrl(raw, out);
}
```

### 3. Composing Low-Level Helpers

```cp
import { extractScheme, isProtocolRelative, stripProtocol, stripWww } from "humanize-url";

pub fn inspectUrl(url: string): string {
    const scheme = extractScheme(url);       // "https" or ""
    const proto_rel = isProtocolRelative(url);
    const no_proto = stripProtocol(url);
    return stripWww(no_proto);
}
```

## Differences from npm `humanize-url` & Safety Highlights

- **Zero-Copy First**: `humanizeUrlSlice` returns a borrowed sub-slice of `url` directly with zero allocations and zero copying.
- **No Exceptions**: Empty or whitespace-only strings return `""` instead of throwing `TypeError`.
- **Preserves Bare `www.` Domains**: `stripWww` checks for a remaining dot in the host, so `https://www.com/` becomes `"www.com"` rather than `"com"`.
