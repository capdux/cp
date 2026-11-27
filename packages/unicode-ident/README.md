# `@cap/unicode-ident`

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:unicode-ident` ([https://github.com/dtolnay/unicode-ident](https://github.com/dtolnay/unicode-ident))

Zero-heap UAX #31 Unicode identifier classification and UTF-8 identifier scanning library in pure CP (clean-room counterpart to dtolnay's Rust `unicode-ident`).

## Features

- **100% Zero Heap Allocation**: Pure value-type table/range classification with fast ASCII short-circuit paths.
- **UAX #31 Core Predicates**: `isXidStart(cp)`, `isXidContinue(cp)`, and `isIdentStart(cp)` covering ASCII, Latin-1/Extended, Greek, Cyrillic, Armenian, Hebrew, Arabic, Devanagari, Thai, Georgian, CJK Unified Ideographs (+ Extensions A–G), Hiragana, Katakana, and Hangul.
- **Strict UTF-8 Identifier Validation & Lexing**: `isValidIdent(s)` and `scanIdentLen(s)` with strict UTF-8 decoding (rejecting overlong sequences, UTF-16 surrogates, truncated sequences, and codepoints above `U+10FFFF`).

## Quick Start

```cp
import { isXidStart, isXidContinue, isIdentStart, isValidIdent, scanIdentLen } from "@cap/unicode-ident";

const ok1 = isValidIdent("_hello_世界_123"); // true
const ok2 = isValidIdent("9invalid");        // false
const n = scanIdentLen("αβγ_1 + 2");        // 8 bytes ("αβγ_1")
```

## License

MIT OR Apache-2.0
