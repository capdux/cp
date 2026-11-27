# natural-compare@1.0.0 (Agent Quickref)

> Zero-allocation natural / alphanumeric string comparison and in-place sorting (natural-compare / natsort / natord parity, arbitrary-length integer segments without overflow) for CP | License: MIT
> Import: `import { ... } from "natural-compare";` | Full types: `src/index.d.cp`

## Functions
- `export fn isDigit(c: u8): bool;`
- `export fn toLowerAscii(c: u8): u8;`
- `export fn compareRawBytes(a: string, b: string): i8;`
- `export fn naturalCompareImpl(a: string, b: string, ignore_case: bool): i8;`
- `export fn naturalCompare(a: string, b: string): i8;`
- `export fn naturalCompareIgnoreCase(a: string, b: string): i8;`
- `export fn naturalLess(a: string, b: string): bool;`
- `export fn naturalLessIgnoreCase(a: string, b: string): bool;`
- `export fn naturalEq(a: string, b: string): bool;`
- `export fn comesBefore(a: string, b: string, desc: bool, ignore_case: bool): bool;`
- `export fn siftDown(items: []string, start: usize, end: usize, desc: bool, ignore_case: bool);`
- `export fn sortInPlace(items: []string, desc: bool, ignore_case: bool);`
- `export fn sortNatural(items: []string);`
- `export fn sortNaturalDesc(items: []string);`
- `export fn sortNaturalIgnoreCase(items: []string);`
- `export fn isSortedNatural(items: []string): bool;`
