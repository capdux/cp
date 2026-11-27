[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：吸收自 **npm** [`slugify`](https://github.com/simov/slugify)（`MIT`）（包名 `slugify`）。

# slugify

Zero-allocation URL slug generator with UTF-8 Latin-1 / Latin Extended-A diacritic folding for CP.

## Features

- **Zero heap allocation**: writes directly into caller-supplied `[]u8` buffers.
- **UTF-8 diacritic folding**: maps accented Latin characters (`é/è/ê/ë -> e`, `á/à/â/ä/ã -> a`, `ü/ú/ù/û -> u`, `ö/ó/ò/ô -> o`, `ñ -> n`, `ç -> c`, `ß -> ss`, `æ -> ae`, `œ -> oe`, and Latin Extended-A).
- **Flexible output**: `slugify` (lowercase `-`), `slugifyWithSep` (custom separator & case), `slugifyTruncate` (max length without trailing `-`), and `isValidSlug`.

## Usage

```cp
import { slugify, slugifyTruncate, isValidSlug } from "slugify";

let buf: [64]u8 = undefined;
const n = slugify("Café Résumé naïve über Straße", buf[0..64]);
// "cafe-resume-naive-uber-strasse"
```
