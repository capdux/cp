# `@cap/pad-str`

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:pad` ([https://github.com/adaltas/node-pad](https://github.com/adaltas/node-pad)) / `crates.io:pad` ([https://github.com/ogham/rust-pad](https://github.com/ogham/rust-pad)) / `npm:left-pad` ([https://github.com/cp-lang/cp](https://github.com/cp-lang/cp))（原库名 `pad` → CP 包名 `@cap/pad-str`）

Zero-heap-allocation UTF-8 codepoint-aware string padding (`padStart`, `padEnd`, `padCenter`) and `repeatStr` in pure CP.

## Features

- **`countChars`**: Counts UTF-8 Unicode scalar codepoints in zero memory.
- **`padStart` / `padEnd` / `padCenter`**: Aligns strings by UTF-8 codepoint count (not raw byte length), placing odd remainder on the right for `padCenter`.
- **`repeatStr`**: Overflow-safe zero-heap string repetition into caller buffers.
- **Zero Heap Allocation**: Pure CP, stack-only, no FFI.

## API

```cp
import { countChars, padStart, padEnd, padCenter, repeatStr } from "@cap/pad-str";
```

## License

MIT
