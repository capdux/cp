# cli-width — AI Agent Quick Ref

Zero-allocation terminal column width resolver with TTY, `COLUMNS` env, default fallback, and min/max clamping (npm `cli-width` parity).

## Import

```cp
import {
    CliWidthOptions, TtyStreamInfo, CliWidthConfig,
    defaultCliWidth, clampCliWidth, parseColumnsEnv,
    parseColumnsFromEnvList, resolveCliWidth, cliWidth,
} from "cli-width";
```

## API Signatures

```cp
export class CliWidthOptions {
    default_width: u32, min_width: u32, max_width: u32,
    static fn defaults(): CliWidthOptions; // 80, 0, 0
}
export class TtyStreamInfo {
    is_tty: bool, columns: u32, rows: u32,
    static fn none(): TtyStreamInfo;
    static fn tty(columns: u32, rows: u32): TtyStreamInfo;
}
export class CliWidthConfig {
    default_width: u32, output: TtyStreamInfo, tty: TtyStreamInfo,
    env_columns: string, min_width: u32, max_width: u32,
    static fn defaults(): CliWidthConfig;
}
export fn defaultCliWidth(): u32;
export fn clampCliWidth(width: u32, min_width: u32, max_width: u32): u32;
export fn parseColumnsEnv(env_val: string): u32?;
export fn parseColumnsFromEnvList(env_entries: []const string): u32?;
export fn resolveCliWidth(tty_cols: u32, env_columns: string, opts: CliWidthOptions): u32;
export fn cliWidth(cfg: CliWidthConfig): u32;
```

## Minimal Example

```cp
import { CliWidthConfig, TtyStreamInfo, cliWidth } from "cli-width";

fn demo(): u32 {
    let mut cfg = CliWidthConfig.defaults();
    cfg.output = TtyStreamInfo.tty(120, 40);
    cfg.max_width = 100;
    return cliWidth(cfg); // 100
}
```

## Caveats & Borrowing Rules

- **Priority order in `cliWidth`**: `cfg.output` (if `is_tty && columns > 0`) > `cfg.tty` (if `is_tty && columns > 0`) > `cfg.env_columns` > `cfg.default_width` (fallback `80` if `0`), then clamped.
- **Clamping semantics**: Setting `min_width = 0` or `max_width = 0` disables clamping on that side.
- **`parseColumnsEnv`**: Rejects `0`, negative numbers, and non-digit strings by returning `null`.
- **Zero-copy**: All `string` and `[]const string` inputs are read-only borrows; no heap memory is allocated.
