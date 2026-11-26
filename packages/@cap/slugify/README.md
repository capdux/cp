# @cap/slugify

Zero-allocation URL slug generator with UTF-8 Latin-1 / Latin Extended-A diacritic folding for CP.

## Features

- **Zero heap allocation**: writes directly into caller-supplied `[]u8` buffers.
- **UTF-8 diacritic folding**: maps accented Latin characters (`é/è/ê/ë -> e`, `á/à/â/ä/ã -> a`, `ü/ú/ù/û -> u`, `ö/ó/ò/ô -> o`, `ñ -> n`, `ç -> c`, `ß -> ss`, `æ -> ae`, `œ -> oe`, and Latin Extended-A).
- **Flexible output**: `slugify` (lowercase `-`), `slugifyWithSep` (custom separator & case), `slugifyTruncate` (max length without trailing `-`), and `isValidSlug`.

## Usage

```cp
import { slugify, slugifyTruncate, isValidSlug } from "@cap/slugify";

let buf: [64]u8 = undefined;
const n = slugify("Café Résumé naïve über Straße", buf[0..64]);
// "cafe-resume-naive-uber-strasse"
```
