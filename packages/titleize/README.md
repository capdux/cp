[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/titleize

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:titleize` ([https://github.com/sindresorhus/titleize](https://github.com/sindresorhus/titleize))

Zero-allocation string title-case formatter for CP — clean-room MIT parity with npm `titleize` v3.

纯 CP 零堆分配标题大小写转换库（对标 npm `titleize` v3），将空白或标点后的每个单词首字母大写、单词内部字母小写，并提供首字符大写工具 `upperCaseFirst`。

## Install / 安装

```bash
cap add @cap/titleize
```

## API

| Symbol | Signature | Description |
| --- | --- | --- |
| `titleize` | `(input: string, out: []u8): usize` | Capitalizes each word's leading letter and lowercases interior letters |
| `upperCaseFirst` | `(input: string, out: []u8): usize` | Uppercases the first character and preserves the remaining characters |

## Usage / 使用示例

```cp
import { titleize, upperCaseFirst } from "@cap/titleize";

let buf: [128]u8 = undefined;
const n1 = titleize("unicorns and rainbows", buf[0..128]); // "Unicorns And Rainbows"
const n2 = titleize("FOO-BAR", buf[0..128]);               // "Foo-Bar"
const n3 = upperCaseFirst("helloWorld", buf[0..128]);      // "HelloWorld"
```
