[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/filenamify

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:filenamify` ([https://github.com/sindresorhus/filenamify](https://github.com/sindresorhus/filenamify))

Zero-allocation cross-platform safe filename sanitizer for CP — clean-room MIT parity with npm `filenamify` v6.

纯 CP 零堆分配跨平台安全文件名清洗库（完整对标 npm `filenamify` v6），自动替换非法字符与控制字符、合并重复替换符与连续点号、剥离首尾替换符及尾部点/空格、拦截 Windows 保留设备名并按 UTF-8 安全边界截断。

## Install / 安装

```bash
cap add @cap/filenamify
```

## API

| Symbol | Signature | Description |
| --- | --- | --- |
| `filenamify` | `(input: string, out: []u8): usize` | Sanitizes `input` with default `replacement = "!"` and `max_len = 100` |
| `filenamifyWithOptions` | `(input: string, replacement: string, max_len: usize, out: []u8): usize` | Sanitizes `input` with custom replacement string and max byte length |
| `isReservedFilename` | `(name: string): bool` | Checks Windows reserved device names (`CON`, `PRN`, `AUX`, `NUL`, `COM1`..`COM9`, `LPT1`..`LPT9`, with or without extensions) |

## Usage / 使用示例

```cp
import { filenamify, filenamifyWithOptions, isReservedFilename } from "@cap/filenamify";

let buf: [128]u8 = undefined;
const n1 = filenamify("<foo/bar:baz*>", buf[0..128]);          // "foo!bar!baz"
const n2 = filenamifyWithOptions("a:b?c", "_", 100, buf[0..128]); // "a_b_c"
const n3 = filenamify("con.txt", buf[0..128]);                  // "con.txt!"
const r = isReservedFilename("LPT9.log");                       // true
```
