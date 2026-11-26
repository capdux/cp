# `@cap/similar-diff`

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:similar` ([https://github.com/mitsuhiko/similar](https://github.com/mitsuhiko/similar))（原库名 `similar` → CP 包名 `@cap/similar-diff`）

Zero-allocation line-level LCS diff, unified diff formatter, and similarity ratio engine for CP (clean-room MIT implementation inspired by Rust's `similar` v2).

## Features

- **Zero Heap Allocation**: Operates directly on UTF-8 `string` slices and caller-supplied `[]DiffOp` / `[]u8` buffers.
- **Exact LCS Minimal Edit Script**: Trims common prefixes/suffixes and runs bottom-up LCS DP (up to 128 lines per side, with safe graceful degradation beyond 128 lines).
- **Standard Unified Diff Output**: Produces `--- old\n+++ new\n@@ -a,b +c,d @@\n` with `-`, `+`, and ` ` context lines.
- **Basis-Point Similarity Ratio**: `similarityRatio` computes `0..10000` (`2 * matches * 10000 / (old_lines + new_lines)`), returning `10000` for identical or empty-to-empty inputs.

## Usage

```typescript
import {
    DIFF_EQUAL,
    DIFF_DELETE,
    DIFF_INSERT,
    DiffOp,
    diffLines,
    similarityRatio,
    formatUnifiedDiff,
    hasChanges,
} from "@cap/similar-diff";

const old_s = "alpha\nbeta\ngamma\n";
const new_s = "alpha\nBETA\ngamma\ndelta\n";

let ops: [32]DiffOp = undefined;
const stats = diffLines(old_s, new_s, ops[0..32]);
const ratio: u32 = similarityRatio(old_s, new_s);

let diff_buf: [512]u8 = undefined;
const written: usize = formatUnifiedDiff("a.txt", "b.txt", old_s, new_s, diff_buf[0..512]);
```

## API

- `DIFF_EQUAL` (`0`), `DIFF_DELETE` (`1`), `DIFF_INSERT` (`2`)
- `struct DiffOp { tag: u8, old_idx: usize, new_idx: usize }`
- `struct DiffStats { equal_lines: usize, deleted_lines: usize, inserted_lines: usize, total_ops: usize }`
- `diffLines(old_text: string, new_text: string, ops_out: []DiffOp): DiffStats`
- `similarityRatio(old_text: string, new_text: string): u32`
- `formatUnifiedDiff(old_name: string, new_name: string, old_text: string, new_text: string, out: []u8): usize`
- `hasChanges(old_text: string, new_text: string): bool`

## License

MIT
