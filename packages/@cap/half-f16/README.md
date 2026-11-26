# `@cap/half-f16`

Zero-heap IEEE 754 `f16` (binary16) and Google `bf16` (Brain Float 16) codec and arithmetic in pure CP (clean-room counterpart to Rust `half`). Designed for AI/LLM tensor weights, Safetensors, ONNX, and GPU interop.

## Features

- **100% Zero Heap Allocation & Zero Dependencies**: Pure IEEE 754 bit manipulation via `@bitCast`.
- **Full `F16` Support**: `fromBits`, `fromF32` (RTNE rounding, subnormals, overflow to ±Inf, NaN preservation, ±0 sign), `toF32`, `isNan`, `isInfinite`, `isZero`, `neg`, `abs`, `toLeBytes`, `fromLeBytes`.
- **Full `Bf16` Support**: `fromBits`, `fromF32` (RTNE rounding), `toF32`, `isNan`, `isInfinite`, `neg`, `abs`.
- **Batch Tensor Slice Conversion**: `f32SliceToF16` and `f16SliceToF32`.

## Quick Start

```cp
import { F16, Bf16, f32SliceToF16, f16SliceToF32 } from "@cap/half-f16";

const h = F16.fromF32(1.0); // bits == 0x3C00
const b = Bf16.fromF32(1.0); // bits == 0x3F80
```

## License

MIT
