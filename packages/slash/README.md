# @cap/slash

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:slash` ([https://github.com/sindresorhus/slash](https://github.com/sindresorhus/slash))

Zero-allocation Windows backslash <-> POSIX forward-slash path converter for CP (npm `slash` parity).

## Features

- **Zero heap allocation**: writes directly into caller-supplied `[]u8` buffers.
- **Extended-length path safe**: preserves Windows `\\?\` paths verbatim per npm `slash` spec.
- **Bidirectional & inspection**: `slash`, `toBackslash`, `isExtendedLengthPath`, and `countSeparators`.

## Usage

```cp
import { slash } from "@cap/slash";

let buf: [64]u8 = undefined;
const n = slash("foo\\bar\\baz.cp", buf[0..64]); // "foo/bar/baz.cp"
```
