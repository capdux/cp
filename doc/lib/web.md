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
