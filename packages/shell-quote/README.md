[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# `@cap/shell-quote`

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:shell-quote` ([https://github.com/ljharb/shell-quote](https://github.com/ljharb/shell-quote))

Zero-heap POSIX shell argument quoting (`quoteArg`, `quoteArgs`) and command-line tokenization (`splitCommand`) in pure CP, modeled after npm `shell-quote`. Safely escapes metacharacters, spaces, and single quotes to prevent shell injection.

零堆分配纯 CP POSIX Shell 参数安全转义与命令行分词器，对标 npm `shell-quote`，支持单/双引号、反斜杠转义与安全防命令注入引号包裹。

## Installation

```bash
cap add @cap/shell-quote
```

## API

| Function | Description |
| --- | --- |
| `needsQuoting(arg)` | Check whether an argument contains shell metacharacters or whitespace |
| `quoteArg(arg, dst)` | POSIX-safe single-quote escaping for one argument |
| `quoteArgs(args, dst)` | Quote and join an argv slice with spaces |
| `splitCommand(cmd, out_buf, out_spans)` | Tokenize a POSIX shell command string with unescaping |

## License

MIT
