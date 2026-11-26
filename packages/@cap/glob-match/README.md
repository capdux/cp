# @cap/glob-match

Zero-allocation POSIX and `.gitignore` glob path matching engine for CP with ReDoS-safe bounded backtracking.

## Features

- **Zero heap allocation**: non-recursive dual-backtrack matcher operating directly on `string` slices.
- **Full glob syntax**: single-segment `*` (never crosses `/`), cross-directory `**` (`src/**/*.cp`, `**/*.md`, `a/**/b`), single-char `?`, character classes (`[abc]`, `[a-z]`, `[!0-9]`, `[^0-9]`), and `\x` escapes.
- **`.gitignore` engine**: `isIgnored` evaluates rules sequentially with `!` negation overrides, `#` comment skipping, and unanchored basename matching.
- **Utilities**: `isMatchAny` and `hasGlobMagic`.

## Usage

```cp
import { isMatch, isIgnored } from "@cap/glob-match";

const ok = isMatch("src/**/*.cp", "src/net/http/server.cp"); // true
```
