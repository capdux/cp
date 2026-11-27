# home@1.0.0 (Agent Quickref)

> CP package home | License: MIT
> Import: `import { ... } from "home";` | Full types: `src/index.d.cp`

## Functions
- `export fn isSlash(ch: u8): bool;`
- `export fn writeByte(out: []u8, pos: usize, b: u8): usize;`
- `export fn trimTrailingSlashes(s: string): usize;`
- `export fn resolveHomeDir(home_env: string, userprofile_env: string, homedrive_env: string, homepath_env: string, is_windows: bool, out: []u8): usize;`
- `export fn expandTilde(path: string, home_dir: string, out: []u8): usize;`
- `export fn contractTilde(path: string, home_dir: string, out: []u8): usize;`
- `export fn joinConfigDir(home_dir: string, xdg_config_home: string, app_name: string, is_windows: bool, out: []u8): usize;`
