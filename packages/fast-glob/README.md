[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/fast-glob

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:fast-glob` ([https://github.com/mrmlnc/fast-glob](https://github.com/mrmlnc/fast-glob))

Zero-heap, pure CP glob pattern set, dynamic pattern detector, and path escaper (clean-room MIT implementation inspired by npm `fast-glob` v3).

## Features

- **Zero heap allocation**: Fixed 16-slot include and 16-slot ignore pattern tables.
- **Composable**: Reuses `@cap/glob-match` for wildcard and brace alternation matching.
- **Full `fast-glob` filtering semantics**: Supports `dot`, `deep`, `only_files`, `only_directories`, `case_sensitive`, leading `!` negation patterns, `isDynamicPattern`, `escapePath`, and `convertPathToPattern`.

## Usage

```cp
import {
    FastGlobOptions, GlobPatternSet, isDynamicPattern, escapePath, convertPathToPattern
} from "@cap/fast-glob";

let set = GlobPatternSet.new(FastGlobOptions.defaults());
* = set.addPattern("src/**/*.{cp,ts}");
* = set.addPattern("!src/**/*.test.cp");

let ok = set.matches("src/app/main.cp", false); // true
```
