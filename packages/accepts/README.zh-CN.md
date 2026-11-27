[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# accepts

纯 CP 实现的零分配 RFC 9110 HTTP 内容协商引擎，支持 `Accept`、`Accept-Encoding`、`Accept-Charset` 与 `Accept-Language` 请求头协商，底层组合 `negotiator`。

- **源包对齐**：对标 `npm` `accepts`（MIT，jshttp/accepts）
- **包版本**：`1.0.0`
- **内存模型**：全程零堆分配，直接基于只读 `[]const u8` 切片解析，返回候选切片或索引。

## 安装与导入

```bash
cap add accepts
```

```cp
import {
    AcceptHeaders,
    Accepts,
    parseQValue,
    negotiateEncoding,
    negotiateLanguage,
    negotiateMediaType,
    negotiateCharset,
    rankMediaTypes,
} from "accepts";
```

## 核心 API

| 符号 | 签名 | 说明 |
| --- | --- | --- |
| `AcceptHeaders` | `class AcceptHeaders { accept, accept_encoding, accept_charset, accept_language: []const u8 }` | 请求协商头集合；`AcceptHeaders.empty()` 创建空集合。 |
| `Accepts` | `class Accepts { headers: AcceptHeaders }` | 协商器封装，通过 `Accepts.new(headers)` 创建。 |
| `Accepts.types` / `typeIndex` | `fn types(available: []const []const u8): ([]const u8)?` / `fn typeIndex(available): i64` | 选择最匹配的媒体类型（或其索引，无匹配返回 `-1`）。 |
| `Accepts.encodings` / `encodingIndex` | `fn encodings(available: []const []const u8): ([]const u8)?` / `fn encodingIndex(available): i64` | 选择最匹配的内容编码（如 `br`、`gzip`、`identity`）。 |
| `Accepts.charsets` / `charsetIndex` | `fn charsets(available: []const []const u8): ([]const u8)?` / `fn charsetIndex(available): i64` | 选择最匹配的字符集（如 `utf-8`、`iso-8859-1`）。 |
| `Accepts.languages` / `languageIndex` | `fn languages(available: []const []const u8): ([]const u8)?` / `fn languageIndex(available): i64` | 选择最匹配的 BCP 47 语言标签（如 `zh-CN`、`en-US`、`en`）。 |
| `parseQValue` | `fn parseQValue(s_in: []const u8): i32` | 解析 RFC 9110 权重 `q`（`0.000..1.000`）为定点整数 `0..1000`（非法返回 `-1`）。 |
| `negotiateMediaType` | `fn negotiateMediaType(accept_header: []const u8, available: []const []const u8): i64` | 独立媒体类型协商函数，返回最佳索引或 `-1`。 |
| `negotiateEncoding` | `fn negotiateEncoding(accept_header: []const u8, available: []const []const u8): i64` | 独立内容编码协商函数，返回最佳索引或 `-1`。 |
| `negotiateCharset` | `fn negotiateCharset(accept_charset: []const u8, available: []const []const u8): i64` | 独立字符集协商函数，返回最佳索引或 `-1`。 |
| `negotiateLanguage` | `fn negotiateLanguage(accept_header: []const u8, available: []const []const u8): i64` | 独立语言协商函数，返回最佳索引或 `-1`。 |
| `rankMediaTypes` | `fn rankMediaTypes(accept_header: []const u8, available: []const []const u8, out_indices: []usize): usize` | 将所有可接受候选索引按权重与精确度降序写入 `out_indices`，返回写入数量。 |

## 使用示例

### 1. 使用 `Accepts` 执行多维度内容协商

```cp
import { AcceptHeaders, Accepts } from "accepts";

fn negotiateRequest(): void {
    let mut h = AcceptHeaders.empty();
    h.accept = "text/html, application/json;q=0.9, */*;q=0.1";
    h.accept_encoding = "br, gzip;q=0.8";
    h.accept_language = "zh-CN, en;q=0.7";

    const acc = Accepts.new(h);
    const avail_types: [2][]const u8 = ["application/json", "text/html"];
    const best_type = acc.types(avail_types[0..]); // Some("text/html")

    const avail_enc: [2][]const u8 = ["gzip", "br"];
    const best_enc = acc.encodings(avail_enc[0..]); // Some("br")
}
```

### 2. 对多个候选媒体类型排序

```cp
import { rankMediaTypes } from "accepts";

fn rankDemo(): void {
    const avail: [3][]const u8 = ["text/plain", "application/json", "text/html"];
    let mut ranked: [3]usize = [0; 3];
    const count = rankMediaTypes("application/json;q=0.9, text/html;q=1.0", avail[0..], ranked[0..]);
    // count == 2; ranked[0] == 2 ("text/html"), ranked[1] == 1 ("application/json")
}
```

## 与源包差异与安全亮点

- **零分配切片 API**：直接处理 `[]const u8` HTTP 头切片并使用 `0..1000` 定点 `q` 值，避免浮点误差与堆分配。
- **索引与可选值双接口**：提供 `*Index(): i64`（无匹配返回 `-1`）与 `([]const u8)?`（无匹配返回 `null`）两套无异常 API。
- **组合复用 `negotiator`**：底层直接复用 `negotiator` 的 RFC 9110 解析核心。
