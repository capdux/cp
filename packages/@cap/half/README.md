# @cap/half

Zero-heap IEEE 754 `binary16` (`f16`) and Google Brain `bfloat16` (`bf16`) bit-exact conversion and inspection codecs in pure CP (inspired by Rust `half`, 599M+ downloads).

Designed for AI/ML tensor quantization, GGUF/SafeTensors/ONNX model weights, and GPU buffer interoperability.

## Install

```bash
cap add @cap/half
```

## Usage

```cp
import { f32BitsToF16Bits, f16BitsToF32Bits, f32BitsToBf16Bits, bf16BitsToF32Bits, F16, Bf16 } from "@cap/half";

// 1.0f32 = 0x3F800000 -> f16 0x3C00 -> bf16 0x3F80
const h = f32BitsToF16Bits(0x3F800000);  // 0x3C00
const b = f32BitsToBf16Bits(0x3F800000); // 0x3F80
```
