# decamelize — AI Agent Quick Reference

Zero-heap ASCII `camelCase` / `PascalCase` to separated lowercase / `CONSTANT_CASE` converter (npm `decamelize` parity).

## Import

```cp
import {
    DecamelizeOptions,
    decamelizedLen,
    decamelizeWithOptions,
    decamelizeOpts,
    decamelize,
    decamelizeDefault,
    toSnakeCase,
    toKebabCase,
    toSpaceCase,
    toConstantCase,
} from "decamelize";
```

## API Signatures

```cp
struct DecamelizeOptions {
    separator: string,
    preserve_consecutive_uppercase: bool,
    static fn defaults(): DecamelizeOptions;
}

fn decamelizedLen(input: string, separator: string, preserve_consecutive_uppercase: bool): usize;
fn decamelizeWithOptions(input: string, separator: string, preserve_consecutive_uppercase: bool, out: []u8): usize;
fn decamelizeOpts(input: string, opts: DecamelizeOptions, out: []u8): usize;
fn decamelize(input: string, separator: string, out: []u8): usize;
fn decamelizeDefault(input: string, out: []u8): usize;
fn toSnakeCase(input: string, out: []u8): usize;
fn toKebabCase(input: string, out: []u8): usize;
fn toSpaceCase(input: string, out: []u8): usize;
fn toConstantCase(input: string, out: []u8): usize;
```

## Minimal Example

```cp
import { decamelize, decamelizeWithOptions, toKebabCase } from "decamelize";

let buf: [64]u8 = [0; 64];
const n1 = decamelize("unicornRainbow", "_", buf[0..64]);             // "unicorn_rainbow"
const n2 = decamelizeWithOptions("testURL", "_", true, buf[0..64]);   // "test_URL"
const n3 = toKebabCase("fooBarBaz", buf[0..64]);                      // "foo-bar-baz"
```

## Gotchas & Borrow Rules

- Output may be longer than `input` because separators are inserted at word boundaries; check `decamelizedLen(input, sep, preserve) <= out.len` to avoid silent truncation.
- `input` and `separator` are read-only `string` borrows; `out: []u8` is caller-owned mutable output.
- Set `preserve_consecutive_uppercase = true` to keep runs of `>= 2` uppercase ASCII letters intact (e.g. `"testURL"` -> `"test_URL"`).
- Operates on ASCII letters/digits (`A-Z`, `a-z`, `0-9`); non-ASCII UTF-8 bytes are copied through unchanged.
