# `@cap/wyhash`

Zero-heap-allocation Wang Yi `wyhash` v4 64-bit fast non-cryptographic hash and `WyRng` (`wyrand`) PRNG in pure CP.

## Features

- **`wymum`**: Overflow-free 128-bit multiply-and-XOR mix via 32-bit splitting.
- **`wyhash` & `wyhashBytes`**: Standard wyhash v4 handling `0..3`, `4..16`, `17..48`, and `>48`-byte blocks with official default secrets.
- **`WyRng`**: Official `wyrand` PRNG with `new`, `nextU64`, `nextU32`, and `nextF64` (`[0.0, 1.0)`).
- **Zero Heap Allocation**: Pure CP, stack-only, no FFI.

## API

```cp
import { wymum, wyhash, wyhashBytes, WyRng } from "@cap/wyhash";
```

## License

MIT
