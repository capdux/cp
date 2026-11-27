# slugify — AI Agent Quick Ref

Zero-allocation URL slug generator with UTF-8 Latin-1 / Latin Extended-A diacritic folding and truncation for CP. (`v1.0.0`, `MIT`)

## Import

```cp
import {
    slToLower,
    slIsAsciiAlnum,
    slFoldLatin,
    slugifyWithSep,
    slugify,
    slugifyTruncate,
    isValidSlug,
} from "slugify";
```

## API Signatures

```cp
fn slToLower(c: u8): u8;
fn slIsAsciiAlnum(c: u8): bool;
fn slFoldLatin(cp: u32, lowercase: bool, buf: []u8): usize;
fn slugifyWithSep(input: string, sep: u8, lowercase: bool, out: []u8): usize;
fn slugify(input: string, out: []u8): usize;
fn slugifyTruncate(input: string, max_len: usize, out: []u8): usize;
fn isValidSlug(s: string): bool;
```

## Minimal Usage

```cp
import { isValidSlug } from "slugify";

pub fn main() {
    let result = isValidSlug("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"slugify"`; install via `cap add slugify`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
