# shellexpand@1.0.0 (Agent Quickref)

> Zero-allocation shell-like tilde (~) and environment variable ($VAR, ${VAR}, ${VAR:-default}) expansion (clean-room MIT implementation of crates.io shellexpand) | License: MIT
> Import: `import { ... } from "shellexpand";` | Full types: `src/index.d.cp`

## Functions
- `export fn tildeWithHome(input: string, home_dir: string, out: []u8): usize;`
- `export fn envExpand(input: string, vars: &[]EnvEntry, keep_unknown: bool, out: []u8): usize;`
- `export fn fullExpand(input: string, home_dir: string, vars: &[]EnvEntry, out: []u8): usize;`
