# CP 标准库：Web、API、RPC、HTTP 协议与浏览器查询套件手册 (`cap:web`)

> 来源：Pure-CP 零 C 依赖 Web/API/RPC/协议与服务端治理标准库收敛（16 个子模块统一聚合） | 创建日期：2026-11-22

## 一、 架构总览

`cap:web` 是 CP 语言官方内置的纯原生、零外部 C 依赖 Web 与 API 生态标准库，统一覆盖 **OpenAPI 3.1 规范校验、MessagePack-RPC、HTTP/1.1/2/3 协议解析与二进制帧编解码、SSE/Multipart/URI Template、RSS/Atom/OPML/Sitemap、CSS3 选择器与 XPath、Webhook 签名幂等与限流熔断治理**。

---

## 二、 模块文件拓扑与核心 API 索引

### 1. API 规范、RPC 与 Webhook（3 个子模块）

| 子模块文件 | 领域分类 | 核心导出符号 |
| :--- | :--- | :--- |
| `lib/web/openapiSpec.cp` / `openapiRoute.cp` | OpenAPI 3.1.0 AST、路由匹配与契约校验 | `OpenApiDoc`, `OpenApiValidator`, `OpenApiEndpoint`, `OpenApiOperation`, `RouteMatchResult`, `ValidationReport`, `HttpMethod` |
| `lib/web/msgpackRpc.cp` | MessagePack-RPC 编解码与流式帧解码 | `encodeRpcMessage`, `decodeRpcMessage`, `encodeRpcValue`, `RpcStreamDecoder`, `RpcRequest`, `RpcResponse`, `RpcNotification`, `RpcValue` |
| `lib/web/webhookSign.cp` / `webhookIdem.cp` | Webhook HMAC 验签 (Standard/GitHub/Stripe) 与幂等存储 | `StandardWebhook`, `GitHubWebhook`, `StripeWebhook`, `IdempotencyStore`, `WebhookError` |

### 2. HTTP/1.1、HTTP/2、HTTP/3、SSE、Multipart 与 URI 模板（6 个子模块）

| 子模块文件 | 领域分类 | 核心导出符号 |
| :--- | :--- | :--- |
| `lib/web/picohttp.cp` | 零分配 HTTP/1.1 请求/响应与 Chunked 解析（对标 `seanmonstar/httparse` MIT/Apache-2.0 加固，含 16-lane SIMD 扫描与 RFC 9112 帧界定歧义防护） | `HTTP_MAX_HEADERS`, `FramingInfo`, `parseRequest`, `parseResponse`, `parseHeaders`, `findHeader`, `parseContentLength`, `validateHttp1Framing`, `parseChunkSizeStrict`, `simdScanTargetBytes`, `simdScanHeaderNameBytes`, `simdScanHeaderValueBytes`, `simdFindCrlf`, `ChunkedDecoder`, `ParsedRequest`, `ParsedResponse` |
| `lib/web/http2.cp` | RFC 9113 HTTP/2 二进制帧、流状态与流控（对标 `hyperium/h2` MIT 加固 + 16-lane SIMD 前言/行切分原语 & 零堆 `validateHttp2FrameStrict`） | `CONNECTION_PREFACE`, `Http2FrameHeader`, `simdH2SseFindLineBreak`, `simdH2SseBytesEqual`, `hasConnectionPreface`, `validateHttp2Preface`, `validateHttp2FrameStrict`, `encodeFrame`, `FrameStreamDecoder`, `FlowWindow`, `Http2Stream`, `encodeSettings`, `decodeSettings`, `encodePing`, `encodeGoaway`, `encodeWindowUpdate`（测试：`cap/test/testWebH2SseRustRefSuite.cp`） |
| `lib/web/http3.cp` | RFC 9114 HTTP/3 帧与 QPACK 静态/动态头编解码 | `encodeH3Frame`, `decodeFrames`, `decodeFramesWithMaxSize`, `freeFrames`, `encodeQpackHeaders`, `decodeQpackHeaders`, `H3_DEFAULT_MAX_FRAME_SIZE`, `H3Frame` |
| `lib/web/sse.cp` | WHATWG Server-Sent Events (SSE) 流式解析与编码（对标 `hyperium/h2`/WHATWG SSE 加固 + 16-lane SIMD 换行/NUL 扫描与字段比对） | `SseParser`, `SseEncoder`, `SseEvent`, `SseError`（测试：`cap/test/testWebH2SseRustRefSuite.cp`） |
| `lib/web/multipart.cp` | RFC 7578 `multipart/form-data` 解析与构造（对标 `rwf2/multer` MIT & RFC 7578 加固 + 16-lane SIMD `simdMpFindDashOrCrlf`、`DEFAULT_MAX_HEADERS_PER_PART` 与注入防护） | `extractBoundary`, `isValidMultipartBoundary`, `parse`, `parseWithLimits`, `MultipartBuilder`, `MultipartDocument`, `MultipartPart`, `MultipartError`（测试：`cap/test/testWebMultipartUriRustRefSuite.cp`） |
| `lib/web/uritemplate.cp` | RFC 6570 Level 1–4 URI Template 展开（对标 RFC 6570 加固 + 16-lane SIMD `simdUriFindBrace` 与 `expandWithLimit`/`URI_TEMPLATE_DEFAULT_MAX_OUTPUT`） | `UriTemplate`, `TemplateVars`, `AssocPair`, `TemplateOp`, `UriTemplateError`（测试：`cap/test/testWebMultipartUriRustRefSuite.cp`） |

### 3. 内容聚合、爬虫协议与浏览器/DOM 查询（5 个子模块）

| 子模块文件 | 领域分类 | 核心导出符号 |
| :--- | :--- | :--- |
| `lib/web/sitemap.cp` | RFC 9309 `robots.txt` 与 XML Sitemap 0.9 | `RobotsRules`, `SitemapCodec`, `SitemapUrl`, `SitemapIndexItem`, `ChangeFreq`, `SitemapKind` |
| `lib/web/feed.cp` | RSS 2.0 / Atom 1.0 / JSON Feed 1.1 | `FeedCodec`, `Feed`, `FeedItem`, `FeedFormat`, `FeedError` |
| `lib/web/opml.cp` | 零分配 OPML 2.0 订阅大纲解析与格式化 | `isOpmlDocument`, `parseOpmlHead`, `OpmlOutlineIter`, `formatOpmlFeedOutline`, `OpmlHead`, `OpmlOutline` |
| `lib/web/selector.cp` | CSS3 选择器编译与 DOM 匹配引擎 | `CssSelector`, `ComplexSelector`, `CompoundSelector`, `AttrSelector`, `PseudoSelector`, `SelectorError` |
| `lib/web/xpath.cp` | 零分配 W3C XPath 词法与步进扫描器 | `XPathIter`, `XPathStepIter`, `validateXPathDelimiters`, `XPathToken`, `XPathStep` |

### 4. API 网关流量治理与容错（4 个子模块）

| 子模块文件 | 领域分类 | 核心导出符号 |
| :--- | :--- | :--- |
| `lib/web/ratelimit.cp` | 确定性限流器 (令牌桶/滑动窗口/GCRA/分键限流) | `TokenBucket`, `SlidingWindowCounter`, `GcraLimiter`, `KeyedRateLimiter`, `RateDecision`, `RateLimitError` |
| `lib/web/breaker.cp` / `breakerPol.cp` | 熔断器、指数退避抖动与舱壁隔离 | `CircuitBreaker`, `CircuitConfig`, `CircuitState`, `CircuitStats`, `BackoffPolicy`, `Bulkhead`, `JitterMode` |
| `lib/web/middleware.cp` | 零堆分配 HTTP 洋葱中间件链、CORS/Bearer 鉴权/413 守卫与指标统计 | `WebContext`, `WebMiddlewareConfig`, `WebMiddlewareOutcome`, `WebMiddlewareStats`, `WebMiddlewarePipeline`, `renderJsonErrorBody` |
| `lib/web/hedged.cp` | Google "The Tail at Scale" 对冲请求（Hedged Requests）与重试预算防风暴控制器 | `HEDGED_MAX_ATTEMPTS`, `RetryBudgetConfig`, `RetryBudgetStats`, `RetryBudget`, `HedgedRequestConfig`, `HedgedAction`, `HedgedAttemptState`, `HedgedWinResult`, `HedgedRequestTracker` |

### 5. 旗舰零分配 REST API 框架（4 个核心子模块）

融合 **ElysiaJS 链式 8 阶段生命周期 / Plugin / exact-mirror / OpenAPI 3.1.0** 与 **Axum 零反射提取器 / Radix Trie / Router 组合**，100% 纯 CP 零动态堆分配：

| 子模块文件 | 领域分类 | 核心导出符号 |
| :--- | :--- | :--- |
| `lib/web/radixRouter.cp` | Memoirist O(1) 静态哈希 + Matchit 压缩 Radix Trie 路由器 | `RadixRouter`, `RadixMatchResult`, `RadixParam`, `RADIX_STATIC_BUCKETS`, `RADIX_MAX_NODES`, `RADIX_METHOD_SLOTS` |
| `lib/web/app.cp` | 旗舰 REST 应用内核（8 阶段生命周期、Plugin、Axum `nest`/`merge`/`fallback`、RFC 6265 Cookie、OpenAPI 3.1.0） | `App`, `Plugin`, `PluginScope`, `RouteGroup`, `Context`, `Request`, `Response`, `AppMethod`, `AppErrorCode`, `CookieOptions`, `CookieSameSite`, `formatSetCookie` |
| `lib/web/schema.cp` | 16-lane SIMD URI 解码、零反射 JSON Schema 校验、`exact-mirror` 字段裁剪与 OpenAPI Schema 导出 | `fastDecodeUriComponent`, `fastDecodeFormComponent`, `fastDecodeUriComponentOpts`, `parseQueryPair`, `findQueryParamDecoded`, `ObjectSchema`, `FieldKind`, `FieldRule`, `ValidationReport`, `ValidationIssue`, `validateJsonObject`, `exactMirrorJson`, `schemaToOpenApiJson` |
| `lib/web/fileType.cp` | 零堆 30 种 Web/媒体/归档/字体/SQLite/文本魔数文件类型检测器 | `FileKind`, `FileTypeInfo`, `detectFileType` |

---

## 三、 使用示例

```typescript
import {
    parseRequest,
    TokenBucket,
    CircuitBreaker,
    CircuitConfig,
    isOpmlDocument,
} from "cap:web";

pub fn demoWeb() {
    // 1. 令牌桶限流
    let bucket = TokenBucket.init(10.0, 5.0, 0);
    const dec = bucket.allow(1000, 1.0);
    * = dec;

    // 2. 熔断器
    let cb = CircuitBreaker.init(CircuitConfig.default());
    * = cb.allow(1000);

    // 3. OPML 检测 & HTTP/1.1 零分配解析
    * = isOpmlDocument("<opml version=\"2.0\"></opml>");
    * = parseRequest;
}
```

### 生产级零堆分配 HTTP 洋葱中间件链示例 (`middleware.cp`)

```cp
import {
    WebContext,
    WebMiddlewareConfig,
    WebMiddlewareOutcome,
    WebMiddlewarePipeline,
    renderJsonErrorBody,
} from "cap:web";

pub fn demoWebMiddleware() {
    let mw = WebMiddlewarePipeline.init(WebMiddlewareConfig{
        max_body_bytes: 65536,
        allowed_origin: "https://app.example.com",
        required_bearer_token: "sk-live-secret",
        auth_skip_paths_csv: "/healthz,/metrics",
    });

    let ctx = WebContext{
        method: "POST",
        path: "/v1/chat",
        origin: "https://app.example.com",
        bearer_token: "sk-live-secret",
        body_len: 512,
    };

    let id_buf: [32]u8 = [0; 32];
    * = mw.ensureRequestId(&ctx, id_buf[0..]); // 自动生成 "req-1"

    const outcome = mw.runBefore(&ctx);
    if (outcome == WebMiddlewareOutcome.Continue) {
        // 执行业务路由并在洋葱后置阶段记录状态码与延迟
        mw.recordAfter(&ctx, 200, 1250);
    } else if (ctx.aborted) {
        let err_buf: [256]u8 = [0; 256];
        * = renderJsonErrorBody(ctx.status_code, ctx.abort_reason, ctx.request_id, err_buf[0..]);
    }
    * = mw.stats();
}
```

### 对冲请求（Hedged Requests）与重试预算防风暴示例 (`hedged.cp`)

```cp
import {
    RetryBudgetConfig,
    RetryBudget,
    HedgedRequestConfig,
    HedgedAction,
    HedgedRequestTracker,
} from "cap:web";

pub fn demoHedgedRequestPolicy() {
    // 1. 重试预算：限制重试+对冲不超过正常流量 10%（1000 bps），每秒保底 2 次
    let budget = RetryBudget.init(RetryBudgetConfig{
        retry_ratio_bps: 1000,
        min_retries_per_sec: 2,
        window_ms: 1000,
        max_burst_retries: 20,
    }, 1000) ?? return;

    budget.recordRequest(1000);

    // 2. 幂等请求在主请求超过 50ms 未返回时自动发射对冲副本
    let tracker = HedgedRequestTracker.init(HedgedRequestConfig{
        hedge_delay_ms: 50,
        max_hedges: 2,
        total_timeout_ms: 300,
        idempotent_only: true,
    }, true, 1000) ?? return;

    const a1 = tracker.evaluate(1020, &budget); // HedgedAction.Wait
    const a2 = tracker.evaluate(1055, &budget); // HedgedAction.LaunchHedge (attempt 1)
    if (a2 == HedgedAction.LaunchHedge) {
        // 对冲副本 1 率先返回，自动取消仍在途的主请求 0
        const win = tracker.onReplicaSucceeded(tracker.latestAttemptIdx());
        * = win.cancelled_inflight; // 1
    }
    * = a1;
}
```

### 零分配 SSE 流式事件扇出代理与 `Last-Event-ID` 断线续传示例 (`sseBroker.cp`)

面向 LLM Token 流、实时通知推送与 MCP/A2A 流式事件的 100% 纯 CP 零动态堆分配 WHATWG/RFC 8895 SSE 扇出代理：

- **常量与模型**：`SSE_BROKER_MAX_HISTORY` (`64`)、`SSE_BROKER_MAX_CLIENTS` (`32`)、`SSE_EVENT_TYPE_MAX_LEN` (`32`)、`SSE_DATA_MAX_LEN` (`256`)、`SseStoredEvent`、`SseClientCursor`、`SsePollStatus`（`has_event | empty | lagged_resynced | client_not_found`）、`SsePollResult`、`SseBrokerStats`。
- **核心引擎 `SseFanoutBroker`**：
  - `publish(*_, topic_hash, event_type, data, retry_ms): u64?`：拦截 `event_type` 中的 `\r`/`\n`/`\0` 头注入字符，写入 64 槽位环形历史并返回单调递增 `event_id`。
  - `subscribe(*_, client_id, topic_filter_hash, last_event_id): bool`：支持全量（`topic_filter_hash == 0`）或按主题过滤，支持 `last_event_id` 断线精确补发（`0` 从当前队尾开始）。
  - `unsubscribe(*_, client_id): bool`：回收客户端槽位。
  - `poll_next(*_, client_id): SsePollResult`：慢消费者落后于环形最旧事件时自动报告 `lagged_resynced` + `skipped_events` 并对齐续传。
  - `SseFanoutBroker.formatEventWire(ev, out_buf): usize?`（及顶层 `formatSseEventWire`）：严格按 WHATWG SSE 线缆格式输出 `id:`、可选 `event:`/`retry:` 与多行拆分 `data:` 字段，防缓冲溢出。

```cp
import {
    SsePollStatus,
    SseFanoutBroker,
} from "cap:web";

pub fn demoSseFanoutBroker() {
    let broker = SseFanoutBroker.init();
    const id1 = broker.publish(10, "token", "hello\nworld", 3000) ?? return;
    const id2 = broker.publish(10, "token", "done", 0) ?? return;

    // 断线客户端携带 Last-Event-ID = id1 重连 -> 精准续传 id2
    * = broker.subscribe(1001, 10, id1);
    const res = broker.poll_next(1001);
    if (res.status == SsePollStatus.has_event) {
        if (ev := broker.get_event(res.event_idx)) {
            let wire: [512]u8 = [0; 512];
            let wire_s: []u8 = wire[0..];
            * = SseFanoutBroker.formatEventWire(ev, wire_s);
        }
    }
    * = id2;
}
```

### 零分配 Stripe 风格 API 幂等键并发锁与响应重放存储引擎 (`idempotency.cp`)

面向支付网关、计费 API、分布式 RPC 与 AI Agent 工具防重执行的 100% 纯 CP 零动态堆分配 Stripe 风格 `Idempotency-Key` 并发锁与响应重放引擎：

- **常量与数据模型**：
  - `IDEM_STORE_MAX_ENTRIES` (`64`)、`IDEM_KEY_MAX_LEN` (`64`)、`IDEM_BODY_MAX_LEN` (`256`)
  - `IdemEntryState`（`in_progress | completed`）、`IdemAcquireKind`（`acquired_new | replay_completed | conflict_in_progress | conflict_payload_mismatch | rejected_invalid | rejected_full`）
  - `IdemStoreConfig`（`lock_ttl_ms`、`result_ttl_ms`，提供 `defaults()` 与 `isValid()`）、`IdemRecord`（提供 `keySlice()` 与 `responseSlice()`）、`IdemAcquireDecision`、`IdemStoreStats`
- **核心引擎 `ApiIdempotencyStore`**：
  - `init(cfg): ApiIdempotencyStore?`：初始化固定 64 槽位内联幂等表（零堆分配）。
  - `begin_request(*_, tenant_id, key, payload_hash, now_ms): IdemAcquireDecision`：自动驱逐过期条目（含崩溃超时的 `in_progress` 锁）；命中同 `(tenant_id, key)` 时依次校验 `payload_hash` 不一致 -> `conflict_payload_mismatch`（HTTP 422）、在途 -> `conflict_in_progress`（HTTP 409）、已完成 -> `replay_completed`（直接重放缓存状态码与响应体）；未命中则加锁置 `in_progress`。
  - `commit_response(*_, slot_idx, status_code, response_body, now_ms): bool`：仅允许 `in_progress` 槽位提交响应并转入 `completed`，延长 TTL 为 `now_ms + result_ttl_ms`。
  - `abort_request(*_, slot_idx): bool`：可重试 5xx 异常时释放 `in_progress` 锁，允许客户端使用同 Key 安全重试。
  - `get_record(slot_idx): IdemRecord?` / `prune_expired(*_, now_ms): usize` / `stats(): IdemStoreStats`。

```cp
import {
    IdemAcquireKind,
    IdemStoreConfig,
    ApiIdempotencyStore,
} from "cap:web";

pub fn demoApiIdempotencyStore() {
    let store = ApiIdempotencyStore.init(IdemStoreConfig.defaults()) ?? return;

    // 1. 首次请求获得执行锁
    const d1 = store.begin_request(1001, "pay_ord_98765", 0xABCD1234, 10_000);
    if (d1.kind == IdemAcquireKind.acquired_new) {
        * = store.commit_response(d1.slot_idx, 201, "{\"charge_id\":\"ch_01\"}", 10_050);
    }

    // 2. 客户端网络抖动重试 -> 直接重放 201 与原响应体，避免重复扣款
    const d2 = store.begin_request(1001, "pay_ord_98765", 0xABCD1234, 12_000);
    if (d2.kind == IdemAcquireKind.replay_completed) {
        if (rec := store.get_record(d2.slot_idx)) {
            * = rec.status_code;     // 201
            * = rec.responseSlice(); // "{\"charge_id\":\"ch_01\"}"
        }
    }
}
```

---

### RFC 9110 §13 HTTP 条件请求与 ETag 强弱比较裁决引擎（`etagPrecond.cp`）

对标 `tower-rs/tower-http` (MIT) 与 RFC 9110 §8.8.3 / §13，面向 CDN、API 网关与对象存储的 100% 纯 CP 零动态堆分配条件请求状态机（集成 16-lane `@Vector(16, u8)` SIMD 字节比对与控制字符拦截）：

- **ETag 解析与强弱比较**：`parseEntityTag`（严格区分强标签 `"abc"` 与大写弱标签 `W/"abc"`，拒绝小写 `w/`、未闭合引号、控制字节/CRLF 及非法 `etagc` 字节）、`etagStrongEqual`、`etagWeakEqual`（热路径直连 `simdWebBytesEqual`）。
- **逗号列表与通配符 `*`**：`etagListMatchesStrong`、`etagListMatchesWeak`、`validateEtagList`（正确处理双引号内含逗号如 `"a,b", W/"c"` 与资源不存在时 `*` 语义）。
- **RFC 9110 §13.2.2 精确五步裁决（`evaluatePreconditions`）**：
  1. `If-Match`（强比较 -> 失败返回 `precondition_failed_412`）；
  2. 无 `If-Match` 时校验 `If-Unmodified-Since`（`last_modified > ius` -> `412`）；
  3. `If-None-Match`（弱比较 -> `GET/HEAD` 命中返回 `not_modified_304`，写方法命中返回 `412`）；
  4. 无 `If-None-Match` 且 `GET/HEAD` 时校验 `If-Modified-Since`（`last_modified <= ims` -> `304`）；
  5. `GET` + `Range` 时校验 `If-Range`（强 ETag 或精确时间戳匹配返回 `serve_range_206`，否则返回 `fallback_full_200`）。

```cp
import {
    PreconditionMethod,
    PreconditionRequest,
    ResourceValidators,
    evaluatePreconditions,
} from "cap:web";

pub fn demoEtagPrecondition() {
    const res = ResourceValidators{
        resource_exists: true,
        etag: "\"v42\"",
        last_modified_sec: 1700000000,
    };
    const req = PreconditionRequest{
        method: PreconditionMethod.get,
        if_none_match: "\"v41\", W/\"v42\"",
    };
    const out = evaluatePreconditions(req, res);
    * = out.http_status; // 304 (Not Modified via weak comparison)
}
```

### 零分配 W3C Fetch CORS 预检裁决与响应头合成器 (`corsGuard.cp`)

对标 `tower-rs/tower-http` (MIT) 与 W3C Fetch CORS 规范的 100% 纯 CP 零动态堆分配安全裁决与响应头合成器（集成 16-lane `@Vector(16, u8)` SIMD 加速原语 `simdWebHasControlOrCrlf` / `simdWebAsciiLowerEql` / `simdWebBytesEqual` 及对应标量参考）：

- **常量与安全匹配原语**：
  - `CORS_MAX_ORIGINS = 32`、`CORS_MAX_HEADERS = 32`、`CORS_MAX_ORIGIN_LEN = 128`、`CORS_MAX_HEADER_NAME_LEN = 64`、`CORS_METHOD_GET..OPTIONS` / `CORS_METHOD_ALL_STANDARD`。
  - `simdWebHasControlOrCrlf` / `scalarWebHasControlOrCrlf`、`simdWebAsciiLowerEql` / `scalarWebAsciiLowerEql`、`simdWebBytesEqual` / `scalarWebBytesEqual`、`hasControlOrCrlf(s)`、`isValidOriginSyntax(origin)`、`isValidCorsHeaderToken(name)`（RFC 7230 `tchar`）、`matchOriginPattern(pattern, origin)`（严格校对协议与点号边界）、`parseCorsMethodMask(method)`。
- **核心引擎 `CorsGuard` 与响应头合成**：
  - `addAllowedOrigin` / `setAllowedMethods` / `addAllowedHeader` / `addExposedHeader` / `setAllowCredentials` / `setAllowNullOrigin`（默认拒绝 `Origin: null`，仅显式配置放行）/ `setMaxAgeSecs`。
  - `evaluatePreflight(origin, req_method, req_headers_csv): CorsDecision`（`preflight_allow_204 | preflight_reject_origin | preflight_reject_method | preflight_reject_header`）。
  - `evaluateActualRequest(origin, method): CorsDecision`（`actual_allow | actual_reject_origin | not_cors_request`）。
  - 严格执行 W3C Fetch 凭据安全不变量：`allow_credentials == true` 时绝不反射 `*`，精确回显匹配 Origin 并自动注入 `Vary: Origin`（预检同时追加 `Access-Control-Request-Method, Access-Control-Request-Headers`）。
  - `formatCorsResponseHeaders(decision, out_buf): usize?`：一键合成标准 CRLF HTTP 响应头片段。

```cp
import {
    CORS_METHOD_GET,
    CORS_METHOD_POST,
    CorsGuard,
    formatCorsResponseHeaders,
} from "cap:web";

pub fn demoCorsGuard() {
    let guard = CorsGuard.init();
    * = guard.addAllowedOrigin("https://*.example.com");
    guard.setAllowedMethods(CORS_METHOD_GET | CORS_METHOD_POST);
    * = guard.addAllowedHeader("Authorization");
    * = guard.addAllowedHeader("Content-Type");
    guard.setAllowCredentials(true);
    guard.setMaxAgeSecs(600);

    const decision = guard.evaluatePreflight("https://api.example.com", "POST", "Authorization, Content-Type");
    let buf: [512]u8 = [0; 512];
    let buf_s: []u8 = buf[0..];
    * = formatCorsResponseHeaders(decision, *buf_s);
}
```

### 零分配 RFC 9111 HTTP 缓存新鲜度与 `Cache-Control` 裁决引擎 (`cachePolicy.cp`)

对标 `kornelski/rusty-http-cache-semantics` (BSD-2-Clause) 与 RFC 9111/9110，面向 CDN、API 网关与 HTTP 客户端缓存的 100% 纯 CP 零动态堆分配缓存策略引擎（集成 16-lane `@Vector(16, u8)` SIMD 原语 `simdWebSemAsciiLowerEql` / `simdWebSemFindDelim` 及对应标量参考）：

- **常量、枚举与数据模型**：
  - `CACHE_MAX_DELTA_SECONDS = 2147483648`
  - `CacheScope`（`private_cache | shared_cache`）、`CacheLookupVerdict`（`fresh_hit | stale_while_revalidate | stale_if_error | revalidate_required | miss_bypass`）
  - `CacheControl`、`StoredResponseMeta`、`CacheLookupRequest`、`CacheLookupDecision`
- **核心函数**：
  - `parseCacheControl(header: string): CacheControl?`：大小写无关解析全部 RFC 9111 / RFC 5861 指令（`no-store`, `no-cache`, `private`, `public`, `must-revalidate`, `proxy-revalidate`, `immutable`, `only-if-cached`, `max-age`, `s-maxage`, `min-fresh`, `max-stale[=N]`, `stale-while-revalidate`, `stale-if-error`），支持引号值与未知扩展跳过，拦截负数/溢出秒数、未闭合引号与 CRLF 注入。
  - `isResponseStorable(resp, scope): bool`：RFC 9111 §3/§3.5 可存储性判定（拦截 `no-store`、共享缓存拦截 `private` 与未授权缓存的 `Authorization` 响应）。
  - `computeFreshnessLifetime(resp, scope): u64` / `computeCurrentAge(resp, now_sec): u64`：RFC 9111 §4.2 精确新鲜度寿命（共享缓存 `s-maxage > max-age > Expires - Date > 10% (Date - Last-Modified)`；私有缓存忽略 `s-maxage`）与饱和当前年龄计算。
  - `evaluateCacheLookup(resp, req): CacheLookupDecision`：综合请求/响应指令、`must-revalidate`/`proxy-revalidate`、`stale-while-revalidate`、`stale-if-error` 与 `max-stale` 输出精确裁决。

```cp
import {
    CacheScope,
    CacheLookupVerdict,
    StoredResponseMeta,
    CacheLookupRequest,
    parseCacheControl,
    evaluateCacheLookup,
} from "cap:web";

pub fn demoCachePolicy() {
    const cc = parseCacheControl("public, s-maxage=60, stale-while-revalidate=30, stale-if-error=300") ?? return;
    const resp = StoredResponseMeta{
        status_code: 200,
        cache_control: cc,
        request_time_sec: 1000,
        response_time_sec: 1000,
    };
    const decision = evaluateCacheLookup(resp, CacheLookupRequest{
        scope: CacheScope.shared_cache,
        now_sec: 1075, // 过期 15s，位于 SWR 30s 窗口内
    });
    * = (decision.verdict == CacheLookupVerdict.stale_while_revalidate); // true
}
```

---

### 零分配 RFC 9110 HTTP 内容协商与 `Vary` 响应头引擎 (`contentNeg.cp`)

对标 `kornelski/rusty-http-cache-semantics` (BSD-2-Clause) 与 RFC 9110 §12 的 100% 纯 CP 零动态堆分配 **Content Negotiation & Vary** 引擎（热路径直连 16-lane SIMD 分隔符扫描与 ASCII 大小写不敏感比对）：

- **常量与枚举**：
  - `NEG_Q_MAX = 1000`（千分比整数权重 0..1000，零浮点漂移）、`NEG_MAX_ITEMS = 32`、`NEG_VARY_MAX_HEADERS = 16`
  - `ContentEncoding`（`identity | gzip | deflate | br | zstd`；提供 `asHeaderValue()`, `fromToken()`）
  - `NegotiationMatchKind`（`matched | not_acceptable | invalid_header`）
- **核心函数**：
  - `parseQValue(raw): u16?`：严格 RFC 9110 §12.4.2 千分比 q-value 解析（`0..=1000`，拒绝 `> 1.0`、超过 3 位小数与非法字符）。
  - `negotiateMediaType(accept_header, offered): MediaMatchResult`：RFC 9110 §12.5.1 `Accept` 媒体类型协商，支持 `type/subtype; param=v; q=0.8`、通配符 `type/*` 与 `*/*`、精度优先 + q-value + 服务端偏好序决胜，以及 `q=0` 显式拒绝（优先于 `*/*`）。
  - `negotiateEncoding(accept_encoding, offered): EncodingMatchResult`：RFC 9110 §12.5.3 `Accept-Encoding` 协商，支持 `zstd`/`br`/`gzip`/`deflate`/`identity`/`*`，精确处理 `identity;q=0` 与 `*;q=0` 返回 `not_acceptable` (406)。
  - `negotiateLanguage(accept_lang, offered): LanguageMatchResult`：RFC 9110 §12.5.4 + BCP 47 语言标签协商，支持大小写无关精确匹配、子标签前缀回退（`zh-Hans-CN` -> `zh-Hans` -> `zh`）、`*` 通配与 `q=0` 排除。
  - `formatVaryHeader(existing_vary, add_headers, *out_buf): usize?`：零分配 `Vary` 头大小写无关去重合并，遵循 RFC 9110 §12.5.5 遇 `*` 自动折叠为 `*`，严格拦截 CRLF/控制字符注入。

```cp
import {
    ContentEncoding,
    negotiateMediaType,
    negotiateEncoding,
    negotiateLanguage,
    formatVaryHeader,
} from "cap:web";

pub fn demoContentNeg() {
    const media_offered: [2][]const u8 = ["application/json", "text/html"];
    const m = negotiateMediaType("text/html;q=0.5, application/json;q=0.9", media_offered[0..]);
    * = m.offered_index; // 0 (application/json)

    const enc_offered: [3]ContentEncoding = [ContentEncoding.zstd, ContentEncoding.br, ContentEncoding.gzip];
    const e = negotiateEncoding("gzip;q=0.8, br;q=1.0", enc_offered[0..]);
    * = e.encoding; // ContentEncoding.br

    const lang_offered: [2][]const u8 = ["zh-Hans", "en"];
    const l = negotiateLanguage("zh-Hans-CN, en;q=0.5", lang_offered[0..]);
    * = l.offered_index; // 0 (zh-Hans 前缀回退命中)

    let vary_buf: [128]u8 = [0; 128];
    let vary_s: []u8 = vary_buf[0..];
    const add: [2][]const u8 = ["Accept-Encoding", "accept-language"];
    * = formatVaryHeader("Accept, accept-encoding", add[0..], *vary_s);
}
```

---

### 零分配 IETF `RateLimit`/`RateLimit-Policy` 与 `Retry-After` 限流响应头编解码与退避规划器 (`rateHeader.cp`)

对标 `kornelski/rusty-http-cache-semantics` (BSD-2-Clause) 与 IETF RateLimit / RFC 9110 §10.2.3 的 100% 纯 CP 零动态堆分配限流响应头编解码与重试退避规划引擎（16-lane SIMD 加速参数解析与饱和时间差防溢出）：

- **常量与数据模型**：
  - `RATE_HDR_MAX_NAME_LEN = 48`、`RATE_HDR_MAX_PK_LEN = 64`
  - `RateLimitHeader`（`policyNameSlice`, `setPolicyName`, `limit`, `remaining`, `reset_s`）
  - `RateLimitPolicyHeader`（`policyNameSlice`, `partitionKeySlice`, `quota`, `window_s`, `concurrency`）
  - `RetryAfterKind`（`delta_seconds | http_date`）、`RetryAfterValue`
  - `RetryDelaySource`（`retry_after_delta | retry_after_date | rate_limit_reset | exponential_backoff`）、`RateLimitHeaderInfo`、`RetryPlan`
- **IETF `RateLimit` / `RateLimit-Policy` 编解码**：
  - `parseRateLimitHeader(raw): RateLimitHeader?` / `formatRateLimitHeader(hdr, structured_sf, *out_buf): usize?`：双向兼容 `limit=100, remaining=42, reset=15` 与结构化字段引号策略 `"default";r=42;t=15`。
  - `parseRateLimitPolicyHeader(raw): RateLimitPolicyHeader?` / `formatRateLimitPolicyHeader(pol, structured_sf, *out_buf): usize?`：支持 `100;w=60` 与 `"burst";q=100;w=60;c=10;pk=:shard1:`（含并发与分区参数）。
- **RFC 9110 `Retry-After` 与传统 `X-RateLimit-*` 编解码**：
  - `parseRetryAfter(raw): RetryAfterValue?` / `formatRetryAfterDelta(seconds, *out_buf)` / `formatRetryAfterHttpDate(epoch_s, *out_buf)`：同时支持非负 delta-seconds 与 IMF-fixdate HTTP 日期（精确星期与闰年校验）。
  - `parseLegacyRateLimitHeaders(limit_hdr, remaining_hdr, reset_hdr, now_epoch_s)` / `formatLegacyRateLimitHeaders(hdr, reset_as_epoch, now_epoch_s, *out_buf)`：自动区分 Unix epoch 与相对秒数。
- **统一客户端退避规划器**：
  - `planRateLimitRetry(info, attempt, now_epoch_s, max_delay_ms): RetryPlan`：严格按 `Retry-After` > `RateLimit.reset` > 指数退避优先级计算等待时间并施加 `max_delay_ms` 上限钳制。

```cp
import {
    RateLimitHeaderInfo,
    parseRateLimitHeader,
    parseRetryAfter,
    planRateLimitRetry,
} from "cap:web";

pub fn demoRateHeader() {
    let info = RateLimitHeaderInfo{};
    if (rl := parseRateLimitHeader("\"default\";r=0;t=15")) {
        info.has_rate_limit = true;
        info.rate_limit = rl;
    }
    if (ra := parseRetryAfter("30")) {
        info.has_retry_after = true;
        info.retry_after = ra;
    }
    // 优先尊重 Retry-After (30s)，并钳制在 max_delay_ms=20000ms
    const plan = planRateLimitRetry(info, 1, 1700000000, 20000);
    * = plan.delay_ms; // 20000 (clamped = true)
}
```

---

### 零分配无状态 HMAC-SHA256 CSRF 令牌与 `Sec-Fetch-Site`/`Origin` 纵深防护引擎 (`csrfGuard.cp`)

对标 `actix/actix-web` (MIT OR Apache-2.0) 与 RFC 6797，面向现代 Web 应用纵深防御的 100% 纯 CP 零动态堆分配、零 C 依赖 CSRF 防护引擎（集成 16-lane `@Vector(16, u8)` 无早退常量时间比对 `simdWebAuthCtEqual`、`simdWebAuthAsciiLowerEql` 与 `std.crypto.secureZero` 栈密钥擦除）：

- **常量与枚举**：
  - `CSRF_NONCE_LEN = 16`、`CSRF_TOKEN_LEN = 76`、`CSRF_MAX_TRUSTED_ORIGINS = 16`、`CSRF_MAX_ORIGIN_LEN = 128`、`CSRF_DEFAULT_TTL_S = 3600`
  - `CsrfTokenStatus`（`valid | invalid | expired`）
  - `CsrfVerdict`（`allowed_safe_method | allowed_verified | denied_cross_site | denied_origin_mismatch | denied_missing_token | denied_invalid_token | denied_expired_token`）
- **无状态令牌签发与验签**：
  - `mintCsrfToken(secret, session_id, issued_at_s, nonce, out_buf: *[]u8): usize?`：内联纯 CP FIPS 180-4 SHA-256 + RFC 2104 HMAC-SHA256 + 无填充 Base64URL，输出 76 字节会话绑定令牌。
  - `verifyCsrfToken(secret, session_id, token, now_s, ttl_s): CsrfTokenStatus`：常量时间验签 + 会话绑定防跨会话重放 + TTL 时效窗口检查。
  - `csrfConstantTimeEq(a, b): bool`：常量时间字节切片比对。
- **纵深防御裁决引擎 `CsrfGuard`**：
  - `init(ttl_s, allow_same_site, require_origin_header)` / `addTrustedOrigin(*_, origin)` / `stats()` / `resetStats(*_)`
  - `evaluateCsrfRequest(*_, req, secret, now_s): CsrfDecision`：安全幂等方法（`GET`/`HEAD`/`OPTIONS`）自动放行；非安全方法依次校验 `Sec-Fetch-Site`、`Origin` 白名单/`Host` 同源、可选双提交 Cookie 常量时间一致及 HMAC-SHA256 会话令牌。

```cp
import {
    CsrfRequest,
    CsrfVerdict,
    CsrfGuard,
    mintCsrfToken,
} from "cap:web";

pub fn demoCsrfGuard() {
    const secret = "prod-hmac-secret-32-bytes-minimum";
    let tok_buf: [80]u8 = [0; 80];
    let tok_s: []u8 = tok_buf[0..];
    const n = mintCsrfToken(secret, "sess-42", 1_700_000_000, "0123456789abcdef", *tok_s) ?? return;

    let guard = CsrfGuard.init(3600, false, true);
    const req = CsrfRequest{
        method: "POST",
        session_id: "sess-42",
        sec_fetch_site: "same-origin",
        origin: "https://app.cap.dev",
        host: "app.cap.dev",
        submitted_token: tok_buf[0..n],
        cookie_token: tok_buf[0..n],
    };
    const res = guard.evaluateCsrfRequest(req, secret, 1_700_000_100);
    * = (res.verdict == CsrfVerdict.allowed_verified);
}
```

### HMAC-SHA256 预签名 URL 与多密钥轮换验签器 (`signedUrl.cp`)

对标 `actix/actix-web` (MIT OR Apache-2.0)，100% 纯 CP、零动态堆分配、零 C 依赖的预签名 URL 生成与 16-lane SIMD 常量时间验签引擎（`std.crypto.secureZero` 栈密钥与摘要擦除）：

- **常量与枚举**：
  - `SIGNED_URL_SIG_LEN = 43`、`SIGNED_URL_MAX_KEYS = 8`、`SIGNED_URL_MAX_KID_LEN = 32`、`SIGNED_URL_MAX_SECRET_LEN = 128`、`SIGNED_URL_DEFAULT_CLOCK_SKEW_S = 30`
  - `SignedUrlStatus`（`valid | missing_signature | missing_expiry | expired | invalid_signature | method_mismatch | malformed_url`）
  - `SignedUrlVerifyResult`（`status`, `valid`, `exp_epoch_s`, `kid()`）、`SignedUrlKeyringStats`
- **无状态预签名与常量时间验签**：
  - `signUrl(secret, method, path, query_without_sig, exp_epoch_s, key_id, *out_buf): usize?`：将 `(method, path, query_without_sig, exp_epoch_s, key_id)` 域分离规范化后签发带 `exp`、可选 `kid` 与 43 字符 `sig` 的预签名 URL；内置 CRLF/空路径/重复保留参数/缓冲溢出守卫。
  - `verifySignedUrl(secret, method, full_path_and_query, now_epoch_s, clock_skew_s): SignedUrlVerifyResult`：流式剥离 `sig`/`exp`/`kid` 零堆验签，校验时钟偏移窗口，精确区分 `method_mismatch` 与 `invalid_signature`。
  - `signedUrlConstantTimeEq(a, b)` / `formatSignedUrlStatus(status)`
- **多密钥零停机轮换引擎 `SignedUrlKeyring`**：
  - `init()` / `addKey(*_, kid, secret, make_primary)` / `setPrimaryKey(*_, kid)` / `revokeKey(*_, kid)` / `hasKey(kid)` / `keyCount()`
  - `signWithPrimary(*_, method, path, query_without_sig, exp_epoch_s, *out_buf)` / `signWithKey(*_, kid, ...)`
  - `verify(*_, method, full_path_and_query, now_epoch_s, clock_skew_s): SignedUrlVerifyResult`（按 `kid` 精准定位密钥验签，无 `kid` 回退主密钥）
  - `stats(): SignedUrlKeyringStats` / `resetStats(*_)`

```cp
import {
    SignedUrlStatus,
    SignedUrlKeyring,
    SIGNED_URL_DEFAULT_CLOCK_SKEW_S,
} from "cap:web";

pub fn demoSignedUrlKeyring() {
    let kr = SignedUrlKeyring.init();
    * = kr.addKey("k2026a", "secret-key-alpha-32bytes-minimum", true);
    * = kr.addKey("k2026b", "secret-key-beta-32bytes-minimum!", true); // 平滑轮换主密钥

    let url_buf: [256]u8 = [0; 256];
    let url_s: []u8 = url_buf[0..];
    const n = kr.signWithPrimary("GET", "/files/report.pdf", "dl=1", 1_700_003_600, *url_s) ?? return;

    const vr = kr.verify("GET", url_buf[0..n], 1_700_000_100, SIGNED_URL_DEFAULT_CLOCK_SKEW_S);
    * = (vr.status == SignedUrlStatus.valid);
}
```

### W3C Content-Security-Policy (CSP Level 3) 解析、源匹配裁决与策略构建器 (`cspPolicy.cp`)

对标 `tower-rs/tower-http` (MIT) 与 W3C CSP Level 3 规范的 100% 纯 CP、零动态堆分配响应头解析、资源与内联脚本安全裁决及规范序列化引擎（集成 16-lane SIMD 控制字节与大小写不敏感匹配）：
- **安全与规范加固**：重复指令仅首条生效、指令名 ASCII 大小写不敏感、`'none'` 仅单独出现时生效（`isEffectiveNone()`）、严格 Base64/Base64URL 校验 `'nonce-'`/`'sha256-'`/`'sha384-'`/`'sha512-'`、拒绝控制字符/CRLF/超限指令数。

- **常量与枚举**：
  - `CSP_MAX_DIRECTIVES = 16`、`CSP_MAX_SOURCES_PER_DIR = 16`、`CSP_MAX_TOKEN_LEN = 96`
  - `CspDirectiveKind`（`default_src | script_src | style_src | img_src | connect_src | font_src | frame_src | frame_ancestors | object_src | base_uri | form_action | upgrade_insecure_requests`）
  - `CspSourceKind`（`kw_none | kw_self | kw_unsafe_inline | kw_unsafe_eval | kw_strict_dynamic | wildcard | scheme | host_expr | nonce | hash_sha256 | hash_sha384 | hash_sha512`）
  - `CspEvalVerdict`（`allowed | blocked_by_none | blocked_no_match | blocked_unsafe_inline | invalid_input`）
- **核心模型与引擎**：
  - `CspSourceExpr`（`kind`, `token()`, `parse(raw): CspSourceExpr?`）
  - `CspDirective`（`kind`, `source_count`, `sources`, `hasNone()`, `hasStrictDynamic()`, `hasUnsafeInline()`, `hasNonceOrHash()`）
  - `CspPolicy`：
    - `init()` / `parseHeader(header): CspPolicy?`（按 `;` 拆解指令、遵循 W3C 首条生效原则忽略重复指令、阻断 CRLF/控制字符注入）
    - `addSource(*_, dir, raw_source): bool` / `setUpgradeInsecure(*_, enabled)`
    - `effectiveDirective(dir): CspDirective?`（严格遵循 W3C 回退链：fetch 指令缺失时回退 `default-src`，`base-uri`/`form-action`/`frame-ancestors` 绝不回退 `default-src`）
    - `checkResourceUrl(dir, page_origin, target_url): CspEvalVerdict`（支持 `'none'`、`'self'`、`*`、协议源、精确主机及严格 `*.example.com` 子域通配匹配）
    - `checkInlineScript(nonce_val, sha256_b64): CspEvalVerdict`（严格遵循 CSP3：存在 `nonce`/`hash`/`'strict-dynamic'` 时 `'unsafe-inline'` 自动失效）
    - `formatHeader(*out_buf): usize?`

```cp
import {
    CspDirectiveKind,
    CspEvalVerdict,
    CspPolicy,
} from "cap:web";

pub fn demoCspPolicy() {
    let pol = CspPolicy.init();
    * = pol.addSource(CspDirectiveKind.default_src, "'self'");
    * = pol.addSource(CspDirectiveKind.script_src, "'nonce-r4nd0m'");
    * = pol.addSource(CspDirectiveKind.script_src, "'strict-dynamic'");
    pol.setUpgradeInsecure(true);

    * = (pol.checkInlineScript("r4nd0m", "") == CspEvalVerdict.allowed);
    * = (pol.checkInlineScript("", "") == CspEvalVerdict.blocked_unsafe_inline);

    let hdr_buf: [256]u8 = [0; 256];
    let hdr_s: []u8 = hdr_buf[0..];
    * = pol.formatHeader(*hdr_s);
}
```

### RFC 6797 HTTP Strict-Transport-Security (HSTS) 解析、域名表与 HTTPS 自动升级引擎 (`hstsStore.cp`)

对标 `actix/actix-web` (MIT OR Apache-2.0) 与 RFC 6797，100% 纯 CP、零动态堆分配的 HSTS 策略解析器、预加载/动态域名表与 16-lane SIMD HTTPS 自动升级裁决引擎：

- **常量与错误集**：
  - `HSTS_MAX_DOMAINS = 64`、`HSTS_HOST_MAX_LEN = 128`、`HSTS_PRELOAD_MIN_MAX_AGE = 31_536_000`
  - `HstsError`（`EmptyHeader | MissingMaxAge | DuplicateDirective | InvalidMaxAge | CrlfRejected | InvalidHost | IpLiteralDisallowed | StoreFull | BufferTooSmall`）
- **数据模型与解析/格式化器**：
  - `HstsPolicy`（`max_age_sec`, `include_subdomains`, `preload`, `isPreloadEligible()`）
  - `HstsEntry`（`host()`, `expires_at_sec`, `include_subdomains`, `is_preload`）、`HstsLookupResult`、`HstsStoreStats`
  - `parseHstsHeader(header): HstsPolicy ! HstsError`（大小写不敏感指令、支持引号包裹 `max-age="31536000"`、严格拒绝重复指令/非法数值/CRLF）
  - `formatHstsHeader(policy, *out_buf): usize ! HstsError`
  - `isHstsEligibleHost(host): bool`（遵循 RFC 6797 §8.1.1 拒绝 IPv4/IPv6 字面量，自动规范化小写与尾点）
- **核心存储引擎 `HstsStore`**：
  - `init()` / `addPreload(*_, host, include_subdomains)`（不可变预加载条目，永不过期且不被动态 `max-age=0` 撤销）
  - `applyHeader(*_, host, header, now_sec): bool ! HstsError`（`max-age=0` 即时移除动态条目，`max-age > 0` 插入/续期）
  - `shouldUpgrade(*_, host, now_sec): HstsLookupResult`（精确匹配优先，否则逐级父域查 `includeSubDomains`）
  - `evictExpired(*_, now_sec): usize` / `stats(): HstsStoreStats`

```cp
import { HstsStore } from "cap:web";

pub fn demoHstsStore() {
    let store = HstsStore.init();
    * = store.applyHeader("example.com", "max-age=31536000; includeSubDomains; preload", 1_700_000_000) catch false;
    const res = store.shouldUpgrade("api.example.com", 1_700_000_100);
    * = res.must_upgrade; // true (matched_via_subdomain == true)
}
```

---

## 四、 `cap:web` 旗舰 REST API 框架（ElysiaJS 生命周期 + Axum 零反射内核）

> 完整可运行示例：[examples/05_web_rest_api.cp](../../examples/05_web_rest_api.cp)（`cap run examples/05_web_rest_api.cp`）

`cap:web` 旗舰 REST 框架由 4 个零动态堆分配核心模块构成：`radixRouter.cp`、`app.cp`、`schema.cp` 与 `fileType.cp`，全链路无 GC、无运行时反射，支持纯内存零 I/O `app.dispatch(req)` 测试与生产服务器挂载。

### 1. 混合极速路由器 (`lib/web/radixRouter.cp`)

融合 **Memoirist O(1) FNV-1a 静态路径哈希表** 与 **Matchit 压缩段 Radix Trie**，按「静态段 > 参数段 (`:id` / `{id}`) > 通配段 (`*wildcard`)」优先级精确匹配：

| 符号 / 方法 | 签名 / 说明 |
| :--- | :--- |
| `RadixRouter.init()` | 初始化零堆路由器（512 静态桶 + 512 Trie 节点 + 7 方法槽位） |
| `insert(*_, method_idx, path, handler_id): bool` | 注册静态或动态路由（`0=GET, 1=POST, 2=PUT, 3=DELETE, 4=PATCH, 5=HEAD, 6=OPTIONS`），同时支持 Elysia `:param`、Axum `{param}` 与 `*wildcard` |
| `lookup(*_, method_idx, path): RadixMatchResult` | O(1) 静态查表 + O(depth) Trie 回退；内置 **Axum 风格 `HEAD -> GET` 自动回退**（`head_fallback_to_get = true`）与 **405 `allowed_mask` 位掩码收集** |
| `staticRouteCount()` / `trieNodeCount()` | 查询静态路由数与 Radix Trie 节点数 |

### 2. 旗舰应用内核、8 阶段生命周期、Plugin、Axum 组合与 Cookie (`lib/web/app.cp`)

#### 2.1 Elysia 8 阶段请求生命周期管道

`app.dispatch(req)` 严格按以下确定性流水线执行（任一前置钩子返回 `false` 或调用 `ctx.abort()` / `ctx.fail()` 时立即短路并进入响应收尾阶段）：

1. **`onRequest`**（`app.onRequest`）→ 全局洋葱中间件（`app.use`）
2. **路由匹配**（`RadixRouter.lookup`：静态 O(1) / Radix Trie + `HEAD -> GET` 自动回退；405 自动生成 `Allow` 头；404 走 `fallback` 或 `onError`）
3. **`derive`**（`app.derive`）：按请求动态派生上下文属性（`ctx.setDerived(k, v)` / `ctx.derived(k)`）
4. **`onParse`**（`app.onParse`）：请求体自定义解析钩子
5. **`onTransform`**（`app.onTransform`）：入参规范化与预处理钩子
6. **`onBeforeHandle`**（`app.onBeforeHandle`）：路由前鉴权/守卫钩子（可用 `ctx.fail(AppErrorCode, msg)` 触发 `onError`）
7. **`handler` 执行** → **`onAfterHandle`**（`app.onAfterHandle`）→ 合并 `ctx.setHeader` 响应头
8. **`mapResponse`**（`app.mapResponse`）→ **`onAfterResponse`**（`app.onAfterResponse`）；任意阶段错误统一路由至 **`onError`**（`app.onError`）

#### 2.2 核心 API 速查表

| 分类 | API / 符号 | 说明 |
| :--- | :--- | :--- |
| **链式路由与分组** | `App.init()`, `.get/post/put/delete/patch/head/options(path, fn)`, `.route(method, path, fn)`, `.routeDoc(method, path, summary, fn)`, `.group(prefix)` | 链式注册路由与 `RouteGroup` 前缀分组，支持 `:param`、`{param}` 与 `*wildcard` |
| **8 阶段钩子与错误处理** | `.onRequest`, `.derive`, `.onParse`, `.onTransform`, `.onBeforeHandle`, `.onAfterHandle`, `.mapResponse`, `.onAfterResponse`, `.onError`, `.use` | `LifecycleHookFn = *const fn (*Context): bool`、`ResponseHookFn = *const fn (*Context, *Response): void`、`ErrorHookFn = *const fn (*Context, AppErrorCode, string): Response` |
| **Elysia Plugin 组合** | `Plugin.init(name)`, `.withPrefix(p)`, `.withScope(PluginScope)`, `.setState(k, v)`, `.get/post/.../route`, `.onRequest/derive/onBeforeHandle/onAfterHandle`, `app.usePlugin(*plug)` | 零堆模块化插件容器（`PluginScope`: `Local | Scoped | Global`），挂载路由、前缀、状态与钩子 |
| **Axum 组合与兜底** | `app.nest(prefix, *sub)`, `app.merge(*other)`, `app.fallback(handler)`, `app.state(ptr)`, `app.setState(k, v)` | 支持子路由嵌套挂载、多路由/状态合并与自定义 404 Fallback |
| **Context 零反射提取器** | `ctx.param(k)`, `ctx.paramU64(k)`, `ctx.paramI64(k)`, `ctx.query(k)`, `ctx.queryU64(k)`, `ctx.queryI64(k)`, `ctx.queryBool(k)`, `ctx.header(k)`, `ctx.bearerToken()`, `ctx.matchedPath()`, `ctx.originalUri()`, `ctx.stateVal(k)`, `ctx.stateAddr()`, `ctx.derived(k)` | 编译期零反射提取路径/查询/请求头/Bearer/状态/派生属性；响应构造：`ctx.json`, `ctx.text`, `ctx.noContent`, `ctx.status`, `ctx.setHeader`, `ctx.abort`, `ctx.fail` |
| **RFC 6265 Cookie** | `ctx.cookie(name): string?`, `CookieOptions`, `CookieSameSite` (`None | Lax | Strict`), `formatSetCookie(name, val, opts, out_buf): usize?` | 零堆解析请求 `Cookie` 头（自动解引号）与生成标准 `Set-Cookie`（`Path`/`Domain`/`Max-Age`/`HttpOnly`/`Secure`/`SameSite`） |
| **OpenAPI 3.1.0 导出** | `app.toOpenApiJson(title, version, out_buf): usize?` | 零堆导出完整 OpenAPI 3.1.0 JSON 文档（自动将 `:param`/`*wildcard` 转为 `{param}` 并生成 `parameters`） |

### 3. SIMD URI 解码、零反射 Schema 校验与 `exact-mirror` (`lib/web/schema.cp`)

| 分类 | API / 符号 | 说明 |
| :--- | :--- | :--- |
| **16-lane SIMD URI 解码** | `fastDecodeUriComponent(in, out)`, `fastDecodeFormComponent(in, out)`, `fastDecodeUriComponentOpts(in, out, plus_as_space)` | 基于 `@Vector(16, u8)` 跳过无 `%`/`+` 块，严格校验 `%XX` 十六进制转义 |
| **零拷贝 Query 解析** | `parseQueryPair(seg): QueryPair`, `findQueryParamDecoded(query, key, out_buf): usize?` | 零堆查找并自动解码 Query 参数 |
| **Schema 构建器** | `ObjectSchema.init(name)`, `.strField`, `.intField`, `.floatField`, `.boolField`, `.emailField`, `.uuidField`, `.urlField`, `.enumField`, `.addRule`, `.findField` | 链式声明最多 32 个字段规则（`FieldKind`: `Str | Int | Float | Bool | Email | Uuid | Url | Enum`） |
| **JSON 校验与裁剪** | `validateJsonObject(json_body, *schema): ValidationReport`, `exactMirrorJson(json_body, *schema, out_buf): usize?` | 零堆校验必填/类型/长度/范围/格式；`exactMirrorJson` 严格剔除未声明敏感字段（对标 Elysia `exact-mirror`） |
| **OpenAPI Schema 导出** | `schemaToOpenApiJson(*schema, out_buf): usize?` | 导出符合 OpenAPI 3.1.0 / JSON Schema Draft 2020-12 的对象 Schema 片段 |

### 4. 零堆魔数文件类型检测器 (`lib/web/fileType.cp`)

1:1 替代 Elysia `file-type` 依赖，零堆安全检测 **30 种**常见二进制魔数与文本启发式格式（越界/截断输入安全返回 `null`）：

| API / 符号 | 说明 |
| :--- | :--- |
| `detectFileType(bytes: string): FileTypeInfo?` | 返回 `{ kind: FileKind, ext: string, mime: string }` 或 `null` |
| `FileKind`（30 种格式） | **图像**：`Png`, `Jpeg`, `Gif`, `Webp`, `Avif`, `Bmp`, `Ico`；**文档与压缩**：`Pdf`, `Zip`, `Gzip`, `Zstd`, `Xz`, `Bz2`, `Lz4`；**音视频**：`Mp4`, `Webm`, `Mp3`, `Flac`, `Ogg`, `Wav`；**Wasm/字体/DB**：`Wasm`, `Woff2`, `Woff`, `Ttf`, `Otf`, `Sqlite3`；**Web 文本**：`Svg`, `Html`, `Xml`, `Json` |

---

## 收敛子模块深度文档

- [circuit.md](web/circuit.md)
- [feed.md](web/feed.md)
- [http2.md](web/http2.md)
- [http3.md](web/http3.md)
- [msgpack_rpc.md](web/msgpack_rpc.md)
- [multipart.md](web/multipart.md)
- [openapi.md](web/openapi.md)
- [opml.md](web/opml.md)
- [picohttp.md](web/picohttp.md)
- [ratelimit.md](web/ratelimit.md)
- [selector.md](web/selector.md)
- [sitemap.md](web/sitemap.md)
- [sse.md](web/sse.md)
- [uritemplate.md](web/uritemplate.md)
- [webhook.md](web/webhook.md)
- [xpath.md](web/xpath.md)

## 历史 Web 路由域散装模块收敛（W29-Sync R5）

以下 2 个原独立顶层目录已统一收敛至 `cap:web`（`lib/web/`），可直接从 `"cap:web"` 导入：

| 原顶层目录 | 收敛目标文件 | 主要导出符号 |
|---|---|---|
| `cap:route_params` | `lib/web/routeParams.cp` | `Param`, `ParamMap`, `URLPath`, `parseURLPath` |
| `cap:fs_router` | `lib/web/fsRouter.cp` | `RouteKind`, `SegmentKind`, `RouteSegment`, `RouteParam`, `FsRouteMatch`, `RoutePattern`, `FsRouter` |

---

## 最新核心框架增强 (`app.cp` / `server.cp` / `wsServer.cp` / `comptimeRouter.cp`)

### 1. 请求级零 GC Arena (`RequestArena`) 与应用内核 (`lib/web/app.cp`)
- **`RequestArena`**：24 字节轻量 8B 对齐 Bump-Pointer Arena（`initSlice`/`initRaw`/`alloc`/`dupe`/`reset`/`used`/`remaining`），挂载于 `Context.arena`，通过 `ctx.arenaAlloc(n): ([]u8)?` 与 `ctx.arenaDupe(s): string?` 使用；小数据路由零触碰零开销。
- **Elysia / Axum 完备融合**：
  - `Response` 快捷构造：`redirect` / `redirectTemporary`(302) / `redirectPermanent`(308) / `html` / `bytes`；
  - `Context` 提取器：`form(key)`（`application/x-www-form-urlencoded`）、`multipartBoundary()` / `isMultipart()`；
  - 单路由与分组级守卫：`getWithGuard`/`postWithGuard`/`routeLayer`/`RouteGroup.guard()`（支持子分组继承）；
  - `hook_mask: u16` 零钩子快路径 + `App.enableTrace(bool)` 自动输出 W3C `Server-Timing` 响应头。

### 2. 多核原生 HTTP/WS 服务器 (`lib/web/server.cp` & `lib/web/wsServer.cp`)
- **`WebServer`**：`SO_REUSEPORT` 多核 + CPU 绑核 + 12B 紧致 `epoll` + 16B SIMD `srvFindCrlf` + 就地 `parseHttp1Request` + 每 Worker 预分配 64KB `RequestArena`（大响应体自动绕过 4KB 写缓冲零拷贝流式直发）+ `startWithComptime(*app, &ct_table)` + 同端口 HTTP→WS 升级（`ServerConfig.ws_path` / `ws_broadcast_mode`）+ 优雅停机排空（`shutdown_grace_ms`）。
- **`WsServer`**：32B `@Vector(32, u8)` SIMD 掩码 + RFC 6455 握手 + 多核 Echo + `broadcast_mode: true` Topic Cork 零拷贝广播。

### 3. 编译期路由表 (`lib/web/comptimeRouter.cp`)
- `ComptimeRouteTable` 同时支持静态路由（64-bit 长度位图 + FNV-1a + 16B SIMD 比较）与编译期预切分 `:param`/`{param}` 动态路由（首段哈希预过滤），提供零拷贝 `dispatchInto(&req, *ctx, *resp)` 与 NestJS 风格 `ControllerSpec` / `controller(prefix, routes)`。

### 4. 基础依赖独立归位（`lib/web/` 保留兼容 re-export 薄壳）
- `fileType.cp` → `cap:mime/fileType`（`lib/mime/fileType.cp`）
- `schema.cp` → `cap:json/schema`（`lib/json/schema.cp`）+ `cap:url/fastDecode`（`lib/url/fastDecode.cp`）
- `radixRouter.cp` → `cap:router/radixRouter`（`lib/router/radixRouter.cp`）
