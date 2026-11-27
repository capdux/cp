[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# accept-language-parser

Zero-heap RFC 9110 / BCP-47 `Accept-Language` quality-weight parser and HTTP content-language negotiation engine in pure CP.

Aligned with npm [`accept-language-parser`](https://github.com/opentable/accept-language-parser) and crates.io `accept-language` (MIT).

## Installation & Import

```bash
cap add accept-language-parser
```

```cp
import {
    LangRange,
    parseQuality,
    parseAcceptLanguage,
    matchesTag,
    pickLanguage,
} from "accept-language-parser";
```

## Core API

### Data Structures

| Class | Fields | Description |
| --- | --- | --- |
| `LangRange` | `tag: string`, `primary: string`, `region: string`, `quality: u16` | Parsed BCP-47 language range with zero-copy sub-slices and fixed-point quality weight in `0..=1000` (e.g. `q=0.85` -> `850`). |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `parseQuality` | `(s: string): i32` | Parses RFC 9110 `qvalue` (`0`..`1` with up to 3 decimal digits) into `0..=1000`, or `-1` if invalid. |
| `parseAcceptLanguage` | `(header: string, out: []LangRange): usize` | Parses an `Accept-Language` header into `out`, sorted by descending `quality` (stable tie-breaking); returns number of ranges written. |
| `matchesTag` | `(range_tag: string, supported_tag: string): bool` | RFC 4647 basic prefix match (case-insensitive): `"*"` matches all, `"en"` matches `"en"` and `"en-US"`. |
| `pickLanguage` | `(header: string, supported: &[]string): string` | Negotiates the best supported language for `header` (honoring `q=0` exclusions); returns `""` if no match. |

## Examples

### 1. Negotiating Best Language for an HTTP Request

```cp
import { print } from "cap:io";
import { pickLanguage } from "accept-language-parser";

pub fn main() {
    let header = "zh-CN,zh;q=0.9,en-US;q=0.8,en;q=0.7";
    let supported: [3]string = ["en-US", "zh-Hans", "zh-CN"];
    let best = pickLanguage(header, supported[0..3]);
    print("selected: {}\n", best); // "zh-CN"
}
```

### 2. Parsing & Inspecting Sorted Language Ranges

```cp
import { print } from "cap:io";
import { LangRange, parseAcceptLanguage } from "accept-language-parser";

pub fn main() {
    let ranges: [8]LangRange = undefined;
    let n = parseAcceptLanguage("fr-CA;q=0.7, en-GB;q=0.9, de", ranges[0..8]);
    let i: usize = 0;
    while (i < n) {
        print("{}: primary={} region={} q={}\n",
            ranges[i].tag, ranges[i].primary, ranges[i].region, ranges[i].quality);
        i += 1;
    }
}
```

### 3. Handling Explicit Exclusions (`q=0`)

```cp
import { print } from "cap:io";
import { pickLanguage } from "accept-language-parser";

pub fn main() {
    let supported: [2]string = ["en-US", "fr-FR"];
    // en-US is explicitly excluded with q=0, so fr-FR wins via wildcard
    let chosen = pickLanguage("en-US;q=0, *;q=0.5", supported[0..2]);
    print("chosen: {}\n", chosen); // "fr-FR"
}
```

## Differences from Upstream & Zero-Heap Highlights

- **Zero heap allocation**: All `LangRange` fields (`tag`, `primary`, `region`) are zero-copy sub-slices into `header`, written directly into caller-provided `out: []LangRange`.
- **Integer fixed-point quality**: Uses `u16` in `0..=1000` instead of floating-point `f64`, eliminating float comparison drift while strictly validating RFC 9110 `qvalue` grammar (max 3 fractional digits).
- **RFC 9110 `q=0` exclusion support**: `pickLanguage` properly excludes candidates explicitly rejected with `q=0`, even when a wildcard `*` is present.
