[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# unflatten

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:unflatten` ([https://github.com/hughsk/flat](https://github.com/hughsk/flat))

Zero-allocation nested JSON flatten & unflatten library for CP — clean-room MIT parity with npm `flat` / `unflatten`.

纯 CP 零堆分配嵌套 JSON 扁平化与反扁平化库（对标 npm `flat` / `unflatten`），支持自定义分隔符、深度截断（`max_depth`）、数组保留模式（`safe`）、共享前缀树合并与双向无损往返。

## Install / 安装

```bash
cap add unflatten
```

## API

| Symbol | Signature | Description |
| --- | --- | --- |
| `FlatOptions` | `struct { delimiter: u8, max_depth: usize, safe: bool }` + `.defaults()` | Configuration for flatten/unflatten |
| `FlatEntry` | `struct { key: [96]u8, key_len: usize, val: [128]u8, val_len: usize }` | Fixed-size stack key-value entry |
| `FlatTable` | `struct` + `.new` / `.set` / `.get` / `.count` | Zero-allocation flat table (up to 48 entries) |
| `flattenJson` | `(json_in: string, opts: FlatOptions, out_table: *FlatTable): i64` | Flatten nested JSON object/array into `FlatTable` |
| `flattenToJson` | `(json_in: string, opts: FlatOptions, out: []u8): i64` | Flatten nested JSON directly into a flat JSON object string |
| `unflattenJson` | `(flat_json: string, opts: FlatOptions, out: []u8): i64` | Reconstruct nested JSON object/arrays from a flat JSON object |

## Usage / 使用示例

```cp
import { FlatOptions, flattenToJson, unflattenJson } from "unflatten";

const opts = FlatOptions.defaults();
let flatBuf: [256]u8 = undefined;
const flen = flattenToJson("{\"a\":{\"b\":[10,20],\"c\":\"hi\"}}", opts, flatBuf[0..256]);
// flatBuf[0..flen] == "{\"a.b.0\":10,\"a.b.1\":20,\"a.c\":\"hi\"}"

let nestedBuf: [256]u8 = undefined;
const nlen = unflattenJson(flatBuf[0..(flen as usize)], opts, nestedBuf[0..256]);
// nestedBuf[0..nlen] == "{\"a\":{\"b\":[10,20],\"c\":\"hi\"}}"
```
