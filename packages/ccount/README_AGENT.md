# ccount — AI Agent Quick Ref

Zero-allocation character/substring occurrence counter and longest consecutive streak finder (`v2.0.1`, `MIT`)

## Import

```cp
import { ccountByte, ccount, longestStreakByte, longestStreak } from "ccount";
```

## API Signatures

```cp
fn ccountByte(value: string, ch: u8): usize;
fn ccount(value: string, needle: string): usize;
fn longestStreakByte(value: string, ch: u8): usize;
fn longestStreak(value: string, needle: string): usize;
```

## Minimal Usage

```cp
import { ccountByte } from "ccount";

pub fn main() {
    let result = ccountByte("example", 0);
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"ccount"`; install via `cap add ccount`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.

## Quick Verification

```bash
cap add ccount
cpc check src/main.cp
```
