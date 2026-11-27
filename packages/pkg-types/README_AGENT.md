# pkg-types@1.3.1 (Agent Quickref)

> Zero-heap package.json and cap.json manifest parser and package name validator | License: MIT
> Import: `import { ... } from "pkg-types";` | Full types: `src/index.d.cp`

## Functions
- `export fn pkgtIsSpace(c: u8): bool;`
- `export fn pkgtSkipSpace(s: string, start: usize): usize;`
- `export fn pkgtStrEq(a: string, b: string): bool;`
- `export fn pkgtScanStringEnd(s: string, open_quote: usize): usize;`
- `export fn pkgtSkipValue(s: string, start: usize): usize;`
- `export fn pkgtEmptyManifest(): PkgManifest;`
- `export fn parsePkgManifest(json_text: string): PkgManifest;`
- `export fn pkgtIsValidSegmentChar(c: u8): bool;`
- `export fn pkgtIsValidSegment(seg: string): bool;`
- `export fn isValidPkgName(name: string): bool;`
- `export fn splitPkgScope(name: string): PkgNameParts;`
