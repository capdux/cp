# is-extglob — AI Agent Quick Reference

Zero-allocation Bash/Micromatch extglob (`@(...)`, `!(...)`, `*(...)`, `+(...)`, `?(...)`) detector and span finder.

## Import

```cp
import {
    ExtglobKind,
    ExtglobSpan,
    isExtglobPrefix,
    extglobKindFromByte,
    extglobPrefixByte,
    matchExtglobEnd,
    findExtglobFrom,
    findExtglob,
    findExtglobStart,
    isExtglob,
    hasNestedExtglob,
    isNegatedExtglob,
    countExtglobs,
} from "is-extglob";
```

## API Signatures

```cp
enum ExtglobKind {
    Optional,   // ?(...)
    ZeroOrMore, // *(...)
    OneOrMore,  // +(...)
    ExactlyOne, // @(...)
    Negated,    // !(...)
}

struct ExtglobSpan {
    kind: ExtglobKind,
    start: usize,
    end: usize, // exclusive end index
}

fn isExtglobPrefix(ch: u8): bool;
fn extglobKindFromByte(ch: u8): ExtglobKind?;
fn extglobPrefixByte(kind: ExtglobKind): u8;
fn matchExtglobEnd(str: string, start: usize): usize;
fn findExtglobFrom(str: string, start_pos: usize): ExtglobSpan?;
fn findExtglob(str: string): ExtglobSpan?;
fn findExtglobStart(str: string): i64;
fn isExtglob(str: string): bool;
fn hasNestedExtglob(str: string): bool;
fn isNegatedExtglob(str: string): bool;
fn countExtglobs(str: string): usize;
```

## Minimal Example

```cp
import { isExtglob, findExtglob } from "is-extglob";

const ok = isExtglob("src/!(test|bench)/*.cp"); // true
if (const span = findExtglob("src/@(a|b).cp")) {
    const slice = "src/@(a|b).cp"[span.start..span.end]; // "@(a|b)"
}
```

## Gotchas & Borrow Rules

- Backslash-escaped prefixes (e.g. `\@(a|b)`) and unclosed parentheses (e.g. `@(a|b`) are not matched as extglobs.
- `ExtglobSpan.end` is exclusive, so slice with `str[span.start..span.end]`; advance loops with `pos = span.end`.
- `matchExtglobEnd` returns `0` when unmatched; `findExtglobStart` returns `-1` (`i64`) when not found.
- All functions take read-only `string` borrows and perform zero heap allocations.
