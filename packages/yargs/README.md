[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# yargs

Zero-allocation CLI application framework (npm yargs v17 parity) composing yargs-parser, cliui, and y18n for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:yargs` ([https://github.com/yargs/yargs](https://github.com/yargs/yargs))

| Metadata | Value |
| --- | --- |
| Package | `yargs` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add yargs
```

```cp
import {
    OptionSpec,
    CommandSpec,
    YargsResult,
    YargsApp,
    ygStrEq,
    ygAppendBytes,
    ygAppendByte,
    ygCopyBuf,
} from "yargs";
```

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `OptionSpec` | `class` | Fields: `name: string, alias_ch: u8, desc: string, kind: u8, required: bool, default_str: string, +2 more` · Methods: `boolOpt, intOpt, strOpt` |
| `CommandSpec` | `class` | Fields: `name: string, desc: string` |
| `YargsResult` | `class` | Fields: `args: ParsedArgs, command: [64]u8, command_len: usize, show_help: bool, show_version: bool, ok: bool, +2 more` · Methods: `hasCommand, commandName, errorMessage, has, getBool, getInt, +5 more` |
| `YargsApp` | `class` | Fields: `script_name: string, usage_text: string, version_str: string, commands: [16]CommandSpec, cmd_count: usize, options: [24]OptionSpec, +2 more` · Methods: `new, usage, command, option, setTranslation, parse, +1 more` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `ygStrEq` | `(a: string, b: string): bool` | Invokes `ygStrEq` and returns `bool`. |
| `ygAppendBytes` | `(dst: []u8, pos: usize, s: string): usize` | Invokes `ygAppendBytes` and returns `usize`. |
| `ygAppendByte` | `(dst: []u8, pos: usize, b: u8): usize` | Invokes `ygAppendByte` and returns `usize`. |
| `ygCopyBuf` | `(dst: []u8, src: string): usize` | Invokes `ygCopyBuf` and returns `usize`. |
| `ygInsertEntry` | `(parsed: &mut ParsedArgs, name: string, kind: u8, bool_val: bool, int_val: i64, str_val: string)` | Executes `ygInsertEntry` in-place. |

## Minimal Example

```cp
import { ygStrEq } from "yargs";

pub fn main() {
    let result = ygStrEq("example", "example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
