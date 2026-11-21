# CP 标准库：MIME 媒体类型、内容协商与扩展名数据库手册 (`cap:mime`)

## 一、 架构总览与设计准则

`cap:mime` 是 CP 语言官方内置的纯 CP **RFC 6838 MediaType 解析/格式化、RFC 9110 HTTP `Accept` 内容协商（Content Negotiation）与全量扩展名双向数据库**，**零 C 依赖、零 `std.posix`、零 `@import("bun")`**，专为 CP Web 框架、静态资源服务器、OpenAPI 路由与文件/邮件网关打造。既可通过 `import { ... } from "cap:mime"` 导入，也支持 `import cap from "cap"; cap.mime.*` 动态懒挂载调用。

### 模块文件拓扑与谱系来源

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/mime/mime.cp` | `// Source: cp/lib/mime/mime.cp (...)` | `MimeError`、`MimeParam`、`MediaType`、`ContentNegotiator` |
| `lib/mime/db.cp` | `// Source: cp/lib/mime/db.cp (...)` | `MimeDb`（110+ 扩展名双向映射与 `isText`/`isCompressible` 判定） |
| `lib/mime/index.cp` | `// Source: cp/lib/mime/index.cp (...)` | `cap:mime` 统一公开导出入口 |

---

## 二、 核心 API 详解

### 1. 错误集与基础类型

- `MimeError`：
  - `InvalidMediaType`：空字符串、缺少 `/`、多 `/`、空白围绕 `/` 或非法 token 字符（如结尾孤立 `+`）。
  - `InvalidParameter`：参数缺少 `=`、空参数名、空未加引号值或未闭合引号字符串。
  - `InvalidQuality`：`Accept` 中的 `q` 权重非法（非浮点数或超出 `[0.0, 1.0]`）。
  - `OutOfMemory`：底层缓冲区分配失败。
- `MimeParam`：
  - `name: []const u8`（小写归一化参数名）
  - `value: []const u8`（已反转义参数值）

### 2. `MediaType`（RFC 6838 媒体类型）

- **核心字段**：
  - `type: []const u8`（小写主类型，如 `"application"`）
  - `subtype: []const u8`（小写子类型，如 `"vnd.api+json"`）
  - `suffix: []const u8`（可选结构化后缀，如 `"+json"` / `"+xml"`，无则 `""`）
  - `params: []MimeParam`（有序参数切片）
- **生命周期与解析**：
  - `MediaType.parse(alloc, raw): MediaType ! MimeError`：支持大小写归一化、带引号参数反转义（`\"` / `\\`）、`+suffix` 提取与严格 token 校验。
  - `deinit(*_)`：释放内部持有的堆内存。
- **查询、匹配与格式化**：
  - `essence(): []const u8`：返回不含参数的 `"type/subtype"`。
  - `getParam(name): ([]const u8)?`：大小写不敏感查询参数值。
  - `charset(): ([]const u8)?`：`getParam("charset")` 快捷方法。
  - `matches(pattern): bool`：支持 `"*/*"`、`"type/*"`、`"type/subtype"` 及结构化后缀模式（`"+json"`、`"*+json"`、`"*/*+json"`、`"application/*+json"`）。
  - `toString(alloc): ([]u8) ! MimeError`：规范化输出 `"type/subtype; name=value"`（含非 token 值自动加引号与转义），调用方通过 `alloc.free` 释放。

### 3. `ContentNegotiator`（RFC 9110 `Accept` 内容协商）

- `ContentNegotiator.negotiateAccept(alloc, accept_header, available_types): ([]const u8)? ! MimeError`
  - 完整解析多个逗号分隔媒体范围与 `q=0.0..1.0` 权重（默认 `1.0`）。
  - `q=0` 显式排除对应媒体类型（精确范围 `text/plain;q=0` 覆盖宽泛范围 `text/*;q=0.9`）。
  - 相同 `q` 值按 specificity 优先级排序：`type/subtype` > `type/*` > `*/*`，再按非 `q` 参数数量与服务端 `available_types` 声明顺序决定胜出者。
  - 无任何可接受匹配时返回 `null`。

### 4. `MimeDb`（扩展名 <-> MIME 数据库）

- 覆盖 Web、字体、音视频、压缩归档、办公文档、源码与 AI/ML 数据格式（`parquet`/`arrow`/`onnx`/`safetensors`/`gguf`/`jsonl` 等）共 110+ 常见映射。
- `MimeDb.fromExtension(ext_or_path): ([]const u8)?`：自动剥离目录路径与前导 `.`，大小写不敏感，支持复合扩展名（如 `"dir/app.tar.gz"` -> `"application/gzip"`、`".WASM"` -> `"application/wasm"`）。
- `MimeDb.toExtension(mime_type): ([]const u8)?`：自动剥离 `; charset=...` 参数后反查标准主扩展名。
- `MimeDb.isText(mime_type): bool`：识别 `text/*`、`+json`、`+xml`、`+yaml`、`+csv` 及 `application/json`、`application/javascript`、`application/toml` 等文本类型。
- `MimeDb.isCompressible(mime_type): bool`：精确区分可 gzip/brotli 压缩类型（文本、`application/wasm`、`font/ttf` 等）与已压缩格式（`image/png`、`video/mp4`、`font/woff2`、`application/zip`、`application/gzip` 等）。

---

## 三、 可运行代码示例

```typescript
const std = @import("std");
import { MediaType, ContentNegotiator, MimeDb } from "cap:mime";
import cap from "cap";

pub fn demoMime(): ! {
    const alloc = std.heap.page_allocator;

    let mt = MediaType.parse(alloc, "Application/VND.Api+JSON; charset=\"utf-8\"")?;
    defer mt.deinit();

    const ess = mt.essence();         // "application/vnd.api+json"
    const is_json = mt.matches("+json"); // true
    * = ess;
    * = is_json;

    const avail: []const []const u8 = ["application/xml", "application/json", "text/html"];
    const best = ContentNegotiator.negotiateAccept(
        alloc,
        "text/html;q=0.8, application/json;q=0.95, */*;q=0.1",
        avail,
    )?;
    * = best;

    const wasm_mime = cap.mime.MimeDb.fromExtension(".WASM"); // "application/wasm"
    * = wasm_mime;
}
```

---

## 零分配 MIME 扩展子模块（收敛自 `mime_*` / `multipart_boundary` / `content_disposition`）

- **RFC 5322 MIME 头部块解析与展开 (`headers.cp`)**：`MimeMessageSplit`、`MimeHeader`、`splitHeadersAndBody`、`MimeHeaderIter`、`findMimeHeader`、`countMimeHeader`。
- **WHATWG MIME 内容嗅探 (`sniff.cp`)**：`isBinaryDataByte`、`isTextData`、`sniffImageMime`、`sniffArchiveOrExecMime`、`sniffAudioVideoMime`、`sniffMime`。
- **RFC 7578 Multipart 边界解析 (`multipart.cp`)**：`isValidBoundary`、`MultipartPart`、`MultipartParser`。
- **RFC 6266 / RFC 5987 Content-Disposition (`disposition.cp`)**：`DispositionType`、`ContentDisposition`、`decodeRfc5987`、`sanitizeFilename`、`formatAttachment`。


---

## 扩展 MIME 查表与路径分类（已收敛至 `cap:mime`）

- **`lib/mime/lookup.cp`**：`MimeCategory`, `ContentType`, `mimeFromExtension`, `categoryFromMime`
- **`lib/mime/type.cp`**：`Category`, `classifyMime`, `byExtension`, `byPath`
