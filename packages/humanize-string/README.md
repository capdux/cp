[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# humanize-string

Zero-allocation `camelCase` / `PascalCase` / `snake_case` / `kebab-case` to humanized sentence converter in pure CP, composing `decamelize`.

- **Upstream Alignment**: `npm` `humanize-string` `v3` (MIT, sindresorhus/humanize-string)
- **Package Version**: `1.0.0`
- **Memory Model**: Zero heap allocation — writes normalized output directly into caller-owned `[]u8` buffers.

## Installation & Import

```bash
cap add humanize-string
```

```cp
import {
    HumanizeOptions,
    humanizeString,
    humanizeStringWithOptions,
    titleizeString,
    humanizeLength,
} from "humanize-string";
```

## Core API

| Symbol | Signature | Description |
| --- | --- | --- |
| `HumanizeOptions` | `class HumanizeOptions { capitalize_first: bool, preserve_acronyms: bool }` | Conversion options. `HumanizeOptions.defaults()` enables first-letter capitalization (`true`) and standard lowercasing (`preserve_acronyms: false`). |
| `humanizeString` | `fn humanizeString(input: string, out: []u8): usize` | Converts `input` into a humanized sentence (capitalized first letter, single spaces) into `out`; returns bytes written. |
| `humanizeStringWithOptions` | `fn humanizeStringWithOptions(input: string, opts: HumanizeOptions, out: []u8): usize` | Converts `input` according to `opts` into `out`; returns bytes written. |
| `titleizeString` | `fn titleizeString(input: string, out: []u8): usize` | Humanizes `input` and capitalizes the first letter of every word into `out`; returns bytes written. |
| `humanizeLength` | `fn humanizeLength(input: string): usize` | Computes the exact output byte length of `humanizeString(input, ...)` without writing to a buffer. |

## Examples

### 1. Humanize Identifiers & Title Case

```cp
import { humanizeString, titleizeString } from "humanize-string";

fn basicDemo(): void {
    let mut buf: [64]u8 = [0; 64];
    const n1 = humanizeString("fooBar_baz-qux", buf[0..]);
    // buf[0..n1] == "Foo bar baz qux"

    const n2 = titleizeString("http_status_code", buf[0..]);
    // buf[0..n2] == "Http Status Code"
}
```

### 2. Custom Options (Preserve Acronyms / Lowercase First)

```cp
import { HumanizeOptions, humanizeStringWithOptions, humanizeLength } from "humanize-string";

fn optionsDemo(): void {
    const src = "parseHTMLDocument";
    const needed = humanizeLength(src);

    let mut opts = HumanizeOptions.defaults();
    opts.preserve_acronyms = true;
    opts.capitalize_first = true;

    let mut buf: [64]u8 = [0; 64];
    const n = humanizeStringWithOptions(src, opts, buf[0..]);
    // buf[0..n] == "Parse HTML document"
}
```

## Differences from Upstream & Highlights

- **Caller-Owned Buffers**: Writes directly into `out: []u8` instead of allocating intermediate JS strings and regex arrays; `humanizeLength` allows exact buffer pre-sizing.
- **Extended Controls**: Adds `HumanizeOptions` (`capitalize_first`, `preserve_acronyms`) and `titleizeString` while maintaining `npm humanize-string` v3 behavior for `humanizeString`.
- **Composed with `decamelize`**: Reuses `decamelize` for consistent camel-case boundary splitting.
