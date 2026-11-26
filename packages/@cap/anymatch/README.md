# `@cap/anymatch`

Zero-heap file path & glob matcher with `!pattern` negation support in pure CP (clean-room counterpart to npm `anymatch`). Directly composes `@cap/normalize-path` and `@cap/is-glob`.

## Features

- **Real Cross-Package Composition**: Reuses `@cap/normalize-path` (`normalizePathDefault`) and `@cap/is-glob` (`isGlobDefault`) without duplicating logic.
- **100% Zero Heap Allocation**: Stack-normalized paths and zero-heap `*` / `**` / `?` glob matching.
- **Full `anymatch` API**: `matchPattern`, `anymatch`, `anymatchIndex`, and `countGlobPatterns`.

## Quick Start

```cp
import { anymatch, anymatchIndex, countGlobPatterns } from "@cap/anymatch";

const rules: [3]string = ["src/**/*.cp", "README.md", "!src/generated/**"];
const matched = anymatch(&rules, "src\\net\\http.cp"); // true
const excluded = anymatch(&rules, "src/generated/a.cp"); // false
const idx = anymatchIndex(&rules, "README.md"); // 1
```

## License

MIT
