# @cap/num-iter

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:num-iter` ([https://github.com/rust-num/num-iter](https://github.com/rust-num/num-iter))

Clean-room MIT, zero-heap stepped numeric range iterators for CP — modeled on crates.io #140 `num-iter` v0.1 (850M+ downloads) and reusing `@cap/num-integer` (`divCeil`, `divFloor`).

## Features

- **Zero heap allocation**: stack-only iterator structs with caller-buffer collection (`collectRangeStep`).
- **Signed & unsigned stepped ranges**: supports positive and negative steps for `i64` (`RangeStep`, `RangeStepInclusive`) and positive steps for `u64` (`RangeStepU64`).
- **Overflow-safe boundaries**: safely terminates when advancing across `i64::MAX`, `i64::MIN`, or `u64::MAX` without trapping.
- **O(1) `remaining()` & `nth(n)`**: computes exact remaining element count via `@cap/num-integer` `divCeil` / `divFloor` and skips directly in constant time.

## API

### Types
- `RangeStep`: half-open `[start, stop)` signed iterator (`next()`, `remaining()`, `isDone()`, `nth(n)`)
- `RangeStepInclusive`: closed `[start, stop]` signed iterator (`next()`, `remaining()`, `isDone()`)
- `RangeStepU64`: half-open `[start, stop)` unsigned iterator (`next()`, `remaining()`, `isDone()`, `nth(n)`)

### Functions
- `rangeStep(start: i64, stop: i64, step: i64): RangeStep`
- `rangeStepInclusive(start: i64, stop: i64, step: i64): RangeStepInclusive`
- `rangeStepU64(start: u64, stop: u64, step: u64): RangeStepU64`
- `collectRangeStep(r: *RangeStep, out: []i64): usize`

## Usage

```typescript
import { rangeStep, rangeStepInclusive, collectRangeStep } from "@cap/num-iter";

let r = rangeStep(0, 10, 3); // yields 0, 3, 6, 9
let buf: [8]i64 = [0, 0, 0, 0, 0, 0, 0, 0];
let n = collectRangeStep(&r, buf[0..8]); // n == 4

let inc = rangeStepInclusive(10, 0, -5); // yields 10, 5, 0
```

## License

MIT
