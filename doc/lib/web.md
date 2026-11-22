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
| `lib/web/picohttp.cp` | 零分配 HTTP/1.1 请求/响应与 Chunked 解析 | `parseRequest`, `parseResponse`, `parseHeaders`, `findHeader`, `parseContentLength`, `ChunkedDecoder`, `ParsedRequest`, `ParsedResponse` |
| `lib/web/http2.cp` | RFC 9113 HTTP/2 二进制帧、流状态与流控 | `encodeFrame`, `FrameStreamDecoder`, `FlowWindow`, `Http2Stream`, `encodeSettings`, `decodeSettings`, `encodePing`, `encodeGoaway`, `encodeWindowUpdate` |
| `lib/web/http3.cp` | RFC 9114 HTTP/3 帧与 QPACK 静态/动态头编解码 | `encodeH3Frame`, `decodeFrames`, `decodeFramesWithMaxSize`, `freeFrames`, `encodeQpackHeaders`, `decodeQpackHeaders`, `H3_DEFAULT_MAX_FRAME_SIZE`, `H3Frame` |
| `lib/web/sse.cp` | Server-Sent Events (SSE) 流式解析与编码 | `SseParser`, `SseEncoder`, `SseEvent`, `SseError` |
| `lib/web/multipart.cp` | RFC 7578 `multipart/form-data` 解析与构造 | `extractBoundary`, `parse`, `parseWithLimits`, `MultipartBuilder`, `MultipartDocument`, `MultipartPart`, `MultipartError` |
| `lib/web/uritemplate.cp` | RFC 6570 Level 1–4 URI Template 展开 | `UriTemplate`, `TemplateVars`, `AssocPair`, `TemplateOp`, `UriTemplateError` |

### 3. 内容聚合、爬虫协议与浏览器/DOM 查询（5 个子模块）

| 子模块文件 | 领域分类 | 核心导出符号 |
| :--- | :--- | :--- |
| `lib/web/sitemap.cp` | RFC 9309 `robots.txt` 与 XML Sitemap 0.9 | `RobotsRules`, `SitemapCodec`, `SitemapUrl`, `SitemapIndexItem`, `ChangeFreq`, `SitemapKind` |
| `lib/web/feed.cp` | RSS 2.0 / Atom 1.0 / JSON Feed 1.1 | `FeedCodec`, `Feed`, `FeedItem`, `FeedFormat`, `FeedError` |
| `lib/web/opml.cp` | 零分配 OPML 2.0 订阅大纲解析与格式化 | `isOpmlDocument`, `parseOpmlHead`, `OpmlOutlineIter`, `formatOpmlFeedOutline`, `OpmlHead`, `OpmlOutline` |
| `lib/web/selector.cp` | CSS3 选择器编译与 DOM 匹配引擎 | `CssSelector`, `ComplexSelector`, `CompoundSelector`, `AttrSelector`, `PseudoSelector`, `SelectorError` |
| `lib/web/xpath.cp` | 零分配 W3C XPath 词法与步进扫描器 | `XPathIter`, `XPathStepIter`, `validateXPathDelimiters`, `XPathToken`, `XPathStep` |

### 4. API 网关流量治理与容错（2 个子模块）

| 子模块文件 | 领域分类 | 核心导出符号 |
| :--- | :--- | :--- |
| `lib/web/ratelimit.cp` | 确定性限流器 (令牌桶/滑动窗口/GCRA/分键限流) | `TokenBucket`, `SlidingWindowCounter`, `GcraLimiter`, `KeyedRateLimiter`, `RateDecision`, `RateLimitError` |
| `lib/web/breaker.cp` / `breakerPol.cp` | 熔断器、指数退避抖动与舱壁隔离 | `CircuitBreaker`, `CircuitConfig`, `CircuitState`, `CircuitStats`, `BackoffPolicy`, `Bulkhead`, `JitterMode` |
| `lib/web/middleware.cp` | 零堆分配 HTTP 洋葱中间件链、CORS/Bearer 鉴权/413 守卫与指标统计 | `WebContext`, `WebMiddlewareConfig`, `WebMiddlewareOutcome`, `WebMiddlewareStats`, `WebMiddlewarePipeline`, `renderJsonErrorBody` |
| `lib/web/hedged.cp` | Google "The Tail at Scale" 对冲请求（Hedged Requests）与重试预算防风暴控制器 | `HEDGED_MAX_ATTEMPTS`, `RetryBudgetConfig`, `RetryBudgetStats`, `RetryBudget`, `HedgedRequestConfig`, `HedgedAction`, `HedgedAttemptState`, `HedgedWinResult`, `HedgedRequestTracker` |

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

面向 CDN、API 网关与对象存储的 100% 纯 CP 零动态堆分配 RFC 9110 §8.8.3 & §13 条件请求状态机：

- **ETag 解析与强弱比较**：`parseEntityTag`（严格区分强标签 `"abc"` 与弱标签 `W/"abc"`，拒绝小写 `w/`、未闭合引号及非法 `etagc` 字节）、`etagStrongEqual`、`etagWeakEqual`。
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

面向 API 网关与 Web 框架的 100% 纯 CP 零动态堆分配 W3C Fetch CORS 安全裁决与响应头合成器：

- **常量与安全匹配原语**：
  - `CORS_MAX_ORIGINS = 32`、`CORS_MAX_HEADERS = 32`、`CORS_MAX_ORIGIN_LEN = 128`、`CORS_MAX_HEADER_NAME_LEN = 64`、`CORS_METHOD_GET..OPTIONS` / `CORS_METHOD_ALL_STANDARD`。
  - `hasControlOrCrlf(s)`、`isValidOriginSyntax(origin)`、`matchOriginPattern(pattern, origin)`（严格校对协议与点号边界，防止 `https://evil-example.com` 绕过 `https://*.example.com`）、`parseCorsMethodMask(method)`。
- **核心引擎 `CorsGuard` 与响应头合成**：
  - `addAllowedOrigin` / `setAllowedMethods` / `addAllowedHeader` / `addExposedHeader` / `setAllowCredentials` / `setMaxAgeSecs`。
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

面向 CDN、API 网关与 HTTP 客户端缓存的 100% 纯 CP 零动态堆分配 RFC 9111 + RFC 5861 缓存策略与新鲜度裁决引擎：

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

面向现代 HTTP 网关、API 服务端与 CDN 边缘节点的 100% 纯 CP 零动态堆分配 **RFC 9110 §12 Content Negotiation** 引擎：

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

面向 HTTP API 网关、代理与高可靠 HTTP 客户端的 100% 纯 CP 零动态堆分配限流响应头编解码与重试退避规划引擎：

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

面向现代 Web 应用纵深防御的 100% 纯 CP 零动态堆分配、零 C 依赖 CSRF 防护引擎：

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

100% 纯 CP、零动态堆分配、零 C 依赖的预签名 URL 生成与常量时间验签引擎（内联 FIPS 180-4 SHA-256 + RFC 2104 HMAC-SHA256 + 无填充 Base64URL）：

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
