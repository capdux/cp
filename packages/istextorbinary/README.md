[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/mime-sniff

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:istextorbinary` ([https://github.com/bevry/istextorbinary](https://github.com/bevry/istextorbinary)) / `crates.io:content_inspector` ([https://github.com/sharkdp/content_inspector](https://github.com/sharkdp/content_inspector))（原库名 `istextorbinary` → CP 包名 `@cap/mime-sniff`）

Zero-allocation WHATWG MIME Sniffing Standard binary/text detector and HTML/XML/JSON/SVG content sniffer for CP.

## Features

- **Zero heap allocation**: inspects up to the first 8192 bytes directly from `[]const u8`.
- **Binary vs Text classification**: `isBinary` / `isText` with UTF-8/UTF-16 BOM awareness and NUL/control-byte inspection.
- **Structured text sniffing**: `sniffContentType`, `contentTypeToMime`, and `sniffMime` recognizing HTML, SVG, XML, JSON, plain text, and binary payloads.

## Usage

```cp
import { sniffContentType, sniffMime, CT_HTML } from "@cap/mime-sniff";

const ct = sniffContentType("<!DOCTYPE html><html><body>Hi</body></html>"); // CT_HTML
const mime = sniffMime("{\"ok\":true}"); // "application/json"
```
