[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# `@cap/proc-args`

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:mri` ([https://github.com/lukeed/mri](https://github.com/lukeed/mri)) / `crates.io:lexopt` ([https://github.com/blyxxyz/lexopt](https://github.com/blyxxyz/lexopt))（原库名 `mri` → CP 包名 `@cap/proc-args`）

Zero-heap-allocation POSIX/GNU CLI argument parser in pure CP.

## Features

- **`parseArgs`**: Parses `--flag`, `--key=val`, clustered short flags `-abc`, lone `"-"` stdin, and `"--"` end-of-options into `ParsedArg` slices.
- **`hasFlag`**: Detects long flags (`--verbose`, `--verbose=true`) and short flags inside clusters (`-vf`) before `--`.
- **`getOption`**: Extracts values for `--key=val`, `--key val`, and `-o val` before `--`.
- **`countPositionals`**: Counts positional arguments including lone `"-"` and all arguments after `--`.
- **Zero Heap Allocation**: Pure CP, zero-copy slices, no FFI.

## API

```cp
import {
    ARG_POSITIONAL,
    ARG_LONG_FLAG,
    ARG_LONG_VALUE,
    ARG_SHORT_FLAG,
    ParsedArg,
    parseArgs,
    hasFlag,
    getOption,
    countPositionals,
} from "@cap/proc-args";
```

## License

MIT
