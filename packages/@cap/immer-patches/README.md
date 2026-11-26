# @cap/immer-patches

Zero-heap structured state patches (Immer / RFC 6902) and RFC 6901 JSON Pointer codec with inverse patch generation for Undo/Redo in pure CP.

## API

- `PatchOp` enum (`Add`, `Replace`, `Remove`)
- `StatePatch { op: PatchOp, path: string, value: string, prev_value: string }`
- `invertPatch(p: StatePatch): StatePatch`
- `formatJsonPointer(segments: []const string, out: []u8): usize`
- `parseJsonPointer(ptr: string, out: []string): usize`
- `unescapePointerToken(token: string, out: []u8): usize`
- `patchOpName(op: PatchOp): string`

<!-- cap-source-attribution -->
## Source & License Attribution

- **Upstream Ecosystem**: npm (`immer@10.1.1`)
- **Upstream Repository**: https://github.com/immerjs/immer
- **License**: MIT (clean-room CP implementation)
