# `@cap/glob-to-regex`

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:glob-to-regexp` ([https://github.com/fitzgen/glob-to-regexp](https://github.com/fitzgen/glob-to-regexp))（原库名 `glob-to-regexp` → CP 包名 `@cap/glob-to-regex`）

Zero-heap-allocation POSIX/glob wildcard to ECMAScript/PCRE regular expression converter in pure CP.

## Features

- **Extended Glob Syntax (`extended: true`)**: `?` -> `.`, `[!a-z]` -> `[^a-z]`, `{ts,cp,zig}` -> `(ts|cp|zig)`.
- **Globstar (`globstar: true`)**: `*` -> `[^/]*`, `/**/` -> `(?:/.+/|/)`, leading `**/` -> `(?:.+/)?`, trailing `**` -> `.*`.
- **Safe Escaping**: Escapes `. + ^ $ ( ) | \` verbatim.
- **Zero Heap Allocation**: Pure CP, stack-only, no FFI.

## API

```cp
import { GlobOptions, globToRegex, globToRegexDefault } from "@cap/glob-to-regex";
```

## License

MIT
