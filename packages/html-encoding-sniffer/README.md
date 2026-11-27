[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# html-encoding-sniffer

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:html-encoding-sniffer` ([https://github.com/jsdom/html-encoding-sniffer](https://github.com/jsdom/html-encoding-sniffer))（原库名 `html-encoding-sniffer` → CP 包名 `html-encoding-sniffer`）

Zero-allocation WHATWG HTML §13.2.3.2 encoding sniffer for CP — clean-room MIT implementation inspired by jsdom `html-encoding-sniffer`.

纯 CP 零堆分配 WHATWG HTML 字节流编码嗅探器（对标 `html-encoding-sniffer`），严格遵循 BOM → 传输层 `Content-Type` → 前 1024 字节 `<meta charset>` / `<meta http-equiv="content-type">`（跳过 HTML 注释）→ 默认编码回退的四级优先级，并内置 WHATWG Encoding Standard 别名规范化。

## Install / 安装

```bash
cap add html-encoding-sniffer
```

## API

### Structs & Functions / 核心类型与函数

| Symbol | Signature / Fields | Description |
| --- | --- | --- |
| `SniffOptions` | `struct { transport_layer_encoding: string, default_encoding: string }` | Transport-layer charset hint and fallback encoding (defaults to `"windows-1252"`) |
| `sniffHtmlEncoding` | `(bytes: []const u8, opts: SniffOptions): string` | Sniff HTML byte stream encoding per WHATWG HTML §13.2.3.2 |
| `normalizeEncodingLabel` | `(label: string): string` | Normalize WHATWG encoding label aliases (e.g. `utf8` → `UTF-8`, `latin1` → `windows-1252`, `gb2312` → `GBK`) |

## Usage / 使用示例

```cp
import { SniffOptions, sniffHtmlEncoding, normalizeEncodingLabel } from "html-encoding-sniffer";

const html = "<html><head><meta charset=\"utf-8\"></head></html>";
const enc = sniffHtmlEncoding(html, SniffOptions{ transport_layer_encoding: "", default_encoding: "" });
// enc == "UTF-8"
```
