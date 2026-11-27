# pluralize — AI Agent Quick Ref

Zero-allocation English pluralization, singularization, irregular/uncountable dictionary, count formatting & ordinal engine for CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    isVowelLower,
    detectCaseStyle,
    copySlice,
    writeWithCase,
    writeStemAndSuffix,
    inList,
    isUncountable,
    plural,
} from "pluralize";
```

## API Signatures

```cp
fn isVowelLower(c: u8): bool;
fn detectCaseStyle(word: &[]u8): u8;
fn copySlice(out: []u8, src: &[]u8): &[]u8;
fn writeWithCase(out: []u8, src_lower: &[]u8, style: u8): &[]u8;
fn writeStemAndSuffix(out: []u8, stem: &[]u8, suffix_lower: &[]u8, all_upper: bool): &[]u8;
fn inList(lower: &[]u8, list: &[]&[]u8): bool;
fn isUncountable(word: &[]u8): bool;
fn plural(word: &[]u8, out: []u8): &[]u8;
fn singular(word: &[]u8, out: []u8): &[]u8;
fn isPlural(word: &[]u8): bool;
fn isSingular(word: &[]u8): bool;
fn pluralizeCount(word: &[]u8, count: i64, out: []u8): &[]u8;
fn ordinal(n: i64, out: []u8): &[]u8;
```

## Minimal Usage

```cp
import { isVowelLower } from "pluralize";

pub fn main() {
    let result = isVowelLower(0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"pluralize"`; install via `cap add pluralize`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
