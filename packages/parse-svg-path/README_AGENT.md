# parse-svg-path@1.0.0 (Agent Quickref)

> Zero-allocation SVG path `d` attribute parser, bounding box calculator, and canonical formatter for CP | License: MIT
> Import: `import { ... } from "parse-svg-path";` | Full types: `src/index.d.cp`

## Functions
- `export fn isWs(c: u8): bool;`
- `export fn isDigit(c: u8): bool;`
- `export fn skipWs(s: string, start: usize): usize;`
- `export fn skipWsComma(s: string, start: usize): usize;`
- `export fn expectedArgCount(code: u8): u8?;`
- `export fn isLowerCmd(code: u8): bool;`
- `export fn isNumberStart(c: u8): bool;`
- `export fn pow10(exp: i32): f64;`
- `export fn parseSvgNumber(s: string, start: usize): ParsedNum;`
- `export fn parseSvgFlag(s: string, start: usize): ParsedNum;`
- `export fn parseSvgPath(d: string): ParsedPath;`
- `export fn includePoint(acc: BoundsAcc, x: f64, y: f64): BoundsAcc;`
- `export fn computeBounds(p: ParsedPath): PathBounds;`
- `export fn appendByte(out: []u8, pos: usize, c: u8): usize;`
- `export fn appendU64(out: []u8, pos: usize, val: u64): usize;`
- `export fn appendFloatCompact(out: []u8, start_pos: usize, val: f64): usize;`
- `export fn formatSvgPath(p: ParsedPath, out: []u8): string;`
