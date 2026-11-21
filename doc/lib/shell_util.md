# `cap:shell_util` — 纯 CP 零堆分配 POSIX Shell 转义、路径与环境变量工具集

## 概述

`cap:shell_util` 吸收自 Bun `src/shell/util.cp` 与 `src/shell/builtin/{basename,dirname}.cp`，纯 CP 零外部依赖、零堆分配，提供 POSIX Shell 安全引用/双引号转义、命令行拼接、严格 POSIX `basename`/`dirname` 路径运算及环境变量名/赋值解析。

## 核心 API

- `isSafeUnquoted(s: []const u8): bool`：判断是否仅含安全无引号字符（`[A-Za-z0-9_@%+=:,./-]+`）
- `quotePosix(input: []const u8, out: []u8): ([]const u8)?`：POSIX 单引号转义（空串 -> `''`，单引号 -> `'\''`）
- `escapeDoubleQuoted(input: []const u8, out: []u8): ([]const u8)?`：双引号内 `$` / `` ` `` / `"` / `\` 转义
- `joinCommand(args: []const []const u8, out: []u8): ([]const u8)?`：安全拼接命令行
- `posixBasename(path: []const u8, suffix: []const u8): []const u8`：严格 POSIX `basename`
- `posixDirname(path: []const u8): []const u8`：严格 POSIX `dirname`
- `isValidEnvVarName(name: []const u8): bool` 与 `splitEnvAssignment(s: []const u8): EnvAssignment?`

## 示例

```cp
import { quotePosix, posixBasename, posixDirname, splitEnvAssignment } from "cap:shell_util";

let buf: [64]u8 = std.mem.zeroes([64]u8);
const q = quotePosix("it's $(rm)", buf[0..]).?; // "'it'\\''s $(rm)'"
const b = posixBasename("/var/log/app.tar.gz", ".gz"); // "app.tar"
const d = posixDirname("/var/log/app.tar.gz");        // "/var/log"
const kv = splitEnvAssignment("NODE_ENV=production").?;
```
