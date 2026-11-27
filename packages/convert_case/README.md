[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# convert_case

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:convert_case` ([https://github.com/rutrum/convert-case](https://github.com/rutrum/convert-case))（原库名 `convert_case` → CP 包名 `convert_case`）

Zero-allocation 14-style string case conversion engine for CP — clean-room MIT parity with crates.io `convert_case` v0.6.

Composes [`heck`](../heck) for Unicode-aware word boundary detection and the 8 core styles with zero code duplication, adding `flatcase`, `UPPERFLATCASE`, `lower words`, `UPPER WORDS`, `aLtErNaTiNg cAsE`, and `COBOL-CASE`.

## Installation

```json
{
  "dependencies": {
    "convert_case": "1.0.0"
  }
}
```

## Quick Start

```cp
import {
    CASE_SNAKE,
    CASE_COBOL,
    CASE_ALTERNATING,
    convertCase,
    toFlatCase,
} from "convert_case";

let buf: [128]u8 = undefined;
const n1 = convertCase("XMLHttpRequest", CASE_SNAKE, buf[0..]);       // "xml_http_request"
const n2 = convertCase("helloWorld", CASE_COBOL, buf[0..]);           // "HELLO-WORLD"
const n3 = convertCase("hello_world", CASE_ALTERNATING, buf[0..]);    // "hElLo WoRlD"
const n4 = toFlatCase("Hello-World", buf[0..]);                       // "helloworld"
```

## Supported Styles (0–13)

| Constant | Value | Example (`"hello_world"`) |
| --- | --- | --- |
| `CASE_UPPER` | `0` | `HELLO WORLD` |
| `CASE_LOWER` | `1` | `hello world` |
| `CASE_TITLE` | `2` | `Hello World` |
| `CASE_CAMEL` | `3` | `helloWorld` |
| `CASE_PASCAL` | `4` | `HelloWorld` |
| `CASE_SNAKE` | `5` | `hello_world` |
| `CASE_UPPER_SNAKE` | `6` | `HELLO_WORLD` |
| `CASE_KEBAB` | `7` | `hello-world` |
| `CASE_UPPER_KEBAB` | `8` | `HELLO-WORLD` |
| `CASE_COBOL` | `9` | `HELLO-WORLD` |
| `CASE_TRAIN` | `10` | `Hello-World` |
| `CASE_FLAT` | `11` | `helloworld` |
| `CASE_UPPER_FLAT` | `12` | `HELLOWORLD` |
| `CASE_ALTERNATING` | `13` | `hElLo WoRlD` |

## License

MIT
