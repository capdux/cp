[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/humanize-string

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:humanize-string` ([https://github.com/sindresorhus/humanize-string](https://github.com/sindresorhus/humanize-string))

Zero-allocation camelCase/snake_case/kebab-case to humanized sentence converter for CP — clean-room MIT parity with npm `humanize-string` v3. Composes `@cap/decamelize` with zero duplication.

纯 CP 零堆分配标识符人性化转换库（对标 npm `humanize-string` v3），跨包复用 `@cap/decamelize`，将驼峰、下划线、连字符及多余空白规范化为首字母大写的自然可读短语。

## Install / 安装

```bash
cap add @cap/humanize-string
```

## Dependencies / 跨包复用

- `@cap/decamelize`: 驼峰标识符拆分与小写化

## API

| Symbol | Signature | Description |
| --- | --- | --- |
| `humanizeString` | `(input: string, out: []u8): usize` | Converts camel/snake/kebab/spaced identifier to a humanized sentence in `out` |

## Usage / 使用示例

```cp
import { humanizeString } from "@cap/humanize-string";

let buf: [128]u8 = undefined;
const n1 = humanizeString("fooBar-baz_qux", buf[0..128]);  // "Foo bar baz qux"
const n2 = humanizeString("  hello_world  ", buf[0..128]); // "Hello world"
```
