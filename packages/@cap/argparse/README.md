# @cap/argparse

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:argparse` ([https://github.com/nodeca/argparse](https://github.com/nodeca/argparse))

Zero-allocation Python-style CLI argument parser for CP — clean-room Python-2.0 parity with npm `argparse` v2 (377M+/wk).

纯 CP 零堆分配 Python 风格命令行参数解析器，支持 `Store` / `StoreTrue` / `StoreFalse` / `Count` 动作、短标志组合（`-ab`、`-vvv`）、长选项（`--output=file`、`--output file`）、`--` 终止选项解析、位置参数绑定及必填参数校验。

## Install / 安装

```bash
cap add @cap/argparse
```

## API

| Symbol | Signature | Description |
| --- | --- | --- |
| `ArgAction` | `enum` | `Store`, `StoreTrue`, `StoreFalse`, `Count` |
| `ArgDef` | `struct` | Argument definition (`short_flag`, `long_name`, `dest`, `action`, `required`, `default_val`, `is_positional`) |
| `ArgParser` | `struct` | Parser supporting up to 32 defs: `ArgParser.init()`, `.addArgument(*_, def)`, `.parseArgs(argv)` |
| `ParseResult` | `struct` | Result with `ok`, `err_code`, `.getString(dest)`, `.getBool(dest)`, `.getCount(dest)`, `.positionalCount()`, `.positionalAt(idx)` |

## Usage / 使用示例

```cp
import { ArgParser, ArgDef } from "@cap/argparse";

let parser = ArgParser.init();
* = parser.addArgument(ArgDef{
    short_flag: 'v', long_name: "--verbose", dest: "verbose",
    action: .Count, required: false, default_val: "", is_positional: false,
});
const args: [1]string = ["-vvv"];
const res = parser.parseArgs(args[0..1]);
const level = res.getCount("verbose"); // 3
```
