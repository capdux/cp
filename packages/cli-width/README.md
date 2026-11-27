[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# cli-width

Zero-allocation terminal column width resolver with TTY streams, `COLUMNS` environment parsing, default fallback, and `[min_width, max_width]` clamping in pure CP.

- **Upstream Alignment**: `npm` `cli-width` (ISC)
- **Package Version**: `1.0.0`
- **Memory Model**: Zero heap allocation — pure value structs and read-only string slices.

## Installation & Import

```bash
cap add cli-width
```

```cp
import {
    CliWidthOptions,
    TtyStreamInfo,
    CliWidthConfig,
    defaultCliWidth,
    clampCliWidth,
    parseColumnsEnv,
    parseColumnsFromEnvList,
    resolveCliWidth,
    cliWidth,
} from "cli-width";
```

## Core API

| Symbol | Signature | Description |
| --- | --- | --- |
| `CliWidthOptions` | `class CliWidthOptions { default_width: u32, min_width: u32, max_width: u32 }` | Options for `resolveCliWidth`. `CliWidthOptions.defaults()` sets `default_width: 80`, `min_width: 0`, `max_width: 0`. |
| `TtyStreamInfo` | `class TtyStreamInfo { is_tty: bool, columns: u32, rows: u32 }` | TTY stream metadata. Constructors: `TtyStreamInfo.none()`, `TtyStreamInfo.tty(columns, rows)`. |
| `CliWidthConfig` | `class CliWidthConfig { default_width, output, tty, env_columns, min_width, max_width }` | Full resolver config matching npm `cli-width` options. `CliWidthConfig.defaults()` defaults to width `80` with no bounds. |
| `defaultCliWidth` | `fn defaultCliWidth(): u32` | Returns standard fallback terminal width (`80`). |
| `clampCliWidth` | `fn clampCliWidth(width: u32, min_width: u32, max_width: u32): u32` | Clamps `width` to `[min_width, max_width]`; a `0` bound disables clamping on that side. |
| `parseColumnsEnv` | `fn parseColumnsEnv(env_val: string): u32?` | Parses a positive decimal `COLUMNS` value (e.g. `"120"`); returns `null` if empty, non-numeric, or `0`. |
| `parseColumnsFromEnvList` | `fn parseColumnsFromEnvList(env_entries: []const string): u32?` | Scans `"KEY=VALUE"` entries for `"COLUMNS=..."` and parses its positive width. |
| `resolveCliWidth` | `fn resolveCliWidth(tty_cols: u32, env_columns: string, opts: CliWidthOptions): u32` | Resolves width by priority: `tty_cols > 0` > `COLUMNS` > `opts.default_width` (fallback `80`), then clamps. |
| `cliWidth` | `fn cliWidth(cfg: CliWidthConfig): u32` | Full npm-compatible priority: `cfg.output` TTY > `cfg.tty` TTY > `cfg.env_columns` > `cfg.default_width` (fallback `80`), then clamps. |

## Examples

### 1. Resolve Width from TTY, Env, and Clamping Bounds

```cp
import { CliWidthOptions, resolveCliWidth } from "cli-width";

fn simpleWidth(): u32 {
    let mut opts = CliWidthOptions.defaults();
    opts.min_width = 40;
    opts.max_width = 120;
    return resolveCliWidth(0, "160", opts); // 120 (clamped)
}
```

### 2. Full `CliWidthConfig` with Output & TTY Streams

```cp
import { CliWidthConfig, TtyStreamInfo, cliWidth } from "cli-width";

fn fullConfigWidth(): u32 {
    let mut cfg = CliWidthConfig.defaults();
    cfg.output = TtyStreamInfo.tty(100, 30);
    cfg.env_columns = "80";
    return cliWidth(cfg); // 100 (output TTY takes priority)
}
```

### 3. Parse `COLUMNS` from an Environment List

```cp
import { parseColumnsFromEnvList } from "cli-width";

fn fromEnv(): u32 {
    const envs: [2]string = ["TERM=xterm-256color", "COLUMNS=132"];
    return parseColumnsFromEnvList(envs[0..]) ?? 80; // 132
}
```

## Differences from Upstream & Highlights

- **Deterministic & Pure**: Instead of reading global `process.stdout` / `process.env` implicitly, TTY dimensions and `COLUMNS` strings are passed explicitly via `CliWidthConfig`, making CLI layout logic 100% deterministic and testable.
- **Built-in Clamping**: Supports `min_width` and `max_width` bounds (`0` disables a bound) directly inside `resolveCliWidth` and `cliWidth`.
- **Zero Allocation**: All parsing operates directly on borrowed `string` / `[]const string` slices.
