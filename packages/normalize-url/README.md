[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# normalize-url

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:normalize-url` ([https://github.com/sindresorhus/normalize-url](https://github.com/sindresorhus/normalize-url))

Zero-allocation URL normalization utility for CP — clean-room MIT parity with npm `normalize-url`. Composes `@cap/prepend-http` with zero duplication.

纯 CP 零堆分配 URL 规范化库（对标 npm `normalize-url`），跨包复用 `@cap/prepend-http`，支持自动补全协议、协议与主机名小写化、去除默认端口（`http:80` / `https:443` / `ftp:21`）、去除 `www.`、折叠路径重复 `//`、解析 `.`/`..`、按需去除尾部 `/` 与 `#hash`。

## Install / 安装

```bash
cap add normalize-url
```

## Dependencies / 跨包复用

- `@cap/prepend-http`: 自动补全协议与协议相对 URL 检测

## API

| Symbol | Signature | Description |
| --- | --- | --- |
| `NormalizeUrlOptions` | `struct { default_https, strip_www, strip_hash, remove_trailing_slash, remove_default_port, normalize_protocol }` | Normalization options struct |
| `defaultNormalizeUrlOptions` | `(): NormalizeUrlOptions` | Returns default options (`default_https: false`, `strip_www: true`, `strip_hash: false`, `remove_trailing_slash: true`, `remove_default_port: true`, `normalize_protocol: true`) |
| `normalizeUrl` | `(url: string, out: []u8): usize` | Normalizes `url` with default options into `out` |
| `normalizeUrlWithOptions` | `(url: string, opts: NormalizeUrlOptions, out: []u8): usize` | Normalizes `url` with custom `NormalizeUrlOptions` into `out` |

## Usage / 使用示例

```cp
import { normalizeUrl, normalizeUrlWithOptions, defaultNormalizeUrlOptions } from "normalize-url";

let buf: [256]u8 = undefined;
const n1 = normalizeUrl("HTTP://WWW.Example.COM:80/foo//bar/../baz/", buf[0..256]);
// "http://example.com/foo/baz"

let opts = defaultNormalizeUrlOptions();
opts.default_https = true;
opts.strip_hash = true;
const n2 = normalizeUrlWithOptions("www.sindresorhus.com/a/./b/#top", opts, buf[0..256]);
// "https://sindresorhus.com/a/b"
```
