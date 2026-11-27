[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# prepend-http

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:prepend-http` ([https://github.com/sindresorhus/prepend-http](https://github.com/sindresorhus/prepend-http))

Zero-allocation utility to prepend `https://` or `http://` to humanized URLs for CP — clean-room MIT parity with npm `prepend-http` v4.

纯 CP 零堆分配 URL 协议补全库（对标 npm `prepend-http` v4），自动修剪首尾空白并智能识别 `scheme://`、`//`、`mailto:`、`./`、`../`、`/`，仅在缺失协议时补全 `https://` 或 `http://`。

## Install / 安装

```bash
cap add prepend-http
```

## API

| Symbol | Signature | Description |
| --- | --- | --- |
| `prependHttp` | `(url: string, https: bool, out: []u8): usize` | Trims whitespace and prepends `https://` or `http://` unless already having a scheme, protocol-relative, or relative path |
| `hasScheme` | `(url: string): bool` | Checks whether `url` already has an explicit `scheme://` or `mailto:` prefix |
| `isProtocolRelative` | `(url: string): bool` | Checks whether `url` starts with protocol-relative `//` |

## Usage / 使用示例

```cp
import { prependHttp, hasScheme, isProtocolRelative } from "prepend-http";

let buf: [128]u8 = undefined;
const n1 = prependHttp("todomvc.com", true, buf[0..128]);       // "https://todomvc.com"
const n2 = prependHttp("  localhost:3000 ", false, buf[0..128]); // "http://localhost:3000"
const n3 = prependHttp("//example.com", true, buf[0..128]);      // "//example.com"
```
