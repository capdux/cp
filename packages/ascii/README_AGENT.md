# ascii — AI Agent Quick Ref

Zero-heap ASCII validation, `AsciiChar`/`AsciiStr` wrappers, byte classification, and case conversion (Rust `ascii` v1.1.0 parity).

## Import

```cp
import {
    AsciiChar, AsciiStr,
    isAsciiAlpha, isAsciiDigit, isAsciiAlphanumeric, isAsciiHexDigit, isAsciiOctDigit,
    isAsciiWhitespace, isAsciiPrintable, isAsciiControl, isAsciiLowercase, isAsciiUppercase,
    isAsciiGraph, isAsciiPunctuation, toAsciiLowerByte, toAsciiUpperByte,
    eqIgnoreAsciiCase, toAsciiLower, toAsciiUpper, toAsciiLowercase, toAsciiUppercase,
    makeAsciiLowercase, makeAsciiUppercase, trimAscii,
} from "ascii";
```

## API Signatures

```cp
export fn isAsciiAlpha(b: u8): bool; // + Digit, Alphanumeric, HexDigit, OctDigit,
// Whitespace, Printable, Control, Lowercase, Uppercase, Graph, Punctuation
export fn toAsciiLowerByte(b: u8): u8;
export fn toAsciiUpperByte(b: u8): u8;

export class AsciiChar {
    byte: u8,
    static fn fromByte(b: u8): AsciiChar?;
    fn asByte(): u8;
    fn isAlphabetic(): bool; // + isAlphanumeric/Control/Digit/Graph/HexDigit/Lowercase/Printable/Punctuation/Uppercase/Whitespace
    fn toAsciiUppercase(): AsciiChar; fn toAsciiLowercase(): AsciiChar;
    fn eqIgnoreAsciiCase(other: AsciiChar): bool;
}
export class AsciiStr {
    bytes: string,
    static fn fromStr(s: string): AsciiStr?;
    fn len(): usize; fn isEmpty(): bool; fn asStr(): string;
    fn get(idx: usize): AsciiChar?;
    fn eqIgnoreAsciiCase(other: AsciiStr): bool;
    fn trim(): AsciiStr; fn trimStart(): AsciiStr; fn trimEnd(): AsciiStr;
    fn toAsciiLowercase(out: []u8): usize; fn toAsciiUppercase(out: []u8): usize;
}
export fn eqIgnoreAsciiCase(a: string, b: string): bool;
export fn toAsciiLower(s: string, out: []u8): usize;     // alias: toAsciiLowercase
export fn toAsciiUpper(s: string, out: []u8): usize;     // alias: toAsciiUppercase
export fn makeAsciiLowercase(buf: []u8);
export fn makeAsciiUppercase(buf: []u8);
export fn trimAscii(s: string): string;
```

## Minimal Example

```cp
import { AsciiStr, makeAsciiLowercase } from "ascii";

fn demo(): void {
    if (let s = AsciiStr.fromStr("  OK  ")) {
        let mut buf: [8]u8 = [0; 8];
        const n = s.trim().toAsciiLowercase(buf[0..]); // "ok"
        makeAsciiLowercase(buf[0..n]);
    }
}
```

## Caveats & Borrowing Rules

- **7-bit validation**: `AsciiChar.fromByte(b)` and `AsciiStr.fromStr(s)` return `null` if any byte has high bit set (`>= 0x80`).
- **Mutable buffers**: `makeAsciiLowercase`/`makeAsciiUppercase` mutate `buf: []u8` in place; `toAsciiLower`/`toAsciiUpper` write up to `min(s.len, out.len)` bytes into `out: []u8`.
- **Zero-copy**: `AsciiStr.trim*()` and `trimAscii(s)` return sub-slices borrowing the original `string`.
