# `@cap/bwt`

Zero-heap-allocation Burrows-Wheeler Transform (BWT) and Move-To-Front (MTF) reversible compression preprocessing engine in pure CP.

## Features

- **Burrows-Wheeler Transform (`bwtForward` / `bwtInverse`)**: Deterministic iterative merge sort over cyclic rotations (`MAX_BLOCK = 512`) and O(N) LF-Mapping exact inverse.
- **Move-To-Front (`mtfEncode` / `mtfDecode`)**: Full 256-byte alphabet MTF transform turning BWT symbol clusters into zero runs for RLE/Huffman stages.
- **Zero Heap Allocation**: All buffers live on stack or caller-provided slices.

## API

```cp
import { MAX_BLOCK, BwtResult, bwtForward, bwtInverse, mtfEncode, mtfDecode } from "@cap/bwt";
```

- `MAX_BLOCK: usize = 512`
- `bwtForward(src: []const u8, dst: []u8) -> BwtResult`
- `bwtInverse(src: []const u8, primary_index: usize, dst: []u8) -> bool`
- `mtfEncode(src: []const u8, dst: []u8) -> bool`
- `mtfDecode(src: []const u8, dst: []u8) -> bool`

## License

MIT
