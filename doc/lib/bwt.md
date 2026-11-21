# `cap:bwt` — Zero-Allocation Burrows-Wheeler Transform & Move-To-Front Frontend

Pure CP, zero-heap-allocation compression frontend combining block-sorting Burrows-Wheeler Transform (BWT), O(n) bucket-counting LF-Mapping inverse transform, and 256-byte Move-To-Front (MTF) coding.

## Core Types & Functions

| Symbol | Description |
| :--- | :--- |
| `BwtResult` | `{ primary_index: usize, len: usize }` returned by `bwtEncode` and `bwtMtfEncode`. |
| `bwtEncode(src, dst: *[]u8, scratch_idx: *[]u32): BwtResult?` | In-place heapsort BWT forward transform (zero recursion, zero heap allocation). |
| `bwtDecode(src, primary_index, dst: *[]u8, scratch_lf: *[]u32): usize?` | O(n) bucket-counting + LF-Mapping inverse transform; validates `primary_index < src.len`. |
| `mtfEncode(src, dst: *[]u8): usize?` | Move-To-Front encoder using a 256-byte stack table. |
| `mtfDecode(src, dst: *[]u8): usize?` | Move-To-Front inverse decoder using a 256-byte stack table. |
| `bwtMtfEncode(src, dst, tmp_bwt, scratch_idx): BwtResult?` | Combined BWT -> MTF forward pipeline. |
| `bwtMtfDecode(src, primary_index, dst, tmp_bwt, scratch_lf): usize?` | Combined MTF -> BWT inverse pipeline. |
| `zeroByteRatioPermille(data: []const u8): u32` | Measures zero-byte clustering ratio in permille (`0..1000`). |

## Example

```cp
import { bwtMtfEncode, bwtMtfDecode, zeroByteRatioPermille } from "cap:bwt";

fn compressFrontend(
    src: []const u8,
    dst: *[]u8,
    tmp: *[]u8,
    scratch: *[]u32,
) {
    if (res := bwtMtfEncode(src, dst, tmp, scratch)) {
        * = zeroByteRatioPermille(dst.*[0..res.len]);
    }
}
```
