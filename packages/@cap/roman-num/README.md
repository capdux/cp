# `@cap/roman-num`

Zero-heap-allocation strict Roman numeral (`1..3999`) bidirectional parser, validator, and formatter in pure CP.

## Features

- **`toRoman`**: Formats `1..3999` to canonical uppercase Roman numerals (`I..MMMCMXCIX`).
- **`parseRoman`**: Case-insensitive parser with canonical round-trip validation rejecting non-standard forms (`IIII`, `VV`, `IL`, `IC`, `XM`).
- **`isValidRoman`**: Strict validation predicate.
- **Zero Heap Allocation**: Pure CP, stack-only, no FFI.

## API

```cp
import { toRoman, parseRoman, isValidRoman } from "@cap/roman-num";
```

## License

MIT
