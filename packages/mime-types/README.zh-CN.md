[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# `mime-types`

CP 零堆分配 MIME 类型、文件扩展名、默认字符集、`Content-Type` 响应头构造与可压缩性判断库 —— 对齐 npm [`mime-types`](https://github.com/jshttp/mime-types)（v2.1）的纯 CP MIT 净室实现。

## 安装与导入

```bash
cap add mime-types
```

```cp
import {
    MimeParts,
    parseMimeType,
    lookup,
    extension,
    charset,
    contentType,
    isCompressible,
} from "mime-types";
```

## 核心 API

| 符号 | 签名 | 说明 |
| :--- | :--- | :--- |
| `MimeParts` | `struct { valid: bool, essence: string, type_part: string, subtype: string, charset_val: string }` | 零拷贝 MIME 解析结果（切片直接引用输入字符串） |
| `parseMimeType` | `(raw: string): MimeParts` | 解析 `"text/html; charset=utf-8"`，拆出主类型、子类型、essence 与 charset 参数 |
| `lookup` | `(path_or_ext: string): string?` | 根据文件路径、文件名或扩展名（如 `"app.wasm"`、`".json"`、`"png"`）查 MIME 类型，未命中返回 `null` |
| `extension` | `(mime_type: string): string?` | 根据 MIME 类型反查默认文件扩展名（不含前导点），未命中返回 `null` |
| `charset` | `(mime_type: string): string?` | 返回文本/JSON/XML/JS/SVG 等类型的默认字符集（如 `"UTF-8"`），不适用则返回 `null` |
| `contentType` | `(type_or_ext: string, out: []u8): usize` | 构造完整 `Content-Type` 头部（自动追加 `; charset=utf-8`）写入 `out`，返回写入字节数（未知类型返回 `0`） |
| `isCompressible` | `(mime_type: string): bool` | 判断该 MIME 类型是否适合启用 gzip/brotli/zstd 压缩 |

## 代码示例

### 1. 静态文件服务的 `Content-Type` 与压缩决策

```cp
import { print } from "cap:io";
import { lookup, contentType, isCompressible } from "mime-types";

fn main() {
    const mime = lookup("/static/bundle.js");
    if (mime != null) {
        print("mime:", mime.?);                    // "text/javascript"
        print("compress:", isCompressible(mime.?)); // true
    }

    let hdr: [128]u8 = undefined;
    const n = contentType("index.html", hdr[0..128]);
    if (n > 0) {
        print("Content-Type:", hdr[0..n]);         // "text/html; charset=utf-8"
    }
}
```

### 2. 解析请求头 MIME 与扩展名反查

```cp
import { print } from "cap:io";
import { parseMimeType, extension, charset } from "mime-types";

fn main() {
    const parts = parseMimeType("application/json; charset=utf-8");
    if (parts.valid) {
        print("essence:", parts.essence);     // "application/json"
        print("type:", parts.type_part);      // "application"
        print("subtype:", parts.subtype);     // "json"
        print("charset:", parts.charset_val); // "utf-8"
    }

    const ext = extension("image/webp");
    if (ext != null) { print("ext:", ext.?); } // "webp"

    const cs = charset("text/markdown");
    if (cs != null) { print("default charset:", cs.?); } // "UTF-8"
}
```

## 与源包差异及性能亮点

- **零堆分配**：内置静态只读映射表，`lookup`、`extension`、`charset` 直接返回可选静态切片 `string?`，`contentType` 零分配写入调用方 `[]u8`。
- **强类型可选返回**：采用 CP 原生 `string?`（未命中返回 `null`），取代 JS 的 `string | false` 联合类型。
- **内置可压缩性判断**：原生提供 `isCompressible()`，覆盖 `text/*`、`+json`、`+xml`、`+text`、`application/wasm`、`image/svg+xml` 等主流类型。
