# `@cap/zbase32`

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:zbase32` ([https://github.com/cp-lang/cp](https://github.com/cp-lang/cp)) / `crates.io:zbase32` ([https://github.com/cp-lang/cp](https://github.com/cp-lang/cp))

Zero-heap-allocation Zooko human-oriented `z-base-32` (`ybndrfg8ejkmcpqxot1uwisza345h769`) encoder, decoder, and validator in pure CP.

## Features

- **`encode` & `decode`**: Unpadded big-endian 5-bit z-base-32 encoding and case-insensitive decoding.
- **`encodedLen`**: Exact encoded length calculation `(byte_len * 8 + 4) / 5`.
- **`isValidZBase32`**: Rejects visually ambiguous characters (`0`, `l`, `v`, `2`) and non-alphabet bytes.
- **Zero Heap Allocation**: Pure CP, stack-only, no FFI.

## API

```cp
import { encode, decode, encodedLen, isValidZBase32 } from "@cap/zbase32";
```

## License

MIT
