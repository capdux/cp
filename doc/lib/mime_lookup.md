# cap:mime_lookup

> 源码吸收自 `/data/dev/bun/src/http_types/{MimeType,mime_type_list_enum}.cp`，提供纯 CP、零外部依赖、零堆分配的 MIME 类型查表与 `Content-Type` 头部解析器。

## 导入方式

```cp
import { MimeCategory, ContentType, mimeFromExtension, categoryFromMime } from "cap:mime_lookup";
```

## 核心类型与 API

### `MimeCategory`

12 种主分类：`text`、`javascript`、`json`、`wasm`、`image`、`audio`、`video`、`font`、`archive`、`application`、`multipart`、`other`。

### `ContentType`

字段：`media_type: []const u8`、`type_part: []const u8`、`subtype: []const u8`、`charset: ([]const u8)?`、`boundary: ([]const u8)?`。

- `ContentType.parse(header: []const u8): ContentType?`：零拷贝解析 `type/subtype; charset=...; boundary="..."`（忽略参数名大小写，自动剥离外层双引号并支持引号内分号）。
- `isText(): bool`：是否为 `text/*`。
- `isJson(): bool`：是否为 `application/json`、`text/json` 或 `*+json`。
- `isMultipart(): bool`：是否为 `multipart/*`。
- `isCompressible(): bool`：判断是否适合 HTTP 压缩（`text/*`、JSON、XML/SVG、JavaScript、WASM、TOML/YAML、TTF/OTF 等）。

### 查表与归类函数

- `mimeFromExtension(ext_or_path: []const u8): []const u8`：自动提取末段扩展名（支持路径/URL 查询串、大小写无关），覆盖 65+ 高频 Web/系统扩展名，未知回退 `"application/octet-stream"`。
- `categoryFromMime(mime: []const u8): MimeCategory`：将 MIME 字符串映射到 `MimeCategory`。

## 使用示例

```cp
import { ContentType, mimeFromExtension, categoryFromMime } from "cap:mime_lookup";

fn example() {
    const mime = mimeFromExtension("/static/bundle.min.JS?v=2");
    * = categoryFromMime(mime); // MimeCategory.javascript

    if (ct := ContentType.parse("multipart/form-data; boundary=\"---cp-bound\"")) {
        * = ct.isMultipart(); // true
        * = ct.boundary;      // "---cp-bound"
    }
}
```
