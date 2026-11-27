[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# `execa`

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:execa` ([https://github.com/sindresorhus/execa](https://github.com/sindresorhus/execa))

Zero-heap subprocess command builder, POSIX argument escaper, `preferLocal` PATH resolver, and `ExecaResult` inspector — clean-room CP equivalent of npm `execa`.

Reuses `@cap/npm-run-path` as an independent package.

## Features
- `escapePosixArg(arg, out)` & `formatCommand(file, args, out)` — shell-safe command formatting
- `resolveExecPath(opts, out)` — prepends ancestor `node_modules/.bin` via `@cap/npm-run-path`
- `stripFinalNewline(s)` — strips trailing `\n` or `\r\n`
- `ExecaOptions`, `ExecaResult` (`success`, `failure`, `timeout`, `isOk`)
