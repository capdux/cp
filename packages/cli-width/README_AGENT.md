# cli-width@1.0.0 (Agent Quickref)

> Zero-allocation terminal column width resolver with TTY, COLUMNS env, default, and min/max clamping (npm cli-width parity) in pure CP | License: ISC
> Import: `import { ... } from "cli-width";` | Full types: `src/index.d.cp`

## Functions
- `export fn cliwIsSpace(c: u8): bool;`
- `export fn defaultCliWidth(): u32;`
- `export fn parseColumnsEnv(env_val: string): u32?;`
- `export fn resolveCliWidth(tty_cols: u32, env_columns: string, opts: CliWidthOptions): u32;`
