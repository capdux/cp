[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# `shell-escape`

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:shell-escape` ([https://github.com/xxorax/node-shell-escape](https://github.com/xxorax/node-shell-escape)) / `crates.io:shell-escape` ([https://github.com/sfackler/shell-escape](https://github.com/sfackler/shell-escape))

Zero-heap-allocation POSIX `sh` and Windows `CommandLineToArgvW` argument escaper, command joiner, and environment variable validator in pure CP.

## Features

- **`escapePosix` & `isSafePosixWord`**: Safe single-quoting for POSIX shells (`'` -> `'\''`, empty -> `''`), leaving safe words unquoted.
- **`escapeWindows`**: Microsoft `CommandLineToArgvW` compliant quoting with exact backslash doubling before quotes and trailing backslashes.
- **`joinPosixCommand`**: Escapes and joins argument arrays with spaces into caller buffers.
- **`isValidEnvName`**: Validates POSIX `[A-Za-z_][A-Za-z0-9_]*` variable names.
- **Zero Heap Allocation**: Pure CP, stack-only, no FFI.

## API

```cp
import {
    isSafePosixWord,
    escapePosix,
    escapeWindows,
    joinPosixCommand,
    isValidEnvName,
} from "shell-escape";
```

## License

MIT
