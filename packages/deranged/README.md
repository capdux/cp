[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# deranged

Zero-heap ranged bounded integer types (`RangedU8`, `RangedU16`, `RangedU32`, `RangedU64`, `RangedI8`, `RangedI16`, `RangedI32`, `RangedI64`) with checked, saturating, wrapping, expand, and narrow operations in pure CP.

## Source & Zero-Cost Migration

| Field | Value |
| --- | --- |
| **Upstream Registry** | [crates.io/crates/deranged](https://crates.io/crates/deranged) |
| **Upstream Package** | `deranged` (v1.0.0 in CP) |
| **Upstream Repository** | https://github.com/jhpratt/deranged |
| **License** | MIT |

Keep the exact package name when migrating from Rust to CP:

```rust
// Rust
use deranged::{RangedU8, RangedI32};
```

```cp
// CP — same package name, runtime-parametric bounds with zero heap allocation
import { RangedU8, RangedI32, RangedI64 } from "deranged";
```

## Installation

```bash
cap add deranged
```

## Quick Start & Practical Examples

### 1. Construction, Validation & Saturating Clamping

```cp
import { RangedU8, RangedU16, RangedI32 } from "deranged";

fn demoBounds() {
    // Month in [1, 12]
    let month = RangedU8.new(5, 1, 12);
    if (month.isValid()) {
        let v = month.get(); // 5
        let lo = month.min(); // 1
        let hi = month.max(); // 12
    }

    // Out-of-bounds construction marks ok = false without panicking
    let invalid = RangedU8.new(13, 1, 12);
    // invalid.isValid() == false

    // Saturating construction clamps value into [min, max]
    let clamped = RangedU16.newSaturating(7000, 1024, 6553);
    // clamped.isValid() == true, clamped.get() == 6553
}
```

### 2. Checked, Saturating & Wrapping Arithmetic

```cp
import { RangedI32, RangedI64, RangedU32 } from "deranged";

fn demoArithmetic() {
    let hour = RangedI32.new(22, 0, 23);

    // Checked addition: returns invalid RangedI32 if result leaves [0, 23]
    let overflow = hour.checkedAdd(5);
    // overflow.isValid() == false

    // Saturating addition: clamps to max (23)
    let sat = hour.saturatingAdd(5);
    // sat.get() == 23

    // Wrapping addition (available on RangedI32 & RangedI64): wraps modulo range size
    let wrapped = hour.wrappingAdd(5);
    // wrapped.get() == 3

    // Unsigned ranged subtraction with underflow protection
    let pct = RangedU32.new(10, 0, 100);
    let safe_sub = pct.saturatingSub(50); // 0
    let bad_sub = pct.checkedSub(50);     // isValid() == false
}
```

### 3. Expanding & Narrowing Bounds (`expand` / `narrow`)

```cp
import { RangedI8, RangedU64 } from "deranged";

fn demoRangeTransform() {
    let temp = RangedI8.new(25, 0, 50);

    // Expand bounds to a wider range (must cover original [min, max])
    let wider = temp.expand(-40, 100);
    // wider.isValid() == true, wider.get() == 25

    // Narrow bounds to a tighter range (fails if current value is outside new bounds)
    let tighter = temp.narrow(20, 30);
    // tighter.isValid() == true

    let fail_narrow = temp.narrow(30, 40);
    // fail_narrow.isValid() == false
}
```

## Performance & Safety Highlights

- **Zero Heap & Stack-Resident**: Every `Ranged*` type is a compact value struct (`val`, `min_v`, `max_v`, `ok`) passed on the stack with zero dynamic allocation.
- **No-Panic Overflow Protection**: Checked and saturating arithmetic guard against both range violations and native integer overflow/underflow.
- **Complete Width Coverage**: Supports all 8 signed/unsigned integer widths (`U8`, `U16`, `U32`, `U64`, `I8`, `I16`, `I32`, `I64`), ideal for calendar/time crates, protocol fields, and percentage clamps.

## API Reference

See [`src/index.d.cp`](src/index.d.cp) for complete type signatures, or [`README_AGENT.md`](README_AGENT.md) for the compact AI-agent reference.
