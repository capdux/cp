# @cap/accept-lang

Zero-heap RFC 9110 / BCP-47 `Accept-Language` quality weight parser and best-language negotiation engine in pure CP.

## Features

- `parseQuality(s)`: strict RFC 9110 `0..1` (up to 3 decimals) parser returning `0..1000` (or `-1`)
- `parseAcceptLanguage(header, out)`: zero-copy comma-list parsing, primary/region subtag extraction, and stable descending quality sort
- `matchesTag(range_tag, supported_tag)`: case-insensitive exact, primary prefix (`zh` -> `zh-CN`), and wildcard `*` matching
- `pickLanguage(header, supported)`: exact-before-prefix negotiation with `q=0` exclusion and `*` fallback

## Usage

```cp
import { pickLanguage } from "@cap/accept-lang";

const supported: [2]string = ["en-US", "zh-CN"];
const best = pickLanguage("zh-CN,zh;q=0.9,en;q=0.8", supported[0..2]); // "zh-CN"
```
