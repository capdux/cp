[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/vary

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:vary` ([https://github.com/jshttp/vary](https://github.com/jshttp/vary))

Zero-heap RFC 9110 HTTP `Vary` response header manipulation in pure CP (npm `vary` parity).

## Features

- `isValidFieldName(field)`: RFC 9110 token (`tchar`) validation including `"*"`
- `hasField(header, field)`: case-insensitive membership check with wildcard `"*"` support
- `append(header, field, dst)`: deduplicated comma-list append preserving first-seen casing and collapsing to `"*"` if either side contains `"*"`
- `appendMany(header, fields, dst)`: batch append for multiple fields

## Usage

```cp
import { append, hasField } from "@cap/vary";

let dst: [128]u8 = undefined;
const n = append("Accept-Encoding", "Origin, accept-encoding", dst[0..128]);
// dst[0..n] => "Accept-Encoding, Origin"
```
