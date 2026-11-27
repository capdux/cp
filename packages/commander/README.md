[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# `commander`

Zero-allocation CLI command framework for CP — a clean-room MIT counterpart to npm [`commander`](https://github.com/tj/commander.js) (v12 parity), composing `yargs-parser` for option parsing and `cliui` for aligned multi-column help generation.

## Installation & Import

```bash
cap add commander
```

```cp
import {
    ARG_BOOL,
    ARG_INT,
    ARG_STRING,
    FlagSpec,
    OptionDef,
    ArgDef,
    SubcommandDef,
    CommandParseResult,
    Command,
    parseOptionFlags,
    parseArgSpec,
} from "commander";
```

## Core API

### Types & Helper Functions

| Symbol | Signature / Fields | Description |
| :--- | :--- | :--- |
| `ARG_BOOL` / `ARG_INT` / `ARG_STRING` | `u8` constants (re-exported from `yargs-parser`) | Option value kind tags |
| `parseOptionFlags` | `(flags: string): FlagSpec` | Parse flag spec like `"-p, --port <num>"`, `"--no-color"`, `"-v, --verbose"` |
| `parseArgSpec` | `(spec: string, desc: string): ArgDef` | Parse positional spec like `"<input>"`, `"[output]"`, `"<files...>"` |
| `FlagSpec` / `OptionDef` / `ArgDef` / `SubcommandDef` | Value structs | Parsed metadata for flags, options, positionals, and subcommands |

### `Command` Builder Methods

| Method | Signature | Description |
| :--- | :--- | :--- |
| `Command.new` | `(name: string): Command` | Create a command with stack-allocated tables (16 options, 16 subcommands, 8 args) |
| `version` / `description` | `(*_, v: string)` / `(*_, d: string)` | Set CLI version string (`-V`/`--version`) and description |
| `option` | `(*_, flags: string, desc: string, default_str: string): bool` | Register an option with optional default (`""` for none); returns `false` if full |
| `requiredOption` | `(*_, flags: string, desc: string): bool` | Register a mandatory option (fails parse if absent) |
| `argument` | `(*_, spec: string, desc: string): bool` | Register a positional argument (`<req>`, `[opt]`, `<var...>`) |
| `subcommand` | `(*_, name: string, desc: string): bool` | Register a subcommand |
| `parse` | `(argv: []const string): CommandParseResult` | Parse `argv` slice (without program name or raw tokens) into result |
| `helpInformation` | `(width: usize, out: []u8): usize` | Render formatted help screen into caller buffer `out`; returns bytes written |

### `CommandParseResult` Methods

| Method | Signature | Description |
| :--- | :--- | :--- |
| `hasSubcommand` / `subcommandName` | `(): bool` / `(out: []u8): usize` | Check and copy matched subcommand name into `out` |
| `errorMessage` | `(out: []u8): usize` | Copy diagnostic message into `out` when `!res.ok` |
| `hasOption` | `(name: string): bool` | Return `true` if option was provided or defaulted |
| `getBool` / `getInt` | `(name: string, default_val): bool / i64` | Read parsed boolean or integer option with fallback |
| `getString` | `(name: string, out: []u8): i64` | Copy string option into `out`; returns byte length (`>= 0`) or `-1` if missing |
| `argCount` / `argAt` | `(): usize` / `(idx: usize, out: []u8): usize` | Count and copy positional arguments |

## Examples

### 1. Defining Options, Subcommands, and Parsing `argv`

```cp
import { print } from "cap:io";
import { Command } from "commander";

fn main() {
    let cmd = Command.new("deploy");
    cmd.version("1.2.0");
    cmd.description("Zero-downtime deployment tool");
    cmd.option("-p, --port <num>", "Server port", "8080");
    cmd.option("-v, --verbose", "Enable verbose logs", "");
    cmd.requiredOption("-e, --env <name>", "Target environment");
    cmd.subcommand("start", "Start service");
    cmd.subcommand("status", "Check service health");

    const argv: [5]string = ["start", "--env", "prod", "-p", "9000"];
    const res = cmd.parse(&argv);

    if (res.ok) {
        let sub_buf: [64]u8 = undefined;
        const sub_len = res.subcommandName(sub_buf[0..64]);
        let env_buf: [64]u8 = undefined;
        const env_len = res.getString("env", env_buf[0..64]);
        print("sub:", sub_buf[0..sub_len], "port:", res.getInt("port", 8080), "env_len:", env_len);
    }
}
```

### 2. Rendering Auto-Aligned Help Output

```cp
import { print } from "cap:io";
import { Command } from "commander";

fn printHelp(cmd: Command) {
    let help_buf: [2048]u8 = undefined;
    const n = cmd.helpInformation(80, help_buf[0..2048]);
    print(help_buf[0..n]);
}
```

## Differences from Upstream & Performance Highlights

- **Zero Heap Allocation**: All option definitions (max 16), subcommands (max 16), arguments (max 8), positionals (max 16), and error messages (192 bytes) reside in inline stack arrays — ideal for fast-startup native CLIs.
- **No `process.exit()` Side Effects**: Instead of exiting the process on `--help`, `--version`, or invalid flags, `parse()` sets `res.show_help`, `res.show_version`, or `res.ok = false` with `errorMessage(out)`.
- **Composable Ecosystem Design**: Delegates low-level flag parsing to `yargs-parser` and responsive column wrapping to `cliui`.
