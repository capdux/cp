# `@cap/hamming`

Zero-heap-allocation Hamming distance, byte popcount weight, 64-bit Charikar SimHash, and Hamming(7,4) single-error-correcting code in pure CP.

## Features

- **Bitwise Metrics**: `popcountU64`, `hammingU64`, `weightBytes`, and `distanceBytes`.
- **64-bit Charikar SimHash**: Case-insensitive ASCII tokenizing `simhash64` for fast near-duplicate document detection.
- **Hamming(7,4) ECC**: `hamming74Encode` and `hamming74Decode` with automatic 1-bit error detection and correction.
- **Zero Heap Allocation**: Pure CP, stack-only, no FFI.

## API

```cp
import {
    Hamming74Decoded,
    popcountU64,
    hammingU64,
    weightBytes,
    distanceBytes,
    simhash64,
    hamming74Encode,
    hamming74Decode,
} from "@cap/hamming";
```

## License

MIT
