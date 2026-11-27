# class-variance-authority — AI Agent Quick Ref

Zero-heap CSS variant class composer for CP (clean-room Apache-2.0 implementation of npm class-variance-authority / cva). (`v1.0.0`, `Apache-2.0`)

## Import

```cp
import {
    VariantRule,
    VariantSelection,
    CompoundRule,
    cvaStrEq,
    cvaIsSpace,
    cvaAppendClasses,
    cvaResolveOption,
    cvaMatchesCond,
} from "class-variance-authority";
```

## Core Types & Constants

```cp
class VariantRule { group: string, option: string, classes: string }
class VariantSelection { group: string, option: string }
class CompoundRule { group1: string, option1: string, group2: string, option2: string, /* +3 fields */ } // methods: pair(...) +1
```

## API Signatures

```cp
fn cvaStrEq(a: string, b: string): bool;
fn cvaIsSpace(ch: u8): bool;
fn cvaAppendClasses(out: []u8, pos: usize, src: string): usize;
fn cvaResolveOption(group: string, defaults: &[]VariantSelection, selected: &[]VariantSelection): string?;
fn cvaMatchesCond(group: string, option: string, defaults: &[]VariantSelection, selected: &[]VariantSelection): bool;
fn cvaCompose(base: string, rules: &[]VariantRule, defaults: &[]VariantSelection, selected: &[]VariantSelection, extra: string, out: []u8): usize;
fn cvaComposeWithCompounds(base: string, rules: &[]VariantRule, compounds: &[]CompoundRule, defaults: &[]VariantSelection, selected: &[]VariantSelection, extra: string, out: []u8): usize;
```

## Minimal Usage

```cp
import { cvaStrEq } from "class-variance-authority";

pub fn main() {
    let result = cvaStrEq("example", "example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"class-variance-authority"`; install via `cap add class-variance-authority`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.
