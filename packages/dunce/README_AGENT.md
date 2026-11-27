# dunce — AI Agent Quick Reference

Zero-heap safe Windows verbatim (`\\?\`) and verbatim UNC (`\\?\UNC\`) path simplifier (Rust `dunce` parity).

## Import

```cp
import {
    MAX_LEGACY_PATH,
    isIllegalWinChar,
    isWindowsReservedName,
    isSafeLegacyComponent,
    hasWindowsVerbatimPrefix,
    hasWindowsVerbatimUncPrefix,
    canStripVerbatimPrefix,
    canSimplifyUncPath,
    simplifiedSlice,
    simplifiedLen,
    simplified,
    simplifiedUnc,
    simplifiedWithUnc,
} from "dunce";
```

## API Signatures

```cp
const MAX_LEGACY_PATH: usize; // 259 (MAX_PATH - 1)

fn isIllegalWinChar(ch: u8): bool;
fn isWindowsReservedName(component: string): bool;
fn isSafeLegacyComponent(component: string): bool;
fn hasWindowsVerbatimPrefix(path: string): bool;
fn hasWindowsVerbatimUncPrefix(path: string): bool;
fn canStripVerbatimPrefix(path: string): bool;
fn canSimplifyUncPath(path: string): bool;
fn simplifiedSlice(path: string): string;
fn simplifiedLen(path: string): usize;
fn simplified(path: string, out: []u8): usize;
fn simplifiedUnc(path: string, out: []u8): usize;
fn simplifiedWithUnc(path: string, out: []u8): usize;
```

## Minimal Example

```cp
import { simplifiedSlice, simplifiedWithUnc } from "dunce";

// Zero-copy drive path simplification
const s = simplifiedSlice("\\\\?\\C:\\dev\\app.cp"); // "C:\dev\app.cp"

// Drive + UNC simplification into buffer
let buf: [512]u8 = [0; 512];
const n = simplifiedWithUnc("\\\\?\\UNC\\srv\\share\\a.txt", buf[0..512]);
// buf[0..n] == "\\srv\share\a.txt"
```

## Gotchas & Borrow Rules

- `simplifiedSlice` only simplifies drive paths (`\\?\C:\...`) and returns a borrowed sub-slice of `path`; for UNC paths (`\\?\UNC\...`), use `simplifiedUnc` or `simplifiedWithUnc` because `\\?\UNC\` rewrites to `\\` (cannot be a contiguous sub-slice).
- Output is always `<= path.len` bytes, so allocating `out.len >= path.len` guarantees no truncation.
- Paths exceeding `MAX_LEGACY_PATH` (259) after stripping, containing `.`/`..`/`/`, ending a component with space/dot, or containing DOS device names (`CON`, `NUL`, `COM1`..`COM9`, `LPT1`..`LPT9`) are preserved verbatim.
