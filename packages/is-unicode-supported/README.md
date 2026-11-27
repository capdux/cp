[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/is-unicode-supported

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:is-unicode-supported` ([https://github.com/sindresorhus/is-unicode-supported](https://github.com/sindresorhus/is-unicode-supported))

Zero-allocation pure-logic terminal Unicode support detector for CP — clean-room MIT parity with npm `is-unicode-supported` v2.

纯 CP 零堆分配、零外部依赖的终端 Unicode 支持检测库（完整对标 npm `is-unicode-supported` v2 逻辑），通过不可变环境快照结构体 `EnvInfo` 实现可测试、跨平台的纯逻辑判定。

## Install / 安装

```bash
cap add @cap/is-unicode-supported
```

## API

| Symbol | Signature | Description |
| --- | --- | --- |
| `EnvInfo` | `struct { is_windows, ci, wt_session, terminus_sublime, conemu_task, term_program, term, terminal_emulator }` | Terminal environment snapshot with `.posix()`, `.posixWithTerm(term)`, `.linuxConsole()`, `.windows()`, `.windowsTerminal()`, `.vscodeWindows()` constructors |
| `isUnicodeSupported` | `(env: EnvInfo): bool` | Full `is-unicode-supported` v2 detection logic |
| `isUnicodeSupportedTerm` | `(term: string): bool` | Checks known Unicode-capable `TERM` identifiers (`xterm-256color`, `alacritty`, `rxvt-unicode`, `rxvt-unicode-256color`) |
| `isUnicodeSupportedTermProgram` | `(term_program: string): bool` | Checks known Unicode-capable `TERM_PROGRAM` identifiers (`Terminus-Sublime`, `vscode`) |
| `isUnicodeSupportedPlatform` | `(is_windows: bool, term: string): bool` | Simplified platform + `TERM` check |
| `selectFallback` | `(env: EnvInfo, unicode_val: string, ascii_val: string): string` | Returns `unicode_val` if supported, otherwise `ascii_val` |

## Usage / 使用示例

```cp
import { EnvInfo, isUnicodeSupported, selectFallback } from "@cap/is-unicode-supported";

const ok1 = isUnicodeSupported(EnvInfo.posix());           // true
const ok2 = isUnicodeSupported(EnvInfo.linuxConsole());    // false
const ok3 = isUnicodeSupported(EnvInfo.windowsTerminal()); // true
const icon = selectFallback(EnvInfo.windows(), "✔", "v");  // "v"
```
