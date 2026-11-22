# `cap:minhash` — Zero-Allocation MinHash & LSH Banding Library

> [!NOTE]
> 本子模块已归并至统一主包 `cap:crypto`，导入请使用 `import { ... } from "cap:crypto"`。

Pure CP, zero-heap-allocation implementation of **Andrei Broder's MinHash** sketch for Jaccard similarity estimation, exact monotonic union merging, cardinality estimation, and Locality-Sensitive Hashing (LSH) band keying.

## Constants

- `MINHASH_DEFAULT_K = 128`
- `MINHASH_EMPTY_VAL = 0xFFFFFFFFFFFFFFFF`

## Core Types & Functions

- **`MinHash`** (caller-owned `sig: []u64` buffer, supports any `k >= 1`):
  - `MinHash.init(sig: []u64): MinHash?` — Initializes all slots to `MINHASH_EMPTY_VAL` (`null` on empty slice).
  - `reset(*_)` / `len(): usize`
  - `addHash(*_, elem_hash: u64)` — Updates all `k` slots via independent SplitMix64 permutations.
  - `addBytes(*_, item: []const u8)` — 64-bit FNV-1a + `addHash`.
  - `addTextWords(*_, text: []const u8)` — Zero-copy lowercase ASCII word tokenizer.
  - `addCharShingles(*_, text: []const u8, k: usize)` — Zero-copy sliding `k`-gram shingler.
  - `mergeFrom(*_, other: MinHash): bool` — Element-wise `min` computing exact union signature.
  - `jaccard(other: MinHash): f64?` — Fraction of matching signature slots.
- **Standalone Utilities**:
  - `minhashJaccard(a: []const u64, b: []const u64): f64?`
  - `minhashBandHash(sig: []const u64, band_idx: usize, rows_per_band: usize): u64?`
  - `minhashCardinality(sig: []const u64): f64`

## Example

```cp
import { MinHash, minhashBandHash } from "cap:crypto";

fn compareDocs(sig_a: []u64, sig_b: []u64): f64? {
    if (a := MinHash.init(sig_a)) {
        if (b := MinHash.init(sig_b)) {
            let ma = a;
            let mb = b;
            ma.addTextWords("Fast native CP language");
            mb.addTextWords("Fast native CP compiler");
            * = minhashBandHash(ma.sig, 0, 4);
            return ma.jaccard(mb);
        }
    }
    return null;
}
```
