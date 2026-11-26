# @cap/isbn

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:isbn` ([https://github.com/cp-lang/cp](https://github.com/cp-lang/cp))

Zero-allocation ISO 2108 ISBN-10, ISBN-13, ISSN, and EAN-13 validator and converter for CP.

## Features

- **Zero heap allocation**: pure CP implementation operating directly on `string` / `[]u8` buffers.
- **Validation**: `isValidIsbn10`, `isValidIsbn13` (978/979 prefixes), `isValidIsbn`, `isValidIssn`, `isValidEan13`.
- **Conversion & Checksums**: `isbn10To13`, `isbn13To10` (978-only per ISO 2108), `computeIsbn13CheckDigit`, and `stripHyphens`.

## Usage

```cp
import { isValidIsbn10, isbn10To13, isbn13To10 } from "@cap/isbn";

const ok = isValidIsbn10("0-306-40615-2"); // true
let out13: [13]u8 = undefined;
const n13 = isbn10To13("0-306-40615-2", out13[0..13]); // "9780306406157"
```
