# color-name@1.0.0 (Agent Quickref)

> Zero-allocation 148 W3C CSS Color 4 named colors lookup and exact RGB/HSL/HSV/HEX color space conversion engine for CP | License: MIT
> Import: `import { ... } from "color-name";` | Full types: `src/index.d.cp`

## Functions
- `export fn colorAt(index: usize): ColorEntry;`
- `export fn lookupColor(name: string): RgbColor;`
- `export fn lookupColorHex(name: string): i32;`
- `export fn reverseLookupColor(r: u8, g: u8, b: u8): string;`
- `export fn isNamedColor(name: string): bool;`
