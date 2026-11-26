# @cap/clsx

Zero-heap conditional CSS class name builder, whitespace normalizer, and deduplicator in pure CP (npm `clsx` / `classnames` parity).

## Features

- `ClassBuilder`: writes normalized single-space-separated class tokens directly into caller `[]u8` buffer
- `add`, `addIf`, `addUnique`, `addConds`, `has` (exact word-boundary check), `build`, `len`, `isOverflow`, `reset`
- Top-level helpers `joinClasses` and `joinCondClasses`

## Usage

```cp
import { ClassBuilder, CondClass } from "@cap/clsx";

let buf: [128]u8 = undefined;
let b = ClassBuilder.init(buf[0..128]);
* = b.add("  btn   btn-lg ");
* = b.addIf(true, "active");
* = b.addUnique("btn text-white");
// b.build() => "btn btn-lg active text-white"
```
