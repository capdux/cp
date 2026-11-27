# title-case — AI Agent Quick Ref

Zero-allocation AP/Chicago style English title-case, sentence-case, and capitalization engine in pure CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    isUpper,
    isLower,
    isSmallWord,
    isWordChar,
    isAllCapsAcronym,
    hasMoreWords,
    toTitleCase,
    toSentenceCase,
} from "title-case";
```

## API Signatures

```cp
fn isUpper(b: u8): bool;
fn isLower(b: u8): bool;
fn isSmallWord(word: string): bool;
fn isWordChar(b: u8): bool;
fn isAllCapsAcronym(word: string): bool;
fn hasMoreWords(input: string, start_pos: usize): bool;
fn toTitleCase(input: string, out: []u8): usize;
fn toSentenceCase(input: string, out: []u8): usize;
fn capitalizeFirst(input: string, out: []u8): usize;
```

## Minimal Usage

```cp
import { isSmallWord } from "title-case";

pub fn main() {
    let result = isSmallWord("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"title-case"`; install via `cap add title-case`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
