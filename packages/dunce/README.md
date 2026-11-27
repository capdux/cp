[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# dunce

Zero-heap safe Windows verbatim (`\\?\`) and verbatim UNC (`\\?\UNC\`) path simplifier for CP, inspired by Rust crates.io [`dunce`](https://gitlab.com/kornelski/dunce).

## Source Alignment

- **Upstream**: crates.io `dunce` (`kornelski/dunce`, MIT)
- **CP Package**: `dunce` v1.0.0 (clean-room MIT in pure CP)
- **Safety Guarantee**: Strips `\\?\` or simplifies `\\?\UNC\server\share` **only** when the resulting path is 100% safe under legacy Win32 rules (length `<= 259`, no `.`/`..` or `/`, no illegal chars `< > : " | ? *`, no trailing space/dot, no DOS reserved device names like `CON`, `NUL`, `COM1`..`COM9`, `LPT1`..`LPT9`).

## Installation & Import

```bash
cap add dunce
```

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

## Core API

### Constants

| Symbol | Type & Value | Description |
| :--- | :--- | :--- |
| `MAX_LEGACY_PATH` | `const MAX_LEGACY_PATH: usize` (`259`) | Maximum safe legacy Win32 path length (`MAX_PATH - 1`) |

### Functions

| Function | Signature | Description |
| :--- | :--- | :--- |
| `simplifiedSlice` | `fn simplifiedSlice(path: string): string` | Zero-copy simplification for drive verbatim paths (`\\?\C:\...` -> `C:\...`); returns `path` unchanged if unsafe, non-verbatim, or UNC |
| `simplifiedLen` | `fn simplifiedLen(path: string): usize` | Exact byte length produced by `simplified(path, out)` |
| `simplified` | `fn simplified(path: string, out: []u8): usize` | Writes drive-simplified path into `out` (strips `\\?\` iff `canStripVerbatimPrefix` is `true`); returns bytes written |
| `simplifiedUnc` | `fn simplifiedUnc(path: string, out: []u8): usize` | Simplifies `\\?\UNC\server\share...` -> `\\server\share...` when safe; otherwise copies `path` verbatim |
| `simplifiedWithUnc` | `fn simplifiedWithUnc(path: string, out: []u8): usize` | Full simplifier handling both drive verbatim (`\\?\C:\...`) and UNC verbatim (`\\?\UNC\...`) paths |
| `canStripVerbatimPrefix` | `fn canStripVerbatimPrefix(path: string): bool` | Returns `true` iff stripping `\\?\` from a drive path is completely safe on Windows |
| `canSimplifyUncPath` | `fn canSimplifyUncPath(path: string): bool` | Returns `true` iff `\\?\UNC\server\share...` can safely be simplified to `\\server\share...` |
| `hasWindowsVerbatimPrefix` | `fn hasWindowsVerbatimPrefix(path: string): bool` | Returns `true` if `path` starts with `\\?\` |
| `hasWindowsVerbatimUncPrefix` | `fn hasWindowsVerbatimUncPrefix(path: string): bool` | Returns `true` if `path` starts with `\\?\UNC\` (case-insensitive `UNC`) |
| `isSafeLegacyComponent` | `fn isSafeLegacyComponent(component: string): bool` | Validates a single path component against legacy Win32 rules |
| `isWindowsReservedName` | `fn isWindowsReservedName(component: string): bool` | Case-insensitive check for DOS device names (`CON`, `PRN`, `AUX`, `NUL`, `COM1`..`COM9`, `LPT1`..`LPT9`, including `CON.txt`) |
| `isIllegalWinChar` | `fn isIllegalWinChar(ch: u8): bool` | Returns `true` if `ch` is `<`, `>`, `:`, `"`, `\|`, `?`, `*`, `/`, or control byte |

## Examples

### 1. Zero-Copy Drive Path Simplification

```cp
import { simplifiedSlice } from "dunce";
import { print } from "cap:io";

pub fn main() {
    print(simplifiedSlice("\\\\?\\C:\\Users\\alice\\doc.txt")); // "C:\Users\alice\doc.txt"
    print(simplifiedSlice("\\\\?\\C:\\CON.txt"));                // "\\?\C:\CON.txt" (kept verbatim!)
}
```

### 2. Full Drive + UNC Simplification

```cp
import { simplifiedWithUnc } from "dunce";

pub fn normalizeWinPath(path: string, out: []u8): usize {
    return simplifiedWithUnc(path, out);
    // "\\?\UNC\filesrv\share\a.txt" -> "\\filesrv\share\a.txt"
    // "\\?\D:\work\repo"            -> "D:\work\repo"
}
```

### 3. Validating Individual Components

```cp
import { isSafeLegacyComponent, isWindowsReservedName } from "dunce";

pub fn checkName(name: string): bool {
    if (isWindowsReservedName(name)) {
        return false;
    }
    return isSafeLegacyComponent(name);
}
```

## Differences from Rust `dunce` & Safety Highlights

- **Pure String / Slice API**: Does not perform filesystem I/O (`canonicalize`); focuses purely on deterministic, cross-platform verbatim path analysis and simplification.
- **Zero-Copy Drive Fast Path**: `simplifiedSlice` returns a sub-slice of `path` with zero allocation and zero copying.
- **Full UNC Support**: `simplifiedUnc` and `simplifiedWithUnc` handle `\\?\UNC\server\share` -> `\\server\share` with the same strict reserved-name and length guards.
