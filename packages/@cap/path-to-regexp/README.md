# @cap/path-to-regexp

Zero-allocation Express/Hono-style URL route pattern matcher, named parameter extractor & reverse URL builder for CP — clean-room MIT parity with npm `path-to-regexp` (65M/wk).

## Install

```bash
cap add @cap/path-to-regexp
```

## Features

- **Named Parameters**: `/users/:id/posts/:postId`
- **Optional Parameters**: `/archive/:year/:month?`
- **Wildcard Catch-All**: `/files/*filepath`
- **Reverse URL Builder**: `buildRoute("/users/:id/posts/:postId", params, out)` → `"/users/42/posts/99"`
- **Query & Trailing Slash Normalization**: automatically ignores `?query` / `#hash` and normalizes trailing `/`.

## Usage

```cp
import { matchRoute, buildRoute, ParamPair } from "@cap/path-to-regexp";

const m = matchRoute("/packages/:scope/:name", "/packages/@cap/lru?v=1");
// m.matched == true, m.get("scope") == "@cap", m.get("name") == "lru"
```
