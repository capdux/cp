# humanize-string — AI Agent Quick Ref

Zero-allocation `camelCase`/`snake_case`/`kebab-case` to humanized sentence converter composing `decamelize` (npm `humanize-string` v3 parity).

## Import

```cp
import {
    HumanizeOptions,
    humanizeString,
    humanizeStringWithOptions,
    titleizeString,
    humanizeLength,
} from "humanize-string";
```

## API Signatures

```cp
export class HumanizeOptions {
    capitalize_first: bool,
    preserve_acronyms: bool,
    static fn defaults(): HumanizeOptions;
}
export fn humanizeString(input: string, out: []u8): usize;
export fn humanizeStringWithOptions(input: string, opts: HumanizeOptions, out: []u8): usize;
export fn titleizeString(input: string, out: []u8): usize;
export fn humanizeLength(input: string): usize;
```

## Minimal Example

```cp
import { HumanizeOptions, humanizeString, humanizeStringWithOptions } from "humanize-string";

fn demo(): void {
    let mut buf: [64]u8 = [0; 64];
    const n = humanizeString("fooBar_baz", buf[0..]); // "Foo bar baz"
    let mut opts = HumanizeOptions.defaults();
    opts.preserve_acronyms = true;
    const m = humanizeStringWithOptions("parseURL", opts, buf[0..]); // "Parse URL"
}
```

## Caveats & Borrowing Rules

- **Output buffer sizing**: Camel-case splitting inserts spaces, so output can be longer than `input.len`; call `humanizeLength(input)` or allocate a buffer with headroom (e.g. `2 * input.len`).
- **Mutable slice**: `out: []u8` must be a mutable buffer slice (`let mut buf: [N]u8 = [0; N]; buf[0..]`); output is truncated safely at `out.len` if too small.
- **Zero-copy input**: `input: string` is borrowed read-only; leading/trailing separators (`_`, `-`, whitespace) are collapsed cleanly.
- **Defaults**: `HumanizeOptions.defaults()` sets `capitalize_first: true`, `preserve_acronyms: false`.
