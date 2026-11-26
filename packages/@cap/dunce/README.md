# `@cap/dunce`

Zero-heap safe Windows verbatim UNC (`\\?\`) path simplifier in pure CP (clean-room counterpart to Rust `dunce`).

## Features

- **100% Zero Heap Allocation**: Evaluates Windows path safety in-place and writes simplified paths directly into `out: []u8`.
- **Full Rust `dunce` Safety Rules**:
  - `isWindowsReservedName`: Detects `CON`, `PRN`, `AUX`, `NUL`, `COM1`..`COM9`, `LPT1`..`LPT9` (case-insensitive, including extensions like `CON.txt`).
  - `canStripVerbatimPrefix`: Verifies `\\?\X:\` drive prefix, stripped length `<= 259`, no `/`/`.`/`..`, no illegal chars `< > : " | ? *`, no reserved names, and no trailing space/dot on components.
  - `simplified`: Strips `\\?\` only when guaranteed safe; otherwise preserves the original path verbatim.

## Quick Start

```cp
import { simplified, canStripVerbatimPrefix, isWindowsReservedName } from "@cap/dunce";

let buf: [260]u8 = undefined;
const n1 = simplified("\\\\?\\C:\\Users\\alice\\file.txt", &buf); // "C:\Users\alice\file.txt"
const n2 = simplified("\\\\?\\C:\\tmp\\CON.txt", &buf);           // kept verbatim
```

## License

MIT
