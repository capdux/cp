# css-select — AI Agent Quick Ref

Zero-heap CSS selector parser, W3C Selectors Level 4 specificity calculator, and compound/list element matcher (clean-room port of css-select + css-what) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    Specificity,
    CssElement,
    csssIsCombinator,
    csssIsIdentStart,
    csssIsIdentChar,
    csssEqMaybeCi,
    csssSkipIdent,
    csssFindBracketEnd,
} from "css-select";
```

## Core Types & Constants

```cp
class Specificity { ids: u32, classes: u32, tags: u32 } // methods: new(...) +2
class CssElement { tag: string, id: string, class_list: string, attr_name: string, /* +1 fields */ } // methods: new(...)
```

## API Signatures

```cp
fn csssIsCombinator(c: u8): bool;
fn csssIsIdentStart(c: u8): bool;
fn csssIsIdentChar(c: u8): bool;
fn csssEqMaybeCi(a: string, b: string, ci: bool): bool;
fn csssSkipIdent(s: string, pos: usize): usize;
fn csssFindBracketEnd(s: string, pos: usize): usize;
fn csssSkipParen(s: string, pos: usize): usize;
fn csssHasToken(list: string, target: string, ci: bool): bool;
fn csssContains(haystack: string, needle: string, ci: bool): bool;
fn csssIsLegacyPseudoElement(name: string): bool;
fn csssComputeSingleSpecificity(s: string, depth: u32): Specificity;
fn csssComputeListSpecificity(selector: string, depth: u32): Specificity;
fn computeSpecificity(selector: string): Specificity;
fn csssMatchAttr(raw_inner: string, el: CssElement): bool;
fn matchesCompoundSelector(selector: string, el: CssElement): bool;
fn matchesSelectorList(selector_list: string, el: CssElement): bool;
fn countSelectorParts(selector: string): usize;
```

## Minimal Usage

```cp
import { csssEqMaybeCi } from "css-select";

pub fn main() {
    let result = csssEqMaybeCi("example", "example", false);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"css-select"`; install via `cap add css-select`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
