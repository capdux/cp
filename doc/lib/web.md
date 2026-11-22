# CP 标准库：Web、API、RPC、HTTP 协议与浏览器查询套件手册 (`cap:web`)

> 来源：Pure-CP 零 C 依赖 Web/API/RPC/协议与服务端治理标准库收敛（16 个子模块统一聚合） | 创建日期：2026-11-22

## 一、 架构总览

`cap:web` 是 CP 语言官方内置的纯原生、零外部 C 依赖 Web 与 API 生态标准库，统一覆盖 **OpenAPI 3.1 规范校验、MessagePack-RPC、HTTP/1.1/2/3 协议解析与二进制帧编解码、SSE/Multipart/URI Template、RSS/Atom/OPML/Sitemap、CSS3 选择器与 XPath、Webhook 签名幂等与限流熔断治理**。

---

## 二、 模块文件拓扑与核心 API 索引

### 1. API 规范、RPC 与 Webhook（3 个子模块）

| 子模块文件 | 领域分类 | 核心导出符号 |
| :--- | :--- | :--- |
| `lib/web/openapi_spec.cp` / `openapi_router.cp` | OpenAPI 3.1.0 AST、路由匹配与契约校验 | `OpenApiDoc`, `OpenApiValidator`, `OpenApiEndpoint`, `OpenApiOperation`, `RouteMatchResult`, `ValidationReport`, `HttpMethod` |
| `lib/web/msgpack_rpc.cp` | MessagePack-RPC 编解码与流式帧解码 | `encodeRpcMessage`, `decodeRpcMessage`, `encodeRpcValue`, `RpcStreamDecoder`, `RpcRequest`, `RpcResponse`, `RpcNotification`, `RpcValue` |
| `lib/web/webhook_sign.cp` / `webhook_idempotency.cp` | Webhook HMAC 验签 (Standard/GitHub/Stripe) 与幂等存储 | `StandardWebhook`, `GitHubWebhook`, `StripeWebhook`, `IdempotencyStore`, `WebhookError` |

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
| `lib/web/circuit_breaker.cp` / `circuit_policy.cp` | 熔断器、指数退避抖动与舱壁隔离 | `CircuitBreaker`, `CircuitConfig`, `CircuitState`, `CircuitStats`, `BackoffPolicy`, `Bulkhead`, `JitterMode` |
| `lib/web/middleware.cp` | 零堆分配 HTTP 洋葱中间件链、CORS/Bearer 鉴权/413 守卫与指标统计 | `WebContext`, `WebMiddlewareConfig`, `WebMiddlewareOutcome`, `WebMiddlewareStats`, `WebMiddlewarePipeline`, `renderJsonErrorBody` |

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
