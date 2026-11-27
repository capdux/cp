# damerau-levenshtein — AI Agent Quick Ref

Zero-heap OSA and true Damerau-Levenshtein edit distance / similarity ratio in pure CP (npm `damerau-levenshtein` 1.0.8 parity).

## Import

```cp
import {
    DL_MAX_OSA_LEN,
    DL_MAX_TRUE_LEN,
    DamLevResult,
    damerauLevenshtein,
    damerauLevenshteinSteps,
    damerauLevenshteinStepsBytes,
    osaDistance,
    osaDistanceBytes,
    trueDamerauLevenshtein,
    trueDamerauLevenshteinSteps,
    trueDamerauLevenshteinBytes,
    damerauLevenshteinBounded,
    damerauLevenshteinBoundedBytes,
    similarity,
    trueSimilarity,
} from "damerau-levenshtein";
```

## API Signatures

```cp
const DL_MAX_OSA_LEN: usize;   // 255
const DL_MAX_TRUE_LEN: usize;  // 64
class DamLevResult { steps: usize, relative: f64, similarity: f64 }

fn damerauLevenshteinStepsBytes(a: []const u8, b: []const u8): usize;
fn damerauLevenshteinSteps(a: string, b: string): usize;
fn osaDistanceBytes(a: []const u8, b: []const u8): usize;
fn osaDistance(a: string, b: string): usize;
fn trueDamerauLevenshteinBytes(a: []const u8, b: []const u8): usize;
fn trueDamerauLevenshteinSteps(a: string, b: string): usize;
fn damerauLevenshteinBoundedBytes(a: []const u8, b: []const u8, maxDistance: usize): usize?;
fn damerauLevenshteinBounded(a: string, b: string, maxDistance: usize): usize?;
fn damerauLevenshtein(a: string, b: string): DamLevResult;
fn trueDamerauLevenshtein(a: string, b: string): DamLevResult;
fn similarity(a: string, b: string): f64;
fn trueSimilarity(a: string, b: string): f64;
```

## Minimal Example

```cp
import { damerauLevenshtein, damerauLevenshteinBounded } from "damerau-levenshtein";

let r = damerauLevenshtein("ca", "abc"); // r.steps == 3
let b = damerauLevenshteinBounded("recieve", "receive", 2);
if (b != null) { let d: usize = b!; }
```

## Pitfalls & Borrowing Rules

- **Stack length caps**: OSA supports up to `DL_MAX_OSA_LEN` (255 bytes); true Damerau-Levenshtein supports up to `DL_MAX_TRUE_LEN` (64 bytes).
- **Optional return**: `damerauLevenshteinBounded*` returns `usize?` (`null` when distance > `maxDistance`); always check `!= null` before unwrapping.
- **Byte-level semantics**: Distances operate on raw UTF-8 bytes (`[]const u8` / `string`), with zero heap allocation and immutable borrows only.
