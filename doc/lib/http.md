# CP 标准库：全栈工业级 HTTP/1.1 · HTTP/2 · HTTP/3 子系统使用手册 (`cap:http`)

> 参考来源：`/data/dev/bun/docs/runtime/http/{server,routing,cookies,tls}.mdx`, `/data/dev/bun/src/http/`, `/data/dev/bun/src/picohttp/` | 吸收日期：2026-11-22

## 一、 架构总览与设计哲学

`cap:http` 是 CP 语言官方内置的**全栈工业级 HTTP 协议子系统**，完整吸收自 Bun（`/data/dev/bun/src/http/`、`/data/dev/bun/src/http_types/`、`/data/dev/bun/src/picohttp/` 与 `/data/dev/bun/src/runtime/webcore/`），以 100% 纯现代 CP 语法重构，兼顾 **零拷贝（Zero-Copy）协议解析、零堆分配栈内缓冲（Small-Buffer Optimization）、确定性资源管理（`defer`）与多协议栈演进（HTTP/1.1 → H2/HPACK → H3/QUIC）**。

### 分层架构图

```text
┌────────────────────────────────────────────────────────────────────────────────────────────┐
│                                 cap:http 全栈分层架构                                       │
│                                                                                            │
│  ┌──────────────────────────────────────────────────────────────────────────────────────┐  │
│  │ L4 · 应用与 Web API 层（高频开发入口）                                                │  │
│  │   • serve(config) -> HttpServer  │  async fetch(url, options) -> Response            │  │
│  │   • Request / Response / json(data) / FetchCacheMode / FetchRedirect / RequestMode   │  │
│  └───────────────────────────────────────────┬──────────────────────────────────────────┘  │
│                                              ▼                                             │
│  ┌──────────────────────────────────────────────────────────────────────────────────────┐  │
│  │ L3 · HTTP 语义与 Web 编解码基础设施                                                   │  │
│  │   • Method (RFC 9110 / WebDAV 34 种方法)  • Headers (32 槽内联 + 动态扩容，大小写无关) │  │
│  │   • HeaderValueIterator (逗号列表迭代)    • ETag (Wyhash 生成 / 强弱 If-None-Match)   │  │
│  │   • MimeType (扩展名/路径极速映射)        • URLPath (路径/扩展名/查询串零拷贝切分)    │  │
│  │   • CookieMap (RFC 6265 解析 & Set-Cookie)• FormData (URL-Encoded & Multipart 边界)  │  │
│  │   • SignalStore / Signals (原子中止与状态)• InternalState / HTTPRequestBody / Cert   │  │
│  └───────────────┬───────────────────────────┬───────────────────────────┬──────────────┘  │
│                  ▼                           ▼                           ▼                 │
│  ┌───────────────────────────┐ ┌───────────────────────────┐ ┌──────────────────────────┐  │
│  │ L2-A · HTTP/1.1 传输与引擎 │ │ L2-B · HTTP/2 与 HPACK    │ │ L2-C · HTTP/3 与 QUIC    │  │
│  │  • HTTPClient (RFC 7230)  │ │  • H2ClientSession        │ │  • AltSvc (RFC 7838)     │  │
│  │    请求序列化 / Chunked 解码│ │    奇数流复用 / GOAWAY    │ │    h3=":443"; ma=...     │  │
│  │  • HTTPContext            │ │  • H2Stream (RFC 7540)    │ │  • H3ClientSession       │  │
│  │    64 槽 Keep-Alive 连接池 │ │    半关闭 / RST_STREAM    │ │    客户端双向流 (0,4,8…) │  │
│  │  • ProxyTunnel (CONNECT)  │ │  • FrameHeader (9B 帧头)  │ │  • H3Stream              │  │
│  │  • SendFile (内核零拷贝)   │ │    SettingsPayload (6B)   │ │    Control/QPACK/Bidi    │  │
│  │  • Decompressor (流解压)   │ │  • HpackCodec (RFC 7541)  │ │                          │  │
│  └─────────────┬─────────────┘ │    前缀整数/静态表/头块编解 │ └──────────────────────────┘  │
│                ▼               └───────────────────────────┘                               │
│  ┌──────────────────────────────────────────────────────────────────────────────────────┐  │
│  │ L1 · 底层零拷贝线协议解析层（_picohttp.cp）                                           │  │
│  │   • PicoHttpParser.parseRequest / parseResponse / findHeader (纯 CP 零拷贝切片)       │  │
│  │   • C-FFI 原生绑定：phr_parse_request / phr_parse_response / phr_decode_chunked      │  │
│  └──────────────────────────────────────────────────────────────────────────────────────┘  │
└────────────────────────────────────────────────────────────────────────────────────────────┘
```

### 模块目录拓扑

| 模块路径 | 核心职责 | 吸收来源（Bun 仓库） |
|---|---|---|
| `lib/http/_picohttp.cp` | PicoHTTPParser C-FFI 绑定与纯 CP 零拷贝 HTTP/1.x 报文解析 | `src/picohttp/picohttp.cp` |
| `lib/http/method.cp` | HTTP 方法枚举（34 种）、幂等性与请求体属性 | `src/http_types/Method.cp` |
| `lib/http/headers.cp` | 大小写不敏感 `Headers` 集合（32 槽栈内联 + 堆动态扩容，支持 `getAll` 多值提取） | `src/http/Headers.cp` |
| `lib/http/header_builder.cp` | 零拷贝连续线协议 `HeaderBuilder`（栈缓冲/堆扩容、span 索引、请求行/状态行生成） | `src/http/HeaderBuilder.cp` |
| `lib/http/header_parsers.cp` | 标准头解析器：`CacheControl`、`ContentDisposition`、`AcceptParser`、`RangeHeader`、`ContentRange` | `src/http/Headers.cp` |
| `lib/http/header_iterator.cp` | 逗号分隔 HTTP Header 值零拷贝迭代器 | `src/http/HeaderValueIterator.cp` |
| `lib/http/status.cp` | `HTTP_STATUS_TABLE`（64 项）与 `HTTPStatus` 状态码短语/语义判定工具 | `src/http/http.cp` |
| `lib/http/etag.cp` | Wyhash ETag 生成与强/弱 `If-None-Match` 校验 | `src/http_types/ETag.cp` |
| `lib/http/mime.cp` | 201 项完整 `MIME_DATABASE` 映射、压缩/文本/字符集判定与二进制 `sniffMagic` 嗅探 | `src/http_types/MimeType.cp` |
| `lib/http/cookie.cp` | RFC 6265 `CookieMap`、`CookieCodec`（`parseSetCookie`/`formatSetCookie`/`parseCookieHeader`）与 `HttpCookieJar`（域名/路径/Secure/过期/公共后缀防护） | `src/runtime/webcore/CookieMap.cp` |
| `lib/http/form_data.cp` | `x-www-form-urlencoded` 解析与 `multipart/form-data` 边界提取 | `src/runtime/webcore/FormData.cp` |
| `lib/http/sse.cp` | WHATWG Server-Sent Events 增量流式状态机 (`SseParser`) 与编码器 (`SseEncoder`)，适配 AI/LLM 流式输出 | `cp/lib/http/sse.cp` |
| `lib/http/multipart.cp` | RFC 7578 二进制安全 `MultipartBuilder`、`MultipartParser` 与 `MultipartBoundary` | `cp/lib/http/multipart.cp` |
| `lib/http/request.cp` / `response.cp` / `server.cp` | 高层 `Request` / `Response` / `HttpServer`、`serve` / `fetch` / `json` | `src/http/http.cp` |
| `lib/http/client.cp` | 工业级 HTTP/1.1 `HTTPClient`（URL 解析、请求格式化、Chunked 解码） | `src/http/http.cp` |
| `lib/http/wire_http.cp` | 工业级 `HttpRequestBuilder` 与 `HttpResponseParser`（`100-continue`、`obs-fold`、Chunked+解压） | `src/http/http.cp` |
| `lib/http/chunked.cp` | RFC 9112 §7.1 流式 `ChunkedDecoder`（hex size/extensions/trailers/`0\r\n\r\n`）与 `ChunkedEncoder` | `src/http/http.cp` |
| `lib/http/redirect.cp` | `RedirectPolicy` 自动重定向策略引擎（301/302/303/307/308 方法重写、URL 合并、循环检测、跨域敏感头剥离） | `src/http/http.cp` |
| `lib/http/context.cp` | `HTTPContext` 与 `HttpConnectionPool`（按 host:port 复用、空闲超时驱逐、最大连接数与健康检查） | `src/http/HTTPContext.cp` |
| `lib/http/proxy.cp` / `sendfile.cp` / `decompressor.cp` | `ProxyTunnel` / `HttpProxyTunnel`（Basic 认证与 `NoProxyMatcher`）、`SendFile`、与 `cap:pack` 协同的 `Decompressor` / `StreamingDecompressor` | `ProxyTunnel.cp` / `SendFile.cp` / `Decompressor.cp` |
| `lib/http/url_path.cp` / `encoding.cp` / `fetch_modes.cp` / `signals.cp` / `internal_state.cp` / `request_body.cp` / `cert.cp` | 路径解析、`ContentEncodingNegotiator` (`Accept-Encoding` q-value 协商)、Fetch 模式枚举、原子中止信号、状态机与证书信息 | `src/http/` & `src/http_types/` |
| `lib/http/h2/frame.cp` / `client.cp` / `hpack.cp` | RFC 7540 二进制帧与流控（`WINDOW_UPDATE`/`RST_STREAM`/`PING`/`GOAWAY`）、多路复用流状态机、RFC 7541 完整 HPACK（61 项静态表 + 257 项 Huffman + 动态表） | `H2FrameParser.cp` / `h2_client/ClientSession.cp` / `lshpack.cp` |
| `lib/http/h3/alt_svc.cp` / `client.cp` / `encode.cp` | RFC 7838 `Alt-Svc` 升级头解析、HTTP/3 QUIC 会话与双向流、RFC 9114 HTTP/3 请求伪头部（`:method`/`:scheme`/`:authority`/`:path`）与 QPACK 静态索引零分配规范化编码（`encodeH3RequestHeaders`） | `h3_client/AltSvc.cp` / `ClientSession.cp` / `encode.cp` |
| `lib/http/accept.cp` / `acceptenc.cp` / `acceptlang.cp` / `acceptranges.cp` | RFC 9110 内容协商（`Accept` / `Accept-Encoding` / `Accept-Language` / `Accept-Ranges`）零分配解析与协商器 | `src/http/` |
| `lib/http/allow.cp` / `altsvc.cp` / `auth.cp` | RFC 9110 `Allow` 方法集、RFC 7838 `Alt-Svc` 编解码、RFC 7617/6750 Basic/Bearer 认证编解码 | `src/http/` |
| `lib/http/cache.cp` / `cachectrl.cp` / `cachekey.cp` | RFC 9111 缓存策略与新鲜度评估（`HttpCacheEvaluator`）、`Cache-Control` 指令迭代、缓存键规范化 | `src/http/` |
| `lib/http/clearsite.cp` / `clienthints.cp` | W3C `Clear-Site-Data` 解析器与 `Sec-CH-UA` Client Hints 解析器 | `src/http/` |
| `lib/http/contentenc.cp` / `contentlang.cp` / `contentrange.cp` | RFC 9110 `Content-Encoding` 编码栈、BCP 47 `Content-Language` 解析与规范化、`Content-Range` 编解码 | `src/http/` |
| `lib/http/contenttype.cp` / `cookiejar.cp` / `cookiematch.cp` | RFC 9110 `Content-Type` 参数解析、RFC 6265 Cookie 扩展解析与域/路径/前缀发送策略校验 | `src/http/` |
| `lib/http/csp.cp` / `date.cp` / `digest.cp` / `digestauth.cp` | W3C CSP 指令迭代与查询、RFC 9110 `HTTP-date` 编解码、RFC 9530 `Digest`/`Want-Digest`、RFC 7616 Digest Auth | `src/http/` |
| `lib/http/earlyhints.cp` / `etaggen.cp` / `expect.cp` | RFC 8297 103 Early Hints、`FNV-1a` 强弱 ETag 生成与校验、RFC 9110 `Expect: 100-continue` 裁定 | `src/http/` |
| `lib/http/fetchmeta.cp` / `form.cp` / `forwarded.cp` / `freshness.cp` / `host.cp` | Fetch 元数据掩码协商、WHATWG Form URL-Encoded、RFC 7239 `Forwarded`/`XFF`、RFC 9111 新鲜度计算、RFC 9110 `Host` 校验 | `src/http/` |
| `lib/http/ifmatch.cp` / `keepalive.cp` / `link.cp` / `maxforwards.cp` | RFC 9110 条件请求（`If-Match`/`If-None-Match`）、`Keep-Alive` 参数、RFC 8288 `Link` 头、RFC 9110 `Max-Forwards` 跳数裁定 | `src/http/` |
| `lib/http/methodparse.cp` / `origin.cp` / `permissions.cp` / `prefer.cp` | RFC 9110 36 种 HTTP 方法与 `MethodSet`、WHATWG `Origin`、W3C `Permissions-Policy`、RFC 7240 `Prefer` 头解析 | `src/http/` |
| `lib/http/proxyurl.cp` / `range.cp` / `ratelimit.cp` / `retryafter.cp` / `router.cp` | HTTP/SOCKS 代理 URL 选择、RFC 9110 `Range` 字节区间、IETF `RateLimit`、`Retry-After`、Radix Trie `HttpRouter` | `src/http/` |
| `lib/http/secfetch.cp` / `security.cp` / `servertiming.cp` / `siginput.cp` | W3C `Sec-Fetch-*` 元数据、HSTS/Referrer/COOP/COEP 安全头、`Server-Timing`、RFC 9421 `Signature-Input` | `src/http/` |
| `lib/http/statusparse.cp` / `te.cp` / `trailer.cp` / `types.cp` | HTTP 状态码分类/重试判定、RFC 9110 `TE`/`Transfer-Encoding`、`Trailer` 禁传字段校验、核心 `http_types` 集合 | `src/http/` |
| `lib/http/upgrade.cp` / `useragent.cp` / `vary.cp` / `via.cp` / `warning.cp` | RFC 9110 `Upgrade`/`Connection`、`User-Agent` 解析、`Vary` 合并、`Via` 代理链、RFC 7234 `Warning` 编解码 | `src/http/` |

---

## 二、 核心 API 详解与代码示例

所有公开类型与函数均可通过 `"cap:http"` 统一按需导入。

### 1. HTTP 方法 (`Method`)

支持 RFC 9110 及 WebDAV 全部 34 种标准方法，提供大小写不敏感解析、幂等性判定与请求体语义检查：

```typescript
import { Method } from "cap:http";

pub fn demoMethod() {
    const m = Method.fromString("patch");
    if (method := m) {
        const name = method.toString();          // "PATCH"
        const idempotent = method.isIdempotent(); // false
        const has_body = method.hasRequestBody(); // true
    }

    // 幂等方法：GET, HEAD, PUT, DELETE, OPTIONS, TRACE, QUERY
    const is_get_safe = Method.GET.isIdempotent();     // true
    const get_has_body = Method.GET.hasRequestBody();  // false
}
```

---

### 2. 大小写不敏感头部集合 (`Headers` & `HeaderValueIterator`)

`Headers` 采用**小对象栈内联优化（SBO）**：前 32 条 Header 直接存放在结构体内部固定数组，零堆内存分配；当条目超过 32 条时自动通过 `allocator` 扩容为 64、128… 条动态切片，并在 `defer headers.deinit()` 时确定性释放。

```typescript
import { Headers, HeaderValueIterator } from "cap:http";
const std = @import("std");

pub fn demoHeaders() {
    let headers = Headers.init(std.heap.page_allocator);
    defer headers.deinit();

    headers.append("Content-Type", "application/json; charset=utf-8");
    headers.append("X-Request-Id", "req-001");
    headers.set("x-request-id", "req-002"); // 大小写无关覆盖

    if (ct := headers.get("CONTENT-TYPE")) {
        // ct == "application/json; charset=utf-8"
    }
    const exists = headers.has("x-request-id"); // true
    headers.delete("X-REQUEST-ID");
    const total = headers.count();              // 1

    // 按逗号零拷贝迭代多值 Header（如 Accept-Encoding / Cache-Control）
    let iter = HeaderValueIterator.init("gzip, deflate, br;q=1.0, zstd");
    while (token := iter.next()) {
        // 依次产出: "gzip", "deflate", "br;q=1.0", "zstd"
    }
}
```

---

### 3. 缓存校验、静态资源识别与状态码工具 (`ETag` / `MimeType` / `HTTPStatus`)

- **`ETag`**：基于 `Wyhash` 快速生成带引号实体标签；`ETag.parse` 识别弱标签（`W/"..."`）与强标签；`ETag.ifNoneMatch` 支持 `*` 通配符、逗号分隔多值列表以及 RFC 9110 弱比较语义。
- **`MimeType`**：内置 201 项完整 `MIME_DATABASE`（覆盖文本/Web/图片/音视频/字体/归档/文档/3D 模型/消息），支持 `lookupExtension`/`lookupPath`/`lookupMime`/`extensionForMime` 双向映射、`isCompressible`/`isText`/`isUtf8Charset`/`categoryOf` 属性判定，以及基于魔数的 `sniffMagic` 二进制嗅探（PNG/JPEG/GIF/WebP/BMP/TIFF/ICO/WASM/PDF/GZIP/ZIP/OGG/MP3/HTML/SVG）。
- **`HTTPStatus`**：内置 64 项 `HTTP_STATUS_TABLE` 标准/WebDAV/非标准状态码映射，提供 `reasonPhrase` / `reasonPhraseOpt`、状态区间判定（`isInformational` / `isSuccess` / `isRedirection` / `isClientError` / `isServerError` / `isError`），以及 RFC 9110 语义判定 `allowsBody` 与 `isCacheableByDefault`。

```typescript
import { ETag, MimeType, MimeCategory, HTTPStatus } from "cap:http";

pub fn demoETagMimeAndStatus() {
    let tag_buf: [32]u8 = undefined;
    const etag = ETag.generate("{\"hello\":\"cp\"}", &tag_buf);

    // 校验客户端 If-None-Match 请求头
    const not_modified = ETag.ifNoneMatch(etag, etag);                    // true
    const weak_match = ETag.ifNoneMatch("\"v42\"", "W/\"v42\", \"v43\""); // true

    // 201 项 MIME 数据库与二进制魔数嗅探
    const m1 = MimeType.fromExtension(".json");        // "application/json; charset=utf-8"
    const m2 = MimeType.fromPath("/static/app.wasm");  // "application/wasm"
    const ext = MimeType.extensionForMime("image/webp; charset=binary"); // "webp"
    const comp = MimeType.isCompressible("svg");       // true
    const cat = MimeType.categoryOf("woff2");          // MimeCategory.Font
    const magic = MimeType.sniffMagic("\x00asm\x01\x00\x00\x00"); // wasm

    // HTTPStatus 状态码语义
    const phrase = HTTPStatus.reasonPhrase(418);       // "I'm a Teapot"
    const no_body = HTTPStatus.allowsBody(204);        // false
    const cacheable = HTTPStatus.isCacheableByDefault(301); // true
}
```

### 3.1 `HeaderBuilder` 与标准 HTTP 头部解析器家族

- **`HeaderBuilder`**：零拷贝连续线协议缓冲写入器（栈缓冲 `HeaderBuilder.init(&buf)` 或堆扩容 `HeaderBuilder.initAlloc(alloc, 1024)`），内置最多 `MAX_BUILDER_HEADERS`（64）个 `HeaderSpan` 索引，支持 `writeRequestLine`、`writeStatusLine`、`append`、`appendInt`、`appendHeaders`、`get`、`at` 与 `finish()`。
- **标准头部解析器 (`header_parsers.cp`)**：
  - `CacheControl`：RFC 9111 解析与格式化（`max-age`、`s-maxage`、`no-cache`、`no-store`、`must-revalidate`、`stale-while-revalidate`、`immutable` 等）；
  - `ContentDisposition`：RFC 6266 + RFC 5987 `filename*` 扩展名提取与 `effectiveFilename()`；
  - `AcceptParser`：RFC 9110 §12.5.1 `q` 权重与特异度降序稳定排序及 `negotiate(accept_header, supported)` 内容协商；
  - `RangeHeader` & `ContentRange`：RFC 9110 §14.1.2 / §14.4 字节范围解析、`resolve(total_len)` 边界归一化与后缀范围支持。

```typescript
import {
    HeaderBuilder, CacheControl, ContentDisposition, DispositionType,
    AcceptParser, RangeHeader, ContentRange
} from "cap:http";

pub fn demoHeaderBuilderAndParsers(): !void {
    let wire_buf: [512]u8 = undefined;
    let hb = HeaderBuilder.init(&wire_buf);
    * = hb.writeStatusLine(206, "Partial Content");
    * = hb.append("Content-Type", "application/octet-stream");
    * = hb.appendInt("Content-Length", 512);
    const wire = try hb.finish();
    const cl = hb.get("content-length").?; // "512"

    // Cache-Control 解析与回写
    const cc = CacheControl.parse("public, max-age=3600, stale-while-revalidate=60");
    let cc_buf: [128]u8 = undefined;
    const cc_str = try cc.format(&cc_buf);

    // Content-Disposition + Accept + Range
    const cd = ContentDisposition.parse("attachment; filename=\"fallback.txt\"; filename*=UTF-8''report.pdf");
    const fname = cd.effectiveFilename().?; // "report.pdf"

    const supported: [2][]const u8 = [ "text/html", "application/json" ];
    const best = AcceptParser.negotiate("text/html;q=0.8, application/json;q=0.95", &supported); // "application/json"

    const rh = RangeHeader.parse("bytes=0-499, -200");
    const r0 = rh.ranges[0].resolve(1000).?; // start=0, end=499, length=500
    const cr = ContentRange.parse("bytes 0-499/1000"); // total_length=1000
}
```

---

### 4. Cookie 解析与序列化 (`CookieMap` / `CookieEntry` / `CookieSameSite`)

遵循 RFC 6265，内置 64 槽零堆分配存储，支持解析请求 `Cookie:` 头（自动剔除空白与包裹双引号）以及将 `CookieEntry` 格式化为安全完备的 `Set-Cookie:` 响应头。

```typescript
import { CookieMap, CookieEntry, CookieSameSite } from "cap:http";

pub fn demoCookies() {
    // 1. 解析客户端 Cookie 请求头
    let cookies = CookieMap.parse("session_id=\"tok_9f8a\"; theme=dark; lang=zh-CN");
    const sid = cookies.get("session_id"); // "tok_9f8a"
    const has_theme = cookies.has("theme"); // true
    cookies.set("theme", "light");          // 原地更新

    // 2. 序列化 Set-Cookie 响应头到栈缓冲区（零堆分配）
    const entry = CookieEntry{
        name: "session_id",
        value: "tok_new",
        path: "/",
        domain: "api.example.com",
        http_only: true,
        secure: true,
        same_site: CookieSameSite.Strict,
    };
    let buf: [256]u8 = undefined;
    const set_cookie = CookieMap.formatSetCookie(entry, &buf);
    // "session_id=tok_new; Path=/; Domain=api.example.com; HttpOnly; Secure; SameSite=Strict"
}
```

---

### 5. 表单数据解析 (`FormData` & `FormField`)

支持解析 `application/x-www-form-urlencoded` 表单体，以及从 `Content-Type: multipart/form-data; boundary=...` 中零拷贝提取 `boundary` 分隔符：

```typescript
import { FormData } from "cap:http";

pub fn demoFormData() {
    // 1. 解析 URL-Encoded 表单
    let form = FormData.parseUrlEncoded("user=alice&role=admin&flag");
    const user = form.get("user");   // "alice"
    const has_flag = form.has("flag"); // true (value == "")
    form.append("team", "core");

    // 2. 提取 Multipart 边界符
    const ct = "multipart/form-data; boundary=\"----CPBoundary7MA4YWxk\"; charset=utf-8";
    const boundary = FormData.extractBoundary(ct); // "----CPBoundary7MA4YWxk"
}
```

---

### 6. 高层 Web API：`Request` / `Response` / `HttpServer` / `serve` / `fetch` / `json`

`cap:http` 提供与 Bun 高度一致的极简服务端与客户端入口，同时在底层根据 `@is_nest` 自动在 **BEAM Gateway 模式** 与 **Native 原生模式** 之间透明切换：

```typescript
import { Request, Response, HttpServer, serve, fetch, json } from "cap:http";

// 1. 启动高性能 HTTP 服务（对标 Bun.serve）
pub fn startServer(): HttpServer {
    const server = serve({
        port: 3000,
        fetch: (req: Request): Response => {
            if (req.url == "/health") {
                return json({ status: "ok", runtime: "cp" });
            }
            const res = new Response("Hello from CP!");
            res.status = 200;
            res.contentType = "text/plain; charset=utf-8";
            return res;
        },
    });
    return server;
}

// 2. 异步发起 HTTP 请求（对标全局 fetch）
pub async fn fetchRemote(): Response {
    const res = await fetch("https://api.example.com/v1/status", null);
    if (res.ok()) {
        const body = res.text();
        // 或 const obj = res.json()?;
    }
    return res;
}
```

---

### 7. 工业级 HTTP/1.1 协议引擎（`HTTPClient` / `HTTPContext` / `ProxyTunnel` / `PicoHttpParser`）

适用于需要精细控制连接池、代理隧道、分块传输解码、内核 `sendfile` 或零拷贝报文解析的系统级网络编程：

```typescript
import {
    Method,
    Headers,
    HTTPClient,
    HTTPContext,
    ProxyTunnel,
    PicoHttpParser,
    PicoHeader,
    URLPath,
    SignalStore,
    Decompressor,
    Encoding
} from "cap:http";
const std = @import("std");

pub fn demoIndustrialClient(): !void {
    // 1. HTTPClient URL 解析与 RFC 7230 报文构建
    let headers = Headers.init(std.heap.page_allocator);
    defer headers.deinit();
    headers.append("X-Trace-Id", "tr-9981");

    let client = HTTPClient.init(Method.POST, "https://api.example.com:8443/v1/items");
    client.headers = headers;
    client.body = "{\"name\":\"widget\"}";

    let req_buf: [1024]u8 = undefined;
    const wire_req = client.formatRequest(&req_buf)?;

    // 2. PicoHttpParser 零拷贝解析 HTTP/1.1 报文
    let parsed_headers: [16]PicoHeader = undefined;
    if (parsed := PicoHttpParser.parseRequest(wire_req, &parsed_headers)) {
        const trace = PicoHttpParser.findHeader(&parsed_headers, parsed.header_count, "x-trace-id");
        // trace.? == "tr-9981"
    }

    // 3. Chunked Transfer-Encoding 解码
    let out_buf: [128]u8 = undefined;
    const n = HTTPClient.decodeChunkedBody("5\r\nHello\r\n3\r\n CP\r\n0\r\n\r\n", &out_buf)?;
    // out_buf[0..n] == "Hello CP"

    // 4. Keep-Alive 连接池借还管理
    let pool = HTTPContext.init();
    defer pool.deinit();
    * = pool.releaseSocket(42, "api.example.com", 8443, true);
    const reused_fd = pool.acquireSocket("api.example.com", 8443, true); // 42

    // 5. HTTP CONNECT 代理隧道请求格式化与响应校验
    const tunnel = ProxyTunnel.init("127.0.0.1", 8080, "api.example.com", 443);
    let conn_buf: [256]u8 = undefined;
    const connect_wire = tunnel.formatConnectRequest(&conn_buf)?;
    const tunnel_ok = ProxyTunnel.verifyConnectResponse("HTTP/1.1 200 Connection Established\r\n\r\n");

    // 6. 原子中止信号
    let store = SignalStore{};
    const signals = store.toSignals();
    signals.abort();
    const aborted = signals.isAborted(); // true
}
```

---

### 8. HTTP/2 会话、二进制帧流控与完整 HPACK 编解码（`H2ClientSession` / `H2FlowControl` / `HpackSessionCodec`）

遵循 RFC 7540（HTTP/2 帧协议与流量控制）与 RFC 7541（完整 HPACK：61 项静态表 + 257 项 Huffman 编解码 + 动态表与大小协商）：

- **RFC 7540 完整帧与状态机**：支持 `FrameHeader`（9B 帧头）、`SettingsPayloadUnit`、`RstStreamFrame`、`WindowUpdateFrame`、`PingFrame`、`GoawayFrame`、`H2FlowControl`（连接级与流级双向发送/接收窗口追踪、半窗口自动触发 `WINDOW_UPDATE` 补充）以及 `H2ClientSession.processFrame`（帧校验与控制帧自动回包 `SETTINGS ACK` / `PING ACK` / `GOAWAY`）和 `buildHeaderFrames`（超限自动分片为 `HEADERS` + `CONTINUATION`）。
- **RFC 7541 完整 HPACK**：
  - `HpackCodec`：无状态前缀整数编解码、`HPACK_STATIC_TABLE`（全部 61 项）、`HPACK_HUFFMAN_TABLE`（257 符号，含 EOS）+ `HPACK_HUFF_BUCKETS` 快速前缀查表解码（自动校验 EOS 填充位）；
  - `HpackSessionCodec` + `HpackDynamicTable`：带状态的会话级编码器/解码器，支持 `HpackIndexMode.Incremental` / `WithoutIndexing` / `NeverIndexed`、Huffman 可选压缩、动态表 FIFO 插入与大小超限自动驱逐，以及动态表容量更新指令（`encodeTableSizeUpdate`）。

```typescript
import {
    CLIENT_PREFACE,
    FrameType,
    FrameHeader,
    SettingsType,
    SettingsPayloadUnit,
    ErrorCode,
    H2ClientSession,
    HpackCodec,
    HpackSessionCodec,
    HpackIndexMode
} from "cap:http";
const std = @import("std");

pub fn demoHttp2AndHpack() {
    // 1. 9 字节 HTTP/2 帧头与控制帧编解码
    const fh = FrameHeader{
        length: 512,
        type: FrameType.Headers,
        flags: 0x05, // END_STREAM | END_HEADERS
        stream_id: 1,
    };
    let frame_wire: [9]u8 = undefined;
    fh.encode(&frame_wire);
    const decoded_fh = FrameHeader.decode(&frame_wire); // length=512, stream_id=1

    // 2. 客户端会话、流量控制与帧处理
    let session = H2ClientSession{};
    let s1 = session.newStream(); // id == 1, state == Open
    let reply: [64]u8 = undefined;
    const wu_hdr = FrameHeader{ length: 4, type: FrameType.WindowUpdate, flags: 0, stream_id: 1 };
    let wu_buf: [4]u8 = undefined;
    std.mem.writeInt(u32, &wu_buf, 32768, .big);
    const act = session.processFrame(wu_hdr, &wu_buf, &reply, &s1); // WindowUpdated

    // 3. 完整 HPACK 会话编码/解码（静态表 + Huffman + 动态表）
    let hpack = HpackSessionCodec.init(std.heap.page_allocator, 4096);
    defer hpack.deinit();

    let wire_blk: [256]u8 = undefined;
    let pos: usize = 0;
    pos += hpack.encodeHeader(wire_blk[pos..], ":method", "GET", HpackIndexMode.Incremental, true);
    pos += hpack.encodeHeader(wire_blk[pos..], "x-custom-token", "secret-val", HpackIndexMode.Incremental, true);
    pos += hpack.encodeHeader(wire_blk[pos..], "authorization", "Bearer xxx", HpackIndexMode.NeverIndexed, false);

    let name_buf: [128]u8 = undefined;
    let val_buf: [256]u8 = undefined;
    let cursor: usize = 0;
    while (cursor < pos) {
        if (hdr := hpack.decodeHeader(wire_blk[cursor..pos], &name_buf, &val_buf)) {
            cursor += hdr.bytes_read;
        } else {
            break;
        }
    }
}
```

---

### 9. HTTP/3 与 QUIC 协议升级（`AltSvc` & `H3ClientSession`）

遵循 RFC 7838（`Alt-Svc` 响应头协商）与 RFC 9114（HTTP/3 客户端双向流 ID `0, 4, 8, 12…`）：

```typescript
import { AltSvc, H3ClientSession } from "cap:http";

pub fn demoHttp3() {
    // 1. 解析服务端返回的 Alt-Svc 升级提示
    if (alt := AltSvc.parse("h2=\":443\"; ma=3600, h3=\":8443\"; ma=86400")) {
        // alt.protocol == "h3", alt.port == 8443, alt.max_age == 86400
    }
    const cleared = AltSvc.parse("clear"); // null（撤销缓存的替代服务）

    // 2. 创建 HTTP/3 客户端会话并分配 QUIC 客户端双向请求流
    let h3 = H3ClientSession{};
    let stream0 = h3.newRequestStream(); // id == 0, stream_type == RequestBidi
    let stream4 = h3.newRequestStream(); // id == 4
    stream0.close();                     // isOpen() == false
}
```

---

### 10. 流式 Chunked 编解码、Keep-Alive 连接池、自动重定向、代理隧道与内容编码协商

`cap:http` 针对生产级 HTTP/1.1 客户端补全了全套传输控制与协议协商引擎：

1. **流式 Chunked Transfer-Encoding (`ChunkedDecoder` / `ChunkedEncoder`)**：
   - `ChunkedDecoder.init(consume_trailer)` 支持逐字节或任意分片喂入 `feed(input, out)`，自动解析十六进制 chunk 长度、跳过 `;ext=val` chunk extensions、提取最多 16 个 Trailer Header（`getTrailer("X-Checksum")`），并严格校验终止块 `0\r\n\r\n`。
   - `ChunkedEncoder` 提供 `encodeChunk`、`encodeFinal`、`encodeFinalWithTrailers` 与 `encodeBody`（自动按分片大小切块并附带结束帧）。
- **工业级 Keep-Alive 连接池 (`HttpConnectionPool`)**：
   - 按 `(host, port, is_ssl)` 管理连接生命周期（`Idle` / `InUse` / `Dead`），支持 `max_per_host`、`max_total_connections`、`idle_timeout_ms`、`max_requests_per_conn`、LIFO 复用、`acquire`/`releaseConnection`、`evictExpired`、`healthCheck` 与 `PoolStats` 监控指标。
3. **自动重定向策略引擎 (`RedirectPolicy`)**：
   - 完整实现 WHATWG Fetch 重定向语义：301/302 对 `POST` 重写为 `GET` 并丢弃请求体；303 对非 `GET`/`HEAD` 重写为 `GET`；307/308 严格保留原方法与请求体。
   - `RedirectPolicy.resolveLocation` 支持绝对 URL、`//host` 协议相对 URL、`/path` 根相对路径、`./`/`../` 相对路径归一化与 `?query` 合并；内置历史哈希环路检测、最大跳数限制以及跨域自动剥离 `Authorization`/`Cookie`/`Proxy-Authorization`（通过 `applyHeaderPolicy`）。
4. **HTTP 代理隧道与 `NO_PROXY` 匹配 (`ProxyTunnel` / `HttpProxyTunnel` / `NoProxyMatcher`)**：
   - `tunnel.setBasicAuth(user, pass)` 基于 `cap:encoding` `Base64` 自动生成 `Proxy-Authorization: Basic ...` 请求头；`parseConnectResponse` 识别 200 隧道建立与 407 `Proxy-Authenticate` 挑战。
   - `NoProxyMatcher.matches(host, port, no_proxy)` 支持 `*`、精确域名、`.domain` / `domain` 后缀匹配、`:port` 过滤与 IPv4 CIDR（如 `10.0.0.0/8`）。
5. **Content-Encoding 协商与 `cap:pack` 流式解压 (`ContentEncodingNegotiator` / `Decompressor` / `StreamingDecompressor`)**：
   - `ContentEncodingNegotiator.parseAcceptEncoding` 与 `negotiate` 解析 `;q=0.x` 权重并按降序匹配服务器支持编码（支持显式 `q=0` 拒绝与 `*` 通配符）。
   - `Decompressor.decompressStrict` 与 `StreamingDecompressor`（`feed` / `finish`）直接桥接 `cap:pack`（`gunzip`/`inflate`/`inflateRaw` 与 `GunzipStream`/`InflateStream`）完成真实 Gzip/Deflate 解压。
6. **高阶线协议请求构建与响应解析 (`HttpRequestBuilder` / `HttpResponseParser` in `wire_http.cp`)**：
   - `HttpRequestBuilder` 集成 URL 拆解、自动填充 `Host` / `Content-Length` / `Connection` / `Accept-Encoding`、`Expect: 100-continue` 握手与 `applyRedirect` 重定向应用。
   - `HttpResponseParser.parse` 自动跳过 `100 Continue` 中间响应、展开多行 `obs-fold` 折叠头、解码 `Transfer-Encoding: chunked`（保留 Trailers）并按 `Content-Encoding` 自动解压响应体。

```typescript
import {
    Method,
    FetchRedirect,
    ChunkedDecoder,
    ChunkedEncoder,
    HttpConnectionPool,
    RedirectPolicy,
    ProxyTunnel,
    NoProxyMatcher,
    ContentEncodingNegotiator,
    StreamingDecompressor,
    Encoding
} from "cap:http";
const std = @import("std");

pub fn demoDeepHttpEngines(): !void {
    // 1. 流式 Chunked 解码
    let dec = ChunkedDecoder.init(true);
    let out: [128]u8 = undefined;
    const res = try dec.feed("5;ext=1\r\nHello\r\n0\r\nX-Tag: ok\r\n\r\n", &out);
    // out[0..res.bytes_written] == "Hello", dec.getTrailer("X-Tag").? == "ok"

    // 2. Keep-Alive 连接池
    let pool = HttpConnectionPool.init(4, 32, 60000, 1000);
    * = pool.addConnection(10, "api.cp.dev", 443, true, 1000);
    * = pool.releaseConnection(10, "api.cp.dev", 443, true, 1200, true);
    const fd = pool.acquire("api.cp.dev", 443, true, 2000); // 10

    // 3. 自动重定向评估与 NO_PROXY 判定
    let policy = RedirectPolicy.init(FetchRedirect.Follow, 10);
    let url_buf: [256]u8 = undefined;
    const decision = policy.evaluate(302, Method.POST, "https://api.cp.dev/a", "https://cdn.cp.dev/b", &url_buf);
    // decision.next_method == Method.GET, decision.strip_auth_headers == true

    const bypass = NoProxyMatcher.matches("10.1.2.3", 443, "localhost,.internal,10.0.0.0/8"); // true

    // 4. Content-Encoding 协商
    const supported = [Encoding.Gzip, Encoding.Deflate, Encoding.Identity];
    const chosen = ContentEncodingNegotiator.negotiate("deflate;q=0.5, gzip;q=0.9", supported[0..3]); // Encoding.Gzip
}
```

---

### 11. 现代 Web & AI Streaming 协议原语：SSE、`Multipart` 与 `HttpCookieJar`

针对 AI Agent 流式推理（OpenAI / Anthropic SSE）、文件上传（RFC 7578）与有状态客户端会话（RFC 6265），`cap:http` 提供三大生产级原语：

1. **WHATWG Server-Sent Events (`SseEvent` / `SseParser` / `SseEncoder` in `sse.cp`)**：
   - `SseParser.init(alloc)` / `initWithMaxSize(alloc, max_bytes)`：增量流式状态机，`feed(chunk)` 支持跨 chunk 切分、`\r\n` / `\n` / `\r` 行尾、多行 `data:` 自动以 `\n` 拼接、`: comment` 注释跳过、流首 UTF-8 BOM 剥离、`lastEventId()` 追踪与单事件最大字节数防 DoS 限制；`SseEvent.isDone()` 直接识别 `[DONE]` 结束哨兵。
   - `SseEncoder.formatEvent(alloc, ev)` 与 `SseEncoder.formatComment(alloc, text)`：自动按行拆解多行 `data` 并校验 `event_type`/`id` 不含非法换行注入。
2. **RFC 7578 二进制安全 Multipart FormData (`MultipartPart` / `MultipartBoundary` / `MultipartBuilder` / `MultipartParser` in `multipart.cp`)**：
   - `MultipartBoundary.extractFromContentType(ct)`：支持带引号/不带引号 `boundary=...` 提取与非 `multipart/*` 校验。
   - `MultipartBuilder`：`addField(name, value)`、`addFile(name, filename, content_type, data)`、`contentTypeHeader()` 与 `build()`。
   - `MultipartParser.parse(alloc, boundary, body)`：跳过 preamble 与 epilogue，二进制安全边界扫描（不受载荷内 `\0`、`\r\n` 或伪边界前缀干扰），提取 `name`、`filename`、`content_type` 与零拷贝 `body` 切片。
3. **RFC 6265 Cookie 编解码与状态容器 (`SameSite` / `HttpCookie` / `CookieCodec` / `HttpCookieJar` in `cookie.cp`)**：
   - `CookieCodec.parseSetCookie(header)`、`formatSetCookie(alloc, cookie)`（含 `Expires` IMF-fixdate / `Max-Age` / `Domain` / `Path` / `Secure` / `HttpOnly` / `SameSite`）与 `parseCookieHeader(alloc, header)`。
   - `HttpCookieJar`：支持存储/覆盖/删除（`Max-Age <= 0` 或过期）、RFC 6265 域名后缀匹配（含基础公共后缀边界防越界校验）、路径前缀匹配与长度降序排列、`Secure` 协议校验以及 `cookieHeaderFor(alloc, host, path, is_https, now_epoch_s)`。

```typescript
import {
    SseEvent, SseParser, SseEncoder,
    MultipartBoundary, MultipartBuilder, MultipartParser,
    HttpCookie, CookieCodec, HttpCookieJar
} from "cap:http";
const std = @import("std");

pub fn demoWebPrimitives(): !void {
    const alloc = std.heap.page_allocator;

    // 1. SSE 增量流式解析（适配 AI LLM 流式响应）
    let sse = SseParser.init(alloc);
    defer sse.deinit();
    const events = try sse.feed("id: 1\nevent: delta\ndata: Hello\ndata: CP\n\ndata: [DONE]\n\n");
    defer sse.freeEvents(events); // events[0].data == "Hello\nCP", events[1].isDone() == true

    // 2. Multipart 文件上传打包与解析
    let mb = MultipartBuilder.init(alloc);
    defer mb.deinit();
    try mb.addField("model", "cp-agent");
    try mb.addFile("file", "weights.bin", "application/octet-stream", "\x00\x01\x02");
    const mp_body = try mb.build();
    defer alloc.free(mp_body);
    const bnd = MultipartBoundary.extractFromContentType(mb.contentTypeHeader()).?;
    const parts = try MultipartParser.parse(alloc, bnd, mp_body);
    defer MultipartParser.freeParts(alloc, parts);

    // 3. RFC 6265 CookieJar
    let jar = HttpCookieJar.init(alloc);
    defer jar.deinit();
    try jar.setFromHeader("api.example.com", "sid=xyz; Domain=example.com; Path=/api; Secure; Max-Age=3600", 1700000000);
    const cookie_hdr = try jar.cookieHeaderFor(alloc, "api.example.com", "/api/chat", true, 1700000010);
    defer if (cookie_hdr.len > 0) alloc.free(cookie_hdr); // "sid=xyz"
}
```

---

## 三、 与 Bun `fetch` / `Bun.serve` 对照表

| 能力领域 | Bun (TypeScript / JS) | CP (`cap:http`) | 核心差异与优势 |
|---|---|---|---|
| **HTTP 服务端入口** | `Bun.serve({ port, fetch(req) { ... } })` | `serve({ port, fetch: (req: Request): Response => ... })` | 返回强类型 `HttpServer`，无 GC 停顿，支持 BEAM 与 Native 双运行模式 |
| **JSON 快捷响应** | `Response.json(obj)` | `json(obj)` | 编译期静态序列化路径，自动设置 `Content-Type: application/json` |
| **异步 HTTP 客户端** | `await fetch(url, init)` | `await fetch(url, options)` | 基于 CPS 状态机挂起与唤醒，无 Promise 堆对象开销 |
| **响应体读取** | `await res.text()` / `await res.json()` | `res.text()` / `res.json()?` | 强类型错误传播（`?`），彻底废除 `try/catch` 异常捕获 |
| **头部集合** | `new Headers()`（GC 托管） | `Headers.init(allocator)` + `defer headers.deinit()` | 前 32 项栈内联零堆分配，超过 32 项自动扩容，确定性析构 |
| **Cookie 管理** | `new Bun.CookieMap(header)` | `CookieMap.parse(header)` & `CookieMap.formatSetCookie(entry, &buf)` | 64 槽零堆分配存储，格式化直接写入调用方栈缓冲 |
| **表单解析** | `await req.formData()` / `URLSearchParams` | `FormData.parseUrlEncoded(body)` & `FormData.extractBoundary(ct)` | 零拷贝切片引用原报文内存，无字符串拷贝开销 |
| **ETag 与 MIME** | 内部 C++/Zig 工具，未全部直出 | `ETag.generate` / `ETag.ifNoneMatch` / `MimeType.fromPath` | 作为标准库一等公民直接开放给业务与中间件 |
| **底层线协议控制** | 隐藏在运行时内部 | `PicoHttpParser`、`HTTPClient`、`HTTPContext`、`ProxyTunnel`、`SendFile` | 支持自建反向代理、网关、零拷贝文件服务与自定义连接池 |
| **HTTP/2 & HTTP/3** | `fetch` 内部隐式协商 | 直接导出 `FrameHeader`、`H2ClientSession`、`HpackCodec`、`AltSvc`、`H3ClientSession` | 既支持高层自动协商，也支持构建自定义 H2/H3 代理与协议分析工具 |

---

## 四、 工程最佳实践与注意事项

1. **确定性释放动态资源**：
   - 使用 `Headers.init(allocator)`、`HTTPContext.init()` 或 `CertificateInfo` 时，务必配对 `defer obj.deinit();`，防止堆切片或连接池句柄泄漏。
2. **只读接收者与零拷贝生命周期**：
   - `PicoHttpParser.parseRequest`、`CookieMap.parse`、`FormData.parseUrlEncoded`、`URLPath.parse` 与 `HpackCodec.decodeHeader` 均返回指向输入缓冲区的零拷贝切片，调用方需确保底层输入缓冲区在解析结果使用期间保持存活。
3. **优先使用栈缓冲进行格式化**：
   - `HTTPClient.formatRequest(&buf)`、`ProxyTunnel.formatConnectRequest(&buf)`、`CookieMap.formatSetCookie(entry, &buf)`、`FrameHeader.encode(&buf)`、`HpackCodec.encodeHeader(buf, ...)` 均无需分配器，天然适配百万并发 Agent 场景。
4. **敏感头部禁止索引（HPACK 安全防线）**：
   - 在通过 `HpackCodec.encodeHeader` 编码 `authorization`、`cookie` 等凭据头部时，应将 `never_index` 置为 `true`（生成 `0x10` Never-Indexed 帧），防止代理与中间节点缓存敏感字段。
5. **上游 Bun HTTP 路由优先级与服务端边界契约**：
   - 路由匹配遵循自上而下的特异性顺序（参考 `/data/dev/bun/docs/runtime/http/routing.mdx`）：精确路径（`/users/all`）> 参数路径（`/users/:id`）> 前缀通配（`/users/*`）> 全局兜底（`/*` / `fetch`）。
   - 端口与主机默认约定（参考 `/data/dev/bun/docs/runtime/http/server.mdx`）：未显式指定 `hostname` 时默认监听 `"0.0.0.0"`，未匹配任何路由时回落至 `fetch` 处理器或返回 `404`。
   - `CookieMap` 遵循 RFC 6265：`CookieMap.parse(header)` 零分配解析请求 `Cookie` 头部（64 槽内联），`CookieMap.formatSetCookie(entry, &buf)` 支持 `Path`、`Domain`、`Max-Age`、`HttpOnly`、`Secure` 与 `SameSite` 属性序列化。
   - **Chunked 传输与连接池复用**：HTTP/1.1 Keep-Alive 连接复用前须确保上一响应体（含分块终止符 `0\r\n\r\n`）已完整消费；HTTP/2 客户端流严格使用奇数流 ID（1, 3, 5…），收到 `GOAWAY` 后停止在现有会话上新建流。

---

## 五、 归并的协议编解码子模块（H2 / H3 QPACK / ByteRange）

- **HTTP/2 请求编码与窗口分帧（`h2/encode.cp`）**：归并自 `lib/h2_encode/`（Bun `src/http/h2_client/encode.cp`），提供 `encodeClientPreface`、`encodeFrameHeader`、`encodeHeaderBlockFrames`、`encodeDataWindowed`、`classifyRequestHeader`、`isSensitiveHeader`、`DataWindowState` 等零分配编码原语。
- **HTTP/2 流状态机与控制帧（`h2/stream.cp`）**：归并自 `lib/h2stream/`（Bun `src/http/h2_client/Stream.cp`），在主包以 `H2StreamMachine`（避开 `h2/client.cp` 的 `H2Stream`）、`StreamState`、`H2ErrorCode`、`encodeRstStreamFrame`、`encodeWindowUpdateFrame` 导出。
- **HTTP/2 帧协议校验与连接级分发器（`h2/dispatch.cp`，吸收自 `/data/dev/bun/src/http/h2_client/dispatch.cp`）**：提供零堆分配的 `H2ParsedFrameHeader`、`stripH2Padding`、`H2DispatchError`、`H2DispatchEventKind`、`H2DispatchEvent`、`H2FrameDispatcher`（`dispatchOne` / `parseNext`，覆盖 RFC 7540 首帧 SETTINGS 强制校验、CONTINUATION 序列锁、WINDOW_UPDATE 2^31-1 溢出防护、PING/GOAWAY/RST_STREAM/HEADERS/DATA PADDED+PRIORITY 零拷贝解析）。

```typescript
import { H2FrameDispatcher } from "cap:http";

let disp = H2FrameDispatcher.init();
let consumed: usize = 0;
if (ev := disp.parseNext(wire_buf, &consumed)?) {
    // 处理 ev.kind / ev.stream_id / ev.payload_slice，并推进 wire_buf[consumed..]
}
```
- **HTTP/3 QPACK 静态表与字段段编解码（`h3/qpack.cp`）**：归并自 `lib/qpack/`（Bun `src/uws_sys/quic/Header.cp`），导出 `QPACK_IDX_*`、`QpackHeader`、`classifyHeader`、`encodePrefixedInt`、`decodePrefixedInt`、`buildHttp3RequestHeaders`、`encodeFieldSection`、`decodeFieldSection`。
- **服务端 HTTP Byte Range 区间求值与 206/416 响应格式化（`byterange.cp`）**：归并自 `lib/byterange/`（Bun `src/runtime/server/RangeRequest.cp`），导出 `ByteRange`、`RangeStatus`、`parseRange`、`parseMultiRange`、`formatUnsatisfiableContentRange`，以及 `ByteRangeContentRange` / `formatByteContentRange` / `parseByteContentRange`（避开 `contentrange.cp` 原导出）。
- **SSE 零堆分配参考实现（`sse_parser_.cp`）**：原 `lib/sse_parser/sse.cp`（Bun `src/http/HTTPRequestBody.cp`）与 `lib/http/sse.cp` 存在 `SseEvent`/`SseParser` 功能重叠，按参考归档规范保留为 `lib/http/sse_parser_.cp`（不从 `cap:http` 重复导出）。
- **线程安全 HTTP 流缓冲区与背压控制（`stream_buffer.cp`，吸收自 `/data/dev/bun/src/http/ThreadSafeStreamBuffer.cp`）**：导出 `DEFAULT_STREAM_SHRINK_THRESHOLD`、`StreamBufferError`、`StreamDrainCallback`、`StreamBuffer`、`ThreadSafeStreamBuffer`。
  - `StreamBuffer` 提供游标式零拷贝读切片（`slice()`）、`write()`/`writeByte()`/`wrote()`/`consume()`、自动头部空洞压缩（`compact()`）、超阈值释放收缩（`maybeShrink()`/`reset()`）及可选容量上限防护（`initBounded` 防资源耗尽返回 `err.BufferOverflow`）；
  - `ThreadSafeStreamBuffer` 使用 `std.Thread.Mutex` 保护内部 `StreamBuffer`，支持与 Bun 对齐的 `acquire()`/`release()` 零拷贝锁区间访问，也提供细粒度线程安全方法（`write`/`read`/`size`/`isEmpty`/`isBackpressured`）与 **解锁后触发排空回调**（`reportDrain()`，彻底避免回调重入死锁）。

```typescript
import { ThreadSafeStreamBuffer, StreamDrainCallback } from "cap:http";

fn onDrained(ctx: usize) {
    * = ctx;
}

pub fn demoStreamBuffer(allocator: std.mem.Allocator): ! {
    let ts_buf = ThreadSafeStreamBuffer.initBounded(allocator, 65536, 16384);
    defer ts_buf.deinit();

    ts_buf.setDrainCallback(StreamDrainCallback.init(onDrained, 1));
    ts_buf.write("HTTP/1.1 200 OK\r\n\r\nhello")?;

    let out: [64]u8 = [0; 64];
    const n = ts_buf.read(out[0..]);
    * = n;
    * = ts_buf.reportDrain(); // 缓冲区为空时在解锁后安全触发回调
}
```
- **HTTP/3 客户端会话池与连接复用调度（`h3/session_pool.cp`，吸收自 `/data/dev/bun/src/http/h3_client/ClientContext.cp`）**：
  - 导出：`H3_POOL_MAX_SESSIONS` (`32`)、`H3_SESSION_MAX_STREAMS` (`32`)、`H3PoolError`、`H3ConnectOutcomeKind`（`ReusedExisting` / `OpenedNew`）、`H3ConnectOutcome`、`H3PendingRequest`、`H3PooledSession`（`matches` / `hasHeadroom` / `enqueue` / `findRequest` / `detachByHttpId` / `markBodyEnded` / `activeCount`）、`H3SessionPool`（`init` / `count` / `connect` / `findSessionIndex` / `unregister` / `abortByHttpId` / `streamBodyByHttpId` / `failSession`）。
  - 严格按 `(hostname[大小写无关], port, reject_unauthorized)` 三元组执行连接合并复用，确保 TLS 校验策略隔离与并发流余量溢出自动开新会话，并支持 `O(1)` swap-remove 注销与按 `http_id` 路由中止/体流结束事件。

```typescript
import { H3SessionPool } from "cap:http";

let pool = H3SessionPool.init();
const c1 = pool.connect("api.example.com", 443, true, 101, 4)?;  // OpenedNew
const c2 = pool.connect("API.EXAMPLE.COM", 443, true, 102, 4)?;  // ReusedExisting (同 session_id)
* = pool.streamBodyByHttpId(101, true);
* = pool.abortByHttpId(102);
* = c1;
* = c2;
```

### 上游 Bun HTTP 运维指标、TLS 与错误处理最佳实践（吸收自 `/data/dev/bun/docs/runtime/http/{metrics,error-handling,tls}.mdx`）

1. **活跃连接与订阅指标监控**：
   - 在高并发 HTTP / WebSocket 网关中，应持续观测活跃请求数（对应 Bun `server.pendingRequests`）、活跃 WebSocket 连接数（`server.pendingWebSockets`）以及按主题统计的订阅者数量（`server.subscriberCount(topic)`），结合 `ThreadSafeStreamBuffer.isBackpressured()` 实施流控与优雅降级。
2. **开发期与生产期错误响应隔离**：
   - 服务端应提供统一的错误处理回调（对应 Bun `error(err)` handler），在开发模式输出详细诊断信息，在生产环境返回脱敏的 5xx 响应并记录结构化错误日志；自定义错误回调返回响应时优先覆盖框架默认 500 错误页。
3. **TLS 证书载荷、CA 覆盖与 SNI 隔离（吸收自 `/data/dev/bun/docs/runtime/http/tls.mdx`）**：
   - TLS 配置中的 `key` 与 `cert` 均期望传入**证书/私钥文件内容本身**（而非文件路径字符串），且必须同时提供；自定义 `ca` 会覆盖内置的 Mozilla 根证书列表，客户端连接池复用时必须严格隔离 `reject_unauthorized` 策略（`H3PooledSession.matches` 强制比对），并通过 SNI（`serverName`）支持多域名证书托管。
4. **异步 HTTP 并发限流与任务状态机编排（`async_http.cp`，吸收自 `/data/dev/bun/src/http/AsyncHTTP.cp` 与 `/data/dev/bun/docs/guides/http/{fetch,proxy,fetch-unix}.mdx`）**：
   - 导出：`ASYNC_HTTP_DEFAULT_MAX_SIMULTANEOUS` (`256`)、`ASYNC_HTTP_MAX_CONFIG_LIMIT` (`65536`)、`AsyncHttpError`、`AsyncHttpState`、`AsyncHttpVerboseLevel`、`AsyncHttpOptions`、`AsyncHttpConcurrencyLimiter`、`AsyncHttpTask`；
   - 默认最大同时请求数对齐上游 `BUN_CONFIG_MAX_HTTP_REQUESTS = 256`（通过 `parseMaxRequestsEnv()` 校验 `1..65536`）；`AsyncHttpOptions.validate()` 拦截代理与 Unix Domain Socket 传输冲突；`AsyncHttpTask` 强制 `pending -> scheduled -> sending -> success|fail` 合法迁移及有界 `resetForRetry()`。
