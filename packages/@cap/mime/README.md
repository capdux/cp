# `@cap/mime`

工业级、纯 CP 实现、零外部依赖、零堆分配的 IANA MIME 媒体类型查找、反向扩展名映射、默认字符集判定与可压缩性分析库（Clean-Room 自主实现，基于公开 IANA Media Types 标准注册表），对标 npm `mime-types` 与 Rust `mime_guess` crate。

## 核心特性

- **100% 纯 CP & 零外部依赖**：无 C/FFI、无 `unsafe`、无堆分配，全部映射表以编译期静态只读表存储，运行期零动态内存开销。
- **覆盖 100+ 生产高频 IANA 扩展名**：完整覆盖 Web 前端、字体、音视频、Office/PDF 文档、压缩归档、平台安装包/二进制与主流源码文件。
- **智能路径与大小写兼容**：支持纯扩展名（`"json"`）、带点扩展名（`".JSON"`）及完整 POSIX/Windows 路径（`"/a/b/file.min.js"`），精准区分隐藏文件与无扩展名路径。
- **参数自动剥离与历史别名兼容**：反向查扩展名与属性判定自动剥离 `; charset=...` 参数，兼容 `application/javascript`、`text/xml`、`text/yaml`、`image/x-icon` 等常见历史 MIME 别名。
- **`Content-Type` 响应头零分配合成**：对标 npm `mime.contentType()`，自动补全 `; charset=utf-8`，保留已有 `charset=` 参数，严格防护小缓冲区越界。

## 类型与错误定义

### `MimeError`

```typescript
pub err MimeError {
    BufferTooSmall, // 输出缓冲区容量不足
    InvalidMime,    // 无效的 MIME 类型或无法识别的扩展名/路径
}
```

### `MimeInfo`

```typescript
pub struct MimeInfo {
    mime: []const u8;         // 规范 IANA MIME 类型（如 "text/html"）
    default_ext: []const u8;  // 规范主扩展名，不含前导点（如 "html"）
    compressible: bool;       // 是否适合 HTTP 压缩（gzip / brotli / zstd）
    utf8_charset: bool;       // 默认是否需要追加 utf-8 字符集
}
```

## API 文档

- **`pub fn lookupByExt(ext_or_path: []const u8): ([]const u8)?`**  
  根据扩展名或文件路径查找规范 MIME 类型（支持 `"json"`、`".json"`、`"/a/b/file.min.js"`，大小写不敏感）。未知扩展名返回 `null`。

- **`pub fn lookupOrOctetStream(ext_or_path: []const u8): []const u8`**  
  根据扩展名或文件路径查找 MIME 类型，未命中时回退返回 `"application/octet-stream"`。

- **`pub fn lookupInfo(ext_or_path: []const u8): MimeInfo?`**  
  根据扩展名或文件路径查找完整的 `MimeInfo` 元数据结构体。

- **`pub fn lookupInfoByMime(mime: []const u8): MimeInfo?`**  
  根据 MIME 类型（自动剥离 `; ...` 参数，大小写不敏感）查找完整的 `MimeInfo` 元数据。

- **`pub fn extensionForMime(mime: []const u8): ([]const u8)?`**  
  根据 MIME 类型反向查找规范主扩展名（不含前导点，如 `"text/html; charset=utf-8"` → `"html"`）。

- **`pub fn defaultCharset(mime: []const u8): ([]const u8)?`**  
  返回给定 MIME 类型的默认字符集：文本（`text/*`）、JSON、JavaScript、XML、SVG、YAML、`+json`、`+xml` 等返回 `"utf-8"`，二进制格式返回 `null`。

- **`pub fn isCompressible(mime: []const u8): bool`**  
  按 IANA / `mime-db` 约定判断 MIME 类型是否适合传输压缩：`text/*`、JSON、JS、XML、SVG、WASM、CSV、TTF/OTF 等返回 `true`；已压缩格式（PNG/JPG/WebP/AVIF/WOFF2/ZIP/GZ/ZST/MP4/MP3 等）返回 `false`。

- **`pub fn contentTypeHeader(ext_or_mime: []const u8, out: []u8): ([]const u8) ! MimeError`**  
  将扩展名、文件路径或 MIME 类型格式化为完整 HTTP `Content-Type` 响应头值写入 `out` 缓冲区。对文本/JSON/JS/XML 等自动补全 `; charset=utf-8`；若输入已包含 `charset=` 则保持原样；缓冲区不足返回 `MimeError.BufferTooSmall`，无法识别返回 `MimeError.InvalidMime`。

## 使用示例

### 示例 1：静态文件服务器的路径 MIME 识别与兜底

```typescript
import { lookupByExt, lookupOrOctetStream } from "@cap/mime";
import { print } from "cap:io";

pub fn main() {
    const js_mime = lookupByExt("/static/dist/app.min.js") ?? "application/octet-stream";
    const css_mime = lookupByExt(".CSS") ?? "application/octet-stream";
    const fallback = lookupOrOctetStream("/var/data/model.unknown");

    print(js_mime);   // text/javascript
    print(css_mime);  // text/css
    print(fallback);  // application/octet-stream
}
```

### 示例 2：反向查扩展名、字符集与 HTTP 压缩决策

```typescript
import { extensionForMime, defaultCharset, isCompressible } from "@cap/mime";
import { print } from "cap:io";

pub fn main() {
    const ext = extensionForMime("application/json; charset=utf-8") ?? "bin";
    const cs = defaultCharset("image/svg+xml") ?? "none";
    const can_gzip_wasm = isCompressible("application/wasm");
    const can_gzip_png = isCompressible("image/png");

    if (can_gzip_wasm && !can_gzip_png) {
        print(ext); // json
        print(cs);  // utf-8
    }
}
```

### 示例 3：零分配生成 HTTP `Content-Type` 响应头

```typescript
import { contentTypeHeader, MimeError } from "@cap/mime";
import { print } from "cap:io";

pub fn main() {
    let buf: [128]u8 = [0; 128];

    const h1 = contentTypeHeader("index.html", buf[0..]) catch unreachable;
    print(h1); // text/html; charset=utf-8

    const h2 = contentTypeHeader("application/json", buf[0..]) catch unreachable;
    print(h2); // application/json; charset=utf-8

    const h3 = contentTypeHeader("photo.webp", buf[0..]) catch unreachable;
    print(h3); // image/webp

    let tiny: [4]u8 = [0; 4];
    * = contentTypeHeader("json", tiny[0..]) catch e => {
        if (e == MimeError.BufferTooSmall) {
            print("Buffer too small");
        }
    };
}
```

## 许可协议 (License)

MIT License — Clean-room implementation based on the public IANA Media Types registry.
