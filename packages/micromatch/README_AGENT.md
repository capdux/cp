# micromatch — AI Agent Quick Ref

Zero-allocation glob matching suite (npm micromatch v4 parity) with brace range expansion, multi-pattern filtering, negation, and contains matching for CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    mmHasBrace,
    mmCountLeadingNeg,
    mmMatchPositiveExpanded,
    mmWriteWrapped,
    mmContainsSinglePositive,
    mmIsMatch,
    mmAll,
    mmAny,
} from "micromatch";
```

## API Signatures

```cp
fn mmHasBrace(s: string): bool;
fn mmCountLeadingNeg(pat: string): usize;
fn mmMatchPositiveExpanded(path: string, inner: string, opts: PicoOptions): bool;
fn mmWriteWrapped(dst: []u8, prefix: string, mid: string, suffix: string): usize;
fn mmContainsSinglePositive(path: string, pat: string, opts: PicoOptions): bool;
fn mmIsMatch(path: string, pattern: string, opts: PicoOptions): bool;
fn mmAll(path: string, patterns: &[]string, opts: PicoOptions): bool;
fn mmAny(path: string, patterns: &[]string, opts: PicoOptions): bool;
fn mmSome(paths: &[]string, patterns: &[]string, opts: PicoOptions): bool;
fn mmEvery(paths: &[]string, patterns: &[]string, opts: PicoOptions): bool;
fn mmFilter(paths: &[]string, patterns: &[]string, opts: PicoOptions, out_indices: []usize): usize;
fn mmNot(paths: &[]string, patterns: &[]string, opts: PicoOptions, out_indices: []usize): usize;
fn mmContains(path: string, pattern: string, opts: PicoOptions): bool;
```

## Minimal Usage

```cp
import { mmHasBrace } from "micromatch";

pub fn main() {
    let result = mmHasBrace("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"micromatch"`; install via `cap add micromatch`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
