# nopt@1.0.0 (Agent Quickref)

> Zero-allocation CLI option parser with shorthands, --no-flag negation, and -- terminator (npm nopt parity) in pure CP | License: ISC
> Import: `import { ... } from "nopt";` | Full types: `src/index.d.cp`

## Functions
- `export fn nptStrEq(a: string, b: string): bool;`
- `export fn nptHasPrefix(s: string, prefix: string): bool;`
- `export fn nptIndexOfByte(s: string, ch: u8): usize?;`
- `export fn nptStripLeadingDashes(s: string): string;`
- `export fn nptFindShorthand(shorthands: &[]NoptShorthand, short_name: string): string?;`
- `export fn nptIsOptionToken(arg: string): bool;`
- `export fn parseNopt(args: &[]string, shorthands: &[]NoptShorthand): NoptResult;`
