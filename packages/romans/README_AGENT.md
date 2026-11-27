# romans — AI Agent Quick Ref

Zero-allocation strict Roman numeral (`1..3999`) bidirectional parser, validator, and formatter (npm `romans` / Rust `roman` parity).

## Import

```cp
import {
    MIN_ROMAN, MAX_ROMAN, MAX_ROMAN_LEN,
    romanCharValue, romanLen, toRoman, toRomanLower,
    romanize, romanizeChecked, parseRoman, deromanize, isValidRoman,
} from "romans";
```

## API Signatures

```cp
export const MIN_ROMAN: u16;      // 1
export const MAX_ROMAN: u16;      // 3999
export const MAX_ROMAN_LEN: usize; // 15

export fn romanCharValue(b: u8): i32;
export fn romanLen(num: u16): usize;
export fn toRoman(num: u16, out: []u8): usize;
export fn toRomanLower(num: u16, out: []u8): usize;
export fn romanize(num: i32, out: []u8): usize;
export fn romanizeChecked(num: i32, out: []u8): usize?;
export fn parseRoman(input: string): i32;
export fn deromanize(input: string): i32?;
export fn isValidRoman(input: string): bool;
```

## Minimal Example

```cp
import { toRoman, deromanize, isValidRoman } from "romans";

fn demo(): void {
    let mut buf: [16]u8 = [0; 16];
    const n = toRoman(1994, buf[0..]); // n == 7, "MCMXCIV"
    const v = deromanize("MCMXCIV");   // 1994
    const ok = isValidRoman("IIII");   // false
}
```

## Caveats & Borrowing Rules

- **Buffer sizing**: Always allocate at least `16` bytes (`>= MAX_ROMAN_LEN`, i.e. 15) for `out: []u8`; `toRoman`/`romanize` return `0` and `romanizeChecked` returns `null` if `out.len < romanLen(num)`.
- **Valid range**: Only `1..=3999` is valid. `0` and `>= 4000` (or negative `i32`) yield `0` / `null`.
- **Strict canonical parsing**: `parseRoman` returns `-1` and `deromanize` returns `null` for non-canonical inputs (`"IIII"`, `"VV"`, `"IC"`, `""`). Case-insensitive (`"xliv"` is valid).
- **Mutable slice**: `out: []u8` requires a mutable stack array slice (`let mut buf: [16]u8 = [0; 16]; buf[0..]`). `input: string` is borrowed read-only with zero copies.
