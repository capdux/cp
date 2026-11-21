# `cap:dct` — Zero-Allocation 1D/2D 8×8 DCT-II / IDCT-III, ZigZag & JPEG Quantization DSP Library

Pure CP, zero-heap-allocation Discrete Cosine Transform library providing arbitrary-length orthonormal 1D DCT-II / IDCT-III, separable 2D 8×8 JPEG/video block DCT-II / IDCT-III, standard ZigZag reordering, ITU-T T.81 quantization tables, and IJG quality-factor scaling.

## Constants

- `DCT_BLOCK_DIM = 8`, `DCT_BLOCK_SIZE = 64`
- `JPEG_ZIGZAG_8X8: [64]u8` — Standard JPEG ZigZag scan order
- `JPEG_LUMA_QUANT_50: [64]u8` / `JPEG_CHROMA_QUANT_50: [64]u8` — ITU-T T.81 Table K.1 / K.2 matrices

## Core Functions

| Symbol | Description |
| :--- | :--- |
| `dct1d(input: []const f64, output: *[]f64): bool` | Orthonormal 1D DCT-II (`N >= 1`, Parseval energy-preserving). |
| `idct1d(input: []const f64, output: *[]f64): bool` | Exact orthonormal inverse 1D IDCT-III. |
| `dct8x8(input: [64]f64): [64]f64` | Separable 2D 8×8 DCT-II (constant block `c` -> `DC = 8 * c`, AC = 0). |
| `idct8x8(coeffs: [64]f64): [64]f64` | Exact separable 2D 8×8 IDCT-III. |
| `zigzagReorder8x8(block)` / `unzigzagReorder8x8(zz)` | Bijective 8×8 raster <-> ZigZag scan permutation. |
| `quantize8x8(coeffs, qtable)` / `dequantize8x8(quant, qtable)` | Round-to-nearest quantization and dequantization (`q == 0` guarded as `1`). |
| `scaleQuantTable(base, quality: u8): [64]u8` | IJG quality factor (`1..100`) scaling (`quality = 50` returns `base`). |

## Example

```cp
import {
    JPEG_LUMA_QUANT_50,
    dct8x8,
    idct8x8,
    quantize8x8,
    dequantize8x8,
    scaleQuantTable,
} from "cap:dct";

fn compressBlock(pixels: [64]f64): [64]f64 {
    const qt = scaleQuantTable(JPEG_LUMA_QUANT_50, 75);
    const freq = dct8x8(pixels);
    const q = quantize8x8(freq, qt);
    return idct8x8(dequantize8x8(q, qt));
}
```
