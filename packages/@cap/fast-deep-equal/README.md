# `@cap/fast-deep-equal`

Zero-heap semantic JSON deep equality checker and root value classifier in pure CP (clean-room counterpart to npm `fast-deep-equal` & `kind-of` / `is-plain-obj`).

## Features

- **100% Zero Heap Allocation**: Direct recursive comparison over JSON slices with bounded depth safety.
- **Full Semantic Equality**:
  - Ignores whitespace differences.
  - Numeric equivalence (`1.0 == 1`, `1e2 == 100`, `-0 == 0`).
  - Escape-aware string comparison (`\"`, `\\`, `\n`, `\uXXXX`).
  - Order-sensitive deep array comparison.
  - **Order-independent deep object comparison** (`{"a":1,"b":2} == {"b":2,"a":1}`).
- **Type Classification**: `kindOfJson` (`KIND_INVALID`..`KIND_OBJECT`) and `isPlainObjectJson`.

## Quick Start

```cp
import { jsonDeepEqual, kindOfJson, isPlainObjectJson } from "@cap/fast-deep-equal";

const eq = jsonDeepEqual("{\"a\":1.0,\"b\":[2,3]}", "{ \"b\": [2, 3], \"a\": 1 }"); // true
const is_obj = isPlainObjectJson("{\"k\":true}"); // true
```

## License

MIT
