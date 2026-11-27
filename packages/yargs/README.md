[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/yargs

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:yargs` ([https://github.com/yargs/yargs](https://github.com/yargs/yargs))

Zero-allocation CLI application framework for CP — clean-room MIT parity with npm `yargs` v17. Composes `@cap/yargs-parser`, `@cap/cliui`, and `@cap/y18n` with zero duplication.

纯 CP 零堆分配命令行框架（对标 npm `yargs` v17），完整串联 `@cap/yargs-parser`（参数解析与驼峰/连字符键映射）、`@cap/cliui`（多列帮助排版引擎）与 `@cap/y18n`（国际化提示与报错文案）。

## Install / 安装

```bash
cap add @cap/yargs
```

## Dependencies / 跨包复用

- `@cap/yargs-parser`: 命令行参数解析、短组合标志、`--` 透传与驼峰/连字符键查询
- `@cap/cliui`: 多列终端排版与自动换行帮助菜单
- `@cap/y18n`: 内置多语言标题、类型标签与必填校验错误格式化

## API

| Symbol | Signature | Description |
| --- | --- | --- |
| `OptionSpec` | `struct` + `.boolOpt` / `.intOpt` / `.strOpt` | Option schema with short alias, type, default & required flag |
| `CommandSpec` | `struct { name: string, desc: string }` | Subcommand definition |
| `YargsResult` | `struct` | Parsed CLI result (`args`, `command`, `show_help`, `show_version`, `ok`, `error_msg`) |
| `YargsApp` | `struct` + `.new` / `.usage` / `.command` / `.option` / `.parse` / `.renderHelp` | Full CLI app builder, validator & help renderer |

## Usage / 使用示例

```cp
import { YargsApp, OptionSpec } from "@cap/yargs";

let app = YargsApp.new("cap-cli", "1.0.0");
app.command("serve", "Start HTTP server");
app.option(OptionSpec.intOpt("port", 'p' as u8, "Listen port", 3000, false));

const argv: [3]string = ["serve", "-p", "8080"];
const res = app.parse(argv[0..3]); // command == "serve", port == 8080
```
