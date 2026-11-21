# `cap:mime_type` — 纯 CP MIME 类型分类与扩展名映射引擎（吸收自 Bun `MimeType`）

## 概述

`cap:mime_type`（亦支持 `cap.mime_type` 动态挂载）从 Bun `src/http_types/MimeType.cp` 吸收并纯 CP 化：提供 MIME `Category` 语义分类（`isCode` / `isTextLike` / `autosetFilename`）、`classifyMime(mime_str)`（带 `; charset=` 参数剥离与大小写不敏感匹配）、以及零堆扩展名/文件路径反查 `byExtension` / `byPath`。

## 核心类型与 API

- `Category`: `none`、`image`、`text`、`html`、`font`、`other`、`css`、`json`、`audio`、`video`、`javascript`、`wasm`、`application` 等
- `classifyMime(mime_str) -> Category`
- `byExtension(ext) -> ([]const u8)?`
- `byPath(path) -> ([]const u8)?`

## 示例

```cp
import { byPath, classifyMime } from "cap:mime_type";

const ct = byPath("/assets/app.wasm").?; // "application/wasm"
const cat = classifyMime(ct);
```
