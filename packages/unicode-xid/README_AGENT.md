# unicode-xid@1.0.0 (Agent Quickref)

> Zero-allocation UAX #31 Unicode Identifier (XID_Start / XID_Continue) classification and identifier scanner for CP (clean-room MIT). | License: MIT
> Import: `import { ... } from "unicode-xid";` | Full types: `src/index.d.cp`

## Functions
- `export fn uxidDecodeUtf8Strict(s: string, offset: usize): UxidRune;`
- `export fn uxidIsExtraStart(cp: u32): bool;`
- `export fn uxidBsearchStart(cp: u32): bool;`
- `export fn uxidBsearchCont(cp: u32): bool;`
- `export fn uxidIsAsciiAlpha(b: u8): bool;`
- `export fn uxidIsAsciiDigit(b: u8): bool;`
- `export fn isXidStart(cp: u32): bool;`
- `export fn isXidContinue(cp: u32): bool;`
- `export fn isIdentStart(cp: u32): bool;`
- `export fn isAsciiIdent(s: string): bool;`
- `export fn isValidIdent(s: string): bool;`
- `export fn scanIdentLen(s: string, start: usize): usize;`
