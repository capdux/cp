# is-unicode-supported@1.0.0 (Agent Quickref)

> Zero-allocation pure-logic terminal Unicode support detector (npm is-unicode-supported v2 parity) for CP | License: MIT
> Import: `import { ... } from "is-unicode-supported";` | Full types: `src/index.d.cp`

## Functions
- `export fn iusStrEq(a: string, b: string): bool;`
- `export fn isUnicodeSupportedTerm(term: string): bool;`
- `export fn isUnicodeSupportedTermProgram(term_program: string): bool;`
- `export fn isUnicodeSupported(env: EnvInfo): bool;`
- `export fn isUnicodeSupportedPlatform(is_windows: bool, term: string): bool;`
- `export fn selectFallback(env: EnvInfo, unicode_val: string, ascii_val: string): string;`
