# @cap/commander

Zero-allocation CLI command framework for CP — clean-room MIT parity with npm `commander` v12. Composes `@cap/yargs-parser` and `@cap/cliui` with zero duplication.

纯 CP 零堆分配命令行框架（对标 npm `commander` v12），复用 `@cap/yargs-parser`（参数解析与短/长标志映射）与 `@cap/cliui`（多列帮助文档排版引擎）。

## Install / 安装

```bash
cap add @cap/commander
```

## Dependencies / 跨包复用

- `@cap/yargs-parser`: 命令行参数解析、短/长选项、`--no-*` 否定开关与位置参数提取
- `@cap/cliui`: 多列自动换行终端排版，用于渲染标准 `Usage / Arguments / Options / Commands` 帮助菜单

## API

| Symbol | Signature | Description |
| --- | --- | --- |
| `parseOptionFlags` | `(flags: string): FlagSpec` | Parse Commander flag declarations (`"-p, --port <number>"`, `"-d, --debug"`, `"--no-color"`, `"[-f, --file [path]]"`) |
| `parseArgSpec` | `(spec: string, desc: string): ArgDef` | Parse positional argument specs (`"<required>"`, `"[optional]"`, `"<items...>"`) |
| `FlagSpec` | `struct` | Parsed short flag, long name, argument name, required/optional/negate/variadic flags |
| `Command` | `struct` + `.new` / `.version` / `.description` / `.option` / `.requiredOption` / `.argument` / `.subcommand` / `.parse` / `.helpInformation` | Zero-allocation CLI command builder, validator, subcommand router & help renderer |
| `CommandParseResult` | `struct` | Parsed result with `.ok`, `.show_help`, `.show_version`, `.hasSubcommand`, `.subcommandName`, `.hasOption`, `.getBool`, `.getInt`, `.getString`, `.argCount`, `.argAt`, `.errorMessage` |

## Usage / 使用示例

```cp
import { Command } from "@cap/commander";

let cmd = Command.new("deploy");
cmd.version("1.2.0");
cmd.description("Deploy services to production");
cmd.option("-p, --port <number>", "Server port", "8080");
cmd.option("-d, --debug", "Enable debug logging", "");
cmd.option("--no-color", "Disable ANSI colors", "");
cmd.argument("<target>", "Target environment");
cmd.subcommand("start", "Start service");

const argv: [3]string = ["-p", "3000", "prod"];
const res = cmd.parse(argv[0..3]);
```
