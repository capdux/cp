# wcag-contrast@1.0.0 (Agent Quickref)

> Zero-allocation W3C WCAG 2.1/2.2 color contrast ratio calculator, AA/AAA compliance grader, and accessible foreground color recommender (wcag-contrast / Rust contrast parity) for CP | License: MIT
> Import: `import { ... } from "wcag-contrast";` | Full types: `src/index.d.cp`

## Types & Constants
- `export enum WcagLevel`

## Functions
- `export fn hexNibble(c: u8): i8;`
- `export fn nibbleToHexLower(n: u8): u8;`
- `export fn parseHex(hex: string): Rgb?;`
- `export fn formatHex(c: Rgb, out: []u8): string;`
- `export fn linearizeChannel(channel: u8): f64;`
- `export fn relativeLuminance(c: Rgb): f64;`
- `export fn contrastRatio(fg: Rgb, bg: Rgb): f64;`
- `export fn contrastRatioHex(fg_hex: string, bg_hex: string): f64?;`
- `export fn meetsAA(ratio: f64, large_text: bool): bool;`
- `export fn meetsAAA(ratio: f64, large_text: bool): bool;`
- `export fn gradeContrast(ratio: f64): WcagLevel;`
- `export fn bestTextColor(bg: Rgb): Rgb;`
- `export fn suggestAccessibleHex(bg_hex: string): string;`
