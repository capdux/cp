# home — AI Agent Quick Ref

Zero-heap cross-platform home directory resolver, tilde expand/contract, and XDG config dir builder in pure CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    isSlash,
    writeByte,
    trimTrailingSlashes,
    resolveHomeDir,
    expandTilde,
    contractTilde,
    joinConfigDir,
} from "home";
```

## API Signatures

```cp
fn isSlash(ch: u8): bool;
fn writeByte(out: []u8, pos: usize, b: u8): usize;
fn trimTrailingSlashes(s: string): usize;
fn resolveHomeDir(home_env: string, userprofile_env: string, homedrive_env: string, homepath_env: string, is_windows: bool, out: []u8): usize;
fn expandTilde(path: string, home_dir: string, out: []u8): usize;
fn contractTilde(path: string, home_dir: string, out: []u8): usize;
fn joinConfigDir(home_dir: string, xdg_config_home: string, app_name: string, is_windows: bool, out: []u8): usize;
```

## Minimal Usage

```cp
import { trimTrailingSlashes } from "home";

pub fn main() {
    let result = trimTrailingSlashes("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"home"`; install via `cap add home`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.
- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.
- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.
