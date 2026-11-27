# `commander` — AI Agent Quick Ref

Zero-allocation CLI command, option, subcommand & help framework (npm `commander` v12 parity; composes `yargs-parser` + `cliui`).

## Import

```cp
import {
    ARG_BOOL, ARG_INT, ARG_STRING,
    FlagSpec, OptionDef, ArgDef, SubcommandDef, CommandParseResult, Command,
    parseOptionFlags, parseArgSpec,
} from "commander";
```

## Core API Signatures

```cp
// Helpers
fn parseOptionFlags(flags: string): FlagSpec;
fn parseArgSpec(spec: string, desc: string): ArgDef;

// Command (stack limits: 16 options, 16 subcommands, 8 arguments, 16 positionals)
Command.new(name: string): Command;
cmd.version(*_, v: string);
cmd.description(*_, d: string);
cmd.option(*_, flags: string, desc: string, default_str: string): bool;
cmd.requiredOption(*_, flags: string, desc: string): bool;
cmd.argument(*_, spec: string, desc: string): bool;
cmd.subcommand(*_, name: string, desc: string): bool;
cmd.parse(argv: []const string): CommandParseResult;
cmd.helpInformation(width: usize, out: []u8): usize;

// CommandParseResult fields: ok, show_help, show_version, args, pos_count, error_len
res.hasSubcommand(): bool;
res.subcommandName(out: []u8): usize;
res.errorMessage(out: []u8): usize;
res.hasOption(name: string): bool;
res.getBool(name: string, default_val: bool): bool;
res.getInt(name: string, default_val: i64): i64;
res.getString(name: string, out: []u8): i64;   // -1 if absent, else bytes written
res.argCount(): usize;
res.argAt(idx: usize, out: []u8): usize;
```

## Minimal Example

```cp
import { Command } from "commander";

let cmd = Command.new("cli");
cmd.version("1.0.0");
cmd.option("-p, --port <num>", "Listen port", "8080");
cmd.requiredOption("-t, --token <val>", "Auth token");
const argv: [4]string = ["-p", "3000", "--token", "abc"];
const res = cmd.parse(&argv);
if (res.ok) { const port = res.getInt("port", 8080); /* 3000 */ }
```

## Gotchas & Borrow Rules

- Builder methods (`version`, `description`, `option`, `requiredOption`, `argument`, `subcommand`) take `*_`; bind with `let cmd = Command.new(...)`.
- `getString` returns `i64`: check `>= 0` before slicing `out[0..(len as usize)]` (`-1` means missing).
- Never exits on `--help`/`--version`; always check `res.show_help` and `res.show_version` before `res.ok`.
- Pass `""` as `default_str` in `cmd.option(...)` when no default value is desired.
