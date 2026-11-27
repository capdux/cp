[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：吸收自 **npm** [`bowser`](https://github.com/bowser-js/bowser)（`MIT`）（包名 `bowser`）。

# `bowser`

Zero-allocation User-Agent parser in pure CP (clean-room MIT implementation inspired by npm `bowser`).

## Features

- **`PlatformType`** — `Desktop`, `Mobile`, `Tablet`, `Bot`, `Unknown`.
- **`ParsedUA`** — Zero-copy slices for `browser_name`, `browser_version`, `os_name`, `os_version`, `engine_name`, and `platform`.
- **`parseUserAgent(ua)`** — Recognizes Chrome, Edge, Firefox, Safari, Opera, Brave, curl, Googlebot; Windows, macOS, Linux, Android, iOS; Blink, Gecko, WebKit.

## Usage

```cp
import { PlatformType, ParsedUA, parseUserAgent } from "bowser";

const ua: ParsedUA = parseUserAgent("Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.6099.109 Safari/537.36");
```

## License

MIT
