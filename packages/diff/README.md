[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# `diff`

Zero-allocation Myers `O(ND)` line & character diff engine for CP — clean-room parity with **npm `diff`** (`jsdiff`) & **`fast-diff`** (plus `similar`-style similarity ratio), featuring FNV-1a 64-bit line hash pre-filtering, GNU Unified Diff formatting, and lossless verified patch application.

## Provenance & Zero-Cost Migration

| Field | Details |
| :--- | :--- |
| **Source Registries** | `npm:diff` ([kpdecker/jsdiff](https://github.com/kpdecker/jsdiff), v7 parity) & `npm:fast-diff` ([jhchen/fast-diff](https://github.com/jhchen/fast-diff), v1 parity) |
| **CP Package** | `diff@1.0.0` (`MIT`) |
| **Dependencies** | None (pure CP, zero external dependencies) |

Keep the exact original package name — no renaming required:

```ts
// TypeScript (npm diff)
import { diffLines, createTwoFilesPatch } from "diff";
const changes = diffLines(oldText, newText);
const patch = createTwoFilesPatch("a.txt", "b.txt", oldText, newText);
```

```cp
// CP — identical package name, zero-heap Myers O(ND)
import { diffLines, formatUnifiedDiff } from "diff";

const res = diffLines(old_text, new_text) catch return;
let patch_buf: [1024]u8 = undefined;
const patch = formatUnifiedDiff("a.txt", "b.txt", old_text, new_text, 3, patch_buf[0..]) catch return;
```

## Architecture & Zero-Heap Design

- **FNV-1a 64-bit hash pre-filtering**: Line splitting precomputes a 64-bit FNV-1a hash per line for `O(1)` hash + length rejection before exact byte comparison.
- **Prefix/suffix trimming + compact triangular trace**: Identical files short-circuit in 0 Myers iterations; the core Myers `O(ND)` trace uses a compact triangular `u16` stack buffer (`undefined` lazy init, zero stack-clear overhead, 8× smaller than a naive 2D matrix).
- **Lossless line endings & verified patching**: Preserves `\n` and `\r\n` verbatim so `applyLineDiff(old, diffLines(old, new), out) == new`, and validates every `Equal`/`Delete` line against `old_text` (returning `DiffError.PatchFailed` on mismatch).

## Examples

### 1. Line-Level Myers Diff, Similarity Ratio & Lossless Patching

```cp
import { diffLines, applyLineDiff, DiffOp } from "diff";

fn demoLineDiff() {
    const old_doc = "alpha\nbeta\ngamma\ndelta\n";
    const new_doc = "alpha\nBETA\ngamma\nepsilon\ndelta\n";

    const res = diffLines(old_doc, new_doc) catch return;
    if (!res.isIdentical()) {
        const stats = res.stats();
        const ratio = stats.similarity(); // 2 * 3 / (4 + 5) = 0.6667
        _ = ratio;
    }

    let out: [512]u8 = undefined;
    const rebuilt = applyLineDiff(old_doc, res, out[0..]) catch return;
    // rebuilt == new_doc
    _ = rebuilt;
}
```

### 2. Standard GNU Unified Diff Formatting (`formatUnifiedDiff`)

```cp
import { formatUnifiedDiff } from "diff";

fn demoUnifiedDiff() {
    const old_src = "line1\nline2\nline3\nline4\nline5\n";
    const new_src = "line1\nline2\nLINE_3_MOD\nline4\nline5\n";

    let patch_buf: [1024]u8 = undefined;
    const patch = formatUnifiedDiff(
        "a/config.cp",
        "b/config.cp",
        old_src,
        new_src,
        1,
        patch_buf[0..],
    ) catch return;
    // --- a/config.cp
    // +++ b/config.cp
    // @@ -2,3 +2,3 @@
    //  line2
    // -line3
    // +LINE_3_MOD
    //  line4
    _ = patch;
}
```

### 3. Character-Level Diff (`diffChars`) & Error Handling (`DiffError`)

```cp
import { diffChars, applyLineDiff, DiffError } from "diff";

fn demoCharsAndErrors() {
    // Byte/character-level Myers diff (up to MAX_LINES = 256 bytes per side)
    const res = diffChars("kitten", "sitting") catch |err| {
        // DiffError.TooManyLines if either side > 256 bytes
        _ = err;
        return;
    };
    const stats = res.stats(); // equal=4, deleted=2, inserted=3
    _ = stats;

    // Applying a diff against mismatched source text fails safely with DiffError.PatchFailed
    let small: [4]u8 = undefined;
    _ = applyLineDiff("wrong-source", res, small[0..]) catch |err| {
        // DiffError.PatchFailed or DiffError.BufferTooSmall
        _ = err;
        return;
    };
}
```

## API Overview

- **Constants**: `MAX_LINES = 256`, `MAX_EDITS = 512`
- **Errors**: `DiffError` (`TooManyLines`, `BufferTooSmall`, `PatchFailed`, `InvalidPatch`)
- **Types**: `DiffOp` (`Equal`, `Delete`, `Insert`), `DiffEdit`, `DiffStats` (`similarity()`), `LineDiffResult` (`stats()`, `isIdentical()`, `slice()`, `get(idx)`)
- **Functions**: `diffLines`, `diffChars`, `formatUnifiedDiff`, `applyLineDiff`, `fnv1a64`, `splitLines`

See [`src/index.d.cp`](src/index.d.cp) for full signatures.
