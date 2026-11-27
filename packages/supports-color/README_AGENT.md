# supports-color@1.0.0 (Agent Quickref)

> Zero-allocation terminal color capability detection (supports-color v9 parity) powered by @cap/has-flag in pure CP | License: MIT
> Import: `import { ... } from "supports-color";` | Full types: `src/index.d.cp`
> Deps: `has-flag`

## Functions
- `export fn translateLevel(level: u8): ColorSupport;`
- `export fn detectColorLevel(env: TerminalEnv, argv: &[]string): u8;`
- `export fn detectColorSupport(env: TerminalEnv, argv: &[]string): ColorSupport;`
