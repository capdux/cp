# CP 标准库：全栈工业级 HTTP/1.1 · HTTP/2 · HTTP/3 子系统使用手册 (`cap:http`)

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
| `lib/http/headers.cp` | 大小写不敏感 `Headers` 集合（32 槽栈内联 + 堆动态扩容） | `src/http/Headers.cp` |
| `lib/http/header_iterator.cp` | 逗号分隔 HTTP Header 值零拷贝迭代器 | `src/http/HeaderValueIterator.cp` |
| `lib/http/etag.cp` | Wyhash ETag 生成与强/弱 `If-None-Match` 校验 | `src/http_types/ETag.cp` |
| `lib/http/mime.cp` | 扩展名与文件路径 MIME Type 极速查询 | `src/http_types/MimeType.cp` |
| `lib/http/cookie.cp` | RFC 6265 `Cookie` 请求头解析、`CookieMap` 与 `Set-Cookie` 序列化 | `src/runtime/webcore/CookieMap.cp` |
| `lib/http/form_data.cp` | `x-www-form-urlencoded` 解析与 `multipart/form-data` 边界提取 | `src/runtime/webcore/FormData.cp` |
| `lib/http/request.cp` / `response.cp` / `server.cp` | 高层 `Request` / `Response` / `HttpServer`、`serve` / `fetch` / `json` | `src/http/http.cp` |
| `lib/http/client.cp` | 工业级 HTTP/1.1 `HTTPClient`（URL 解析、请求格式化、Chunked 解码） | `src/http/http.cp` |
| `lib/http/chunked.cp` | RFC 9112 §7.1 流式 `ChunkedDecoder`（hex size/extensions/trailers/`0\r\n\r\n`）与 `ChunkedEncoder` | `src/http/http.cp` |
| `lib/http/redirect.cp` | `RedirectPolicy` 自动重定向策略引擎（301/302/303/307/308 方法重写、URL 合并、循环检测、跨域敏感头剥离） | `src/http/http.cp` |
| `lib/http/context.cp` | `HTTPContext` 与 `HttpConnectionPool`（按 host:port 复用、空闲超时驱逐、最大连接数与健康检查） | `src/http/HTTPContext.cp` |
| `lib/http/proxy.cp` / `sendfile.cp` / `decompressor.cp` | `ProxyTunnel` / `HttpProxyTunnel`（Basic 认证与 `NoProxyMatcher`）、`SendFile`、与 `cap:pack` 协同的 `Decompressor` / `StreamingDecompressor` | `ProxyTunnel.cp` / `SendFile.cp` / `Decompressor.cp` |
| `lib/http/url_path.cp` / `encoding.cp` / `fetch_modes.cp` / `signals.cp` / `internal_state.cp` / `request_body.cp` / `cert.cp` | 路径解析、`ContentEncodingNegotiator` (`Accept-Encoding` q-value 协商)、Fetch 模式枚举、原子中止信号、状态机与证书信息 | `src/http/` & `src/http_types/` |
| `lib/http/h2/frame.cp` / `client.cp` / `hpack.cp` | RFC 7540 二进制帧、多路复用流状态机、RFC 7541 HPACK 头部压缩 | `H2FrameParser.cp` / `h2_client/ClientSession.cp` / `lshpack.cp` |
| `lib/http/h3/alt_svc.cp` / `client.cp` | RFC 7838 `Alt-Svc` 升级头解析、HTTP/3 QUIC 会话与双向流 | `h3_client/AltSvc.cp` / `h3_client/ClientSession.cp` |

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

### 3. 缓存校验与静态资源识别 (`ETag` & `MimeType`)

- **`ETag`**：基于 `Wyhash` 快速生成带引号实体标签；`ETag.parse` 识别弱标签（`W/"..."`）与强标签；`ETag.ifNoneMatch` 支持 `*` 通配符、逗号分隔多值列表以及 RFC 9110 弱比较语义。
- **`MimeType`**：支持从扩展名（带或不带 `.`）或完整文件路径提取标准 `Content-Type`，未知类型自动回退至 `application/octet-stream`。

```typescript
import { ETag, MimeType } from "cap:http";

pub fn demoETagAndMime() {
    let tag_buf: [32]u8 = undefined;
    const etag = ETag.generate("{\"hello\":\"cp\"}", &tag_buf);

    // 校验客户端 If-None-Match 请求头
    const not_modified = ETag.ifNoneMatch(etag, etag);                    // true
    const weak_match = ETag.ifNoneMatch("\"v42\"", "W/\"v42\", \"v43\""); // true
    const parsed = ETag.parse("W/\"abc123\"");
    // parsed.is_weak == true, parsed.tag == "abc123"

    // 极速 MIME 映射
    const m1 = MimeType.fromExtension(".json");        // "application/json; charset=utf-8"
    const m2 = MimeType.fromPath("/static/app.wasm");  // "application/wasm"
    const m3 = MimeType.fromPath("/files/README");     // "application/octet-stream"
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

### 8. HTTP/2 会话、二进制帧与 HPACK 编解码（`H2ClientSession` / `FrameHeader` / `HpackCodec`）

遵循 RFC 7540（HTTP/2 帧协议）与 RFC 7541（HPACK 头部压缩），全程零堆分配：

```typescript
import {
    CLIENT_PREFACE,
    FrameType,
    FrameHeader,
    SettingsType,
    SettingsPayloadUnit,
    ErrorCode,
    H2ClientSession,
    HpackCodec
} from "cap:http";

pub fn demoHttp2AndHpack() {
    // 1. 9 字节 HTTP/2 帧头大端序编解码
    const fh = FrameHeader{
        length: 512,
        type: FrameType.Headers,
        flags: 0x05, // END_STREAM | END_HEADERS
        stream_id: 1,
    };
    let frame_wire: [9]u8 = undefined;
    fh.encode(&frame_wire);
    const decoded_fh = FrameHeader.decode(&frame_wire); // length=512, stream_id=1

    // 2. 6 字节 SETTINGS 参数编码
    const setting = SettingsPayloadUnit{
        type: SettingsType.MaxConcurrentStreams,
        value: 128,
    };
    let setting_wire: [6]u8 = undefined;
    setting.encode(&setting_wire);

    // 3. 客户端会话与奇数流 ID 分配 + 半关闭/重置状态机
    let session = H2ClientSession{};
    let s1 = session.newStream(); // id == 1, state == Open
    let s2 = session.newStream(); // id == 3
    s1.sentEndStream();           // state == HalfClosedLocal
    s1.recvEndStream();           // state == Closed
    s2.rst(ErrorCode.Cancel);     // rst_done == true, last_error == Cancel
    session.onGoaway();           // canCreateStream() == false

    // 4. HPACK 头部块压缩与解码（自动命中静态表或使用 Literal / Never-Index）
    let hpack_buf: [256]u8 = undefined;
    let pos: usize = 0;
    pos += HpackCodec.encodeHeader(hpack_buf[pos..], ":method", "GET", false);           // 静态表索引 #2 (1B: 0x82)
    pos += HpackCodec.encodeHeader(hpack_buf[pos..], ":scheme", "https", false);         // 静态表索引 #7 (1B: 0x87)
    pos += HpackCodec.encodeHeader(hpack_buf[pos..], "authorization", "Bearer xxx", true); // Never-Index (0x10)

    let cursor: usize = 0;
    while (cursor < pos) {
        if (hdr := HpackCodec.decodeHeader(hpack_buf[cursor..pos])) {
            // hdr.name, hdr.value, hdr.never_index
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
