// Origin: Pure-CP Stdlib (Unified Web, API, RPC, HTTP Protocol & Browser Query Suite: cap:web) | Created: 2026-11-22
// ============================================================================
// Pure-CP Zero-C-Dependency Web/API Specification, RPC, HTTP/1.1/2/3 Framing,
// SSE, Multipart, Feeds, Sitemaps, Selectors, XPath, Webhooks & Resilience.
// ============================================================================

// --- API Specification, RPC & Webhooks ---
export {
    OpenApiError,
    HttpMethod,
    ParamIn,
    SecuritySchemeType,
    OpenApiInfo,
    OpenApiServer,
    OpenApiTag,
    OpenApiParam,
    OpenApiResponse,
    OpenApiSecurityScheme,
    OpenApiOperation,
    OpenApiComponentSchema,
    OpenApiEndpoint,
    OpenApiDoc,
} from "./openapiSpec.cp";

export {
    KeyValuePair,
    MatchStatus,
    RouteMatch,
    RouteMatchResult,
    ValidationErrorItem,
    ValidationReport,
    OpenApiValidator,
} from "./openapiRoute.cp";

export {
    MAX_RPC_DEPTH,
    DEFAULT_MAX_MSG_BYTES,
    RpcError,
    RpcMapEntry,
    RpcValue,
    RpcRequest,
    RpcResponse,
    RpcNotification,
    RpcMessage,
    DecodedRpcMessage,
    RpcStreamDecoder,
    encodeRpcValue,
    encodeRpcMessage,
    decodeRpcMessage,
} from "./msgpackRpc.cp";

export {
    WebhookError,
    StandardWebhook,
    GitHubWebhook,
    StripeWebhook,
} from "./webhookSign.cp";

export {
    IdempotencyStore,
} from "./webhookIdem.cp";

// --- HTTP/1.1, HTTP/2, HTTP/3, SSE, Multipart & URI Templates ---
export {
    MAX_HEADER_LINE_LEN,
    MAX_HEADERS_BYTES,
    PicoError,
    HttpHeader,
    ParsedRequest,
    ParsedResponse,
    ParsedHeaders,
    ChunkedState,
    ChunkDecodeResult,
    ChunkedDecoder,
    parseRequest,
    parseResponse,
    parseHeaders,
    findHeader,
    parseContentLength,
} from "./picohttp.cp";

export {
    CONNECTION_PREFACE,
    FRAME_HEADER_SIZE,
    DEFAULT_MAX_FRAME_SIZE,
    MAX_ALLOWED_FRAME_SIZE,
    DEFAULT_INITIAL_WINDOW_SIZE,
    MAX_WINDOW_SIZE,
    Http2Error,
    FrameType,
    FrameFlags,
    Http2ErrorCode,
    SettingId,
    SettingParam,
    GoawayPayload,
    StreamState,
    Http2Stream,
    Http2Frame,
    FrameStreamDecoder,
    FlowWindow,
    hasConnectionPreface,
    encodeFrame,
    encodeSettings,
    decodeSettings,
    encodePing,
    decodePing,
    encodeGoaway,
    decodeGoaway,
    encodeWindowUpdate,
    decodeWindowUpdate,
    encodeRstStream,
    decodeRstStream,
    encodeDataFrame,
    decodeDataPayload,
} from "./http2.cp";

export {
    MAX_VARINT,
    DEFAULT_MAX_FRAME_SIZE as H3_DEFAULT_MAX_FRAME_SIZE,
    QPACK_MAX_TABLE_CAPACITY,
    MAX_FIELD_SECTION_SIZE,
    QPACK_BLOCKED_STREAMS,
    Http3Error,
    H3StreamType,
    H3FrameType,
    H3Setting,
    H3PushPromiseFrame,
    H3UnknownFrame,
    H3Frame,
    H3Header,
    QPACK_STATIC_TABLE,
    parseStreamType,
    encodeFrame as encodeH3Frame,
    decodeFrames,
    decodeFramesWithMaxSize,
    freeFrames,
    encodeQpackHeaders,
    decodeQpackHeaders,
} from "./http3.cp";

export {
    SseError,
    SseEvent,
    SseParser,
    SseEncoder,
} from "./sse.cp";

export {
    DEFAULT_MAX_PARTS,
    DEFAULT_MAX_PART_BYTES,
    MultipartError,
    MultipartPart,
    MultipartDocument,
    MultipartBuilder,
    extractBoundary,
    parse,
    parseWithLimits,
} from "./multipart.cp";

export {
    UriTemplateError,
    TemplateOp,
    AssocPair,
    TemplateVars,
    UriTemplate,
} from "./uritemplate.cp";

// --- Syndication, Crawling & Browser/DOM Query ---
export {
    SitemapError,
    ChangeFreq,
    SitemapKind,
    SitemapUrl,
    SitemapIndexItem,
    RobotsRules,
    SitemapCodec,
} from "./sitemap.cp";

export {
    FeedError,
    FeedFormat,
    FeedItem,
    Feed,
    FeedCodec,
} from "./feed.cp";

export {
    OpmlHead,
    OpmlOutline,
    isOpmlDocument,
    parseOpmlHead,
    OpmlOutlineIter,
    formatOpmlFeedOutline,
} from "./opml.cp";

export {
    SelectorError,
    AttrOp,
    Combinator,
    NodePtr,
    AttrSelector,
    PseudoKind,
    PseudoSelector,
    CompoundSelector,
    ComplexPart,
    ComplexSelector,
    CssSelector,
} from "./selector.cp";

export {
    XPathTokenKind,
    XPathToken,
    XPathIter,
    XPathStep,
    XPathStepIter,
    validateXPathDelimiters,
} from "./xpath.cp";

// --- API Gateway Resilience & Traffic Control ---
export {
    RateLimitError,
    RateDecision,
    TokenBucket,
    SlidingWindowCounter,
    GcraLimiter,
    KeyedRateLimiter,
} from "./ratelimit.cp";

export {
    CircuitError,
    CircuitState,
    CircuitConfig,
    CircuitStats,
    CircuitBreaker,
} from "./breaker.cp";

export {
    JitterMode,
    BackoffPolicy,
    Bulkhead,
} from "./breakerPol.cp";

export {
    WebContext,
    WebMiddlewareConfig,
    WebMiddlewareOutcome,
    WebMiddlewareStats,
    WebMiddlewarePipeline,
    renderJsonErrorBody,
} from "./middleware.cp";

export {
    HEDGED_MAX_ATTEMPTS,
    RetryBudgetConfig,
    RetryBudgetStats,
    RetryBudget,
    HedgedRequestConfig,
    HedgedAction,
    HedgedAttemptState,
    HedgedWinResult,
    HedgedRequestTracker,
} from "./hedged.cp";

// --- Zero-Alloc SSE Fanout Broker & Last-Event-ID Resumption Ring ---
export {
    SSE_BROKER_MAX_HISTORY,
    SSE_BROKER_MAX_CLIENTS,
    SSE_EVENT_TYPE_MAX_LEN,
    SSE_DATA_MAX_LEN,
    SseStoredEvent,
    SseClientCursor,
    SsePollStatus,
    SsePollResult,
    SseBrokerStats,
    formatSseEventWire,
    SseFanoutBroker,
} from "./sseBroker.cp";

// --- Zero-Alloc Stripe-Style API Idempotency-Key Lock & Response Replay Store ---
export {
    IDEM_STORE_MAX_ENTRIES,
    IDEM_KEY_MAX_LEN,
    IDEM_BODY_MAX_LEN,
    IdemEntryState,
    IdemAcquireKind,
    IdemStoreConfig,
    IdemRecord,
    IdemAcquireDecision,
    IdemStoreStats,
    ApiIdempotencyStore,
} from "./idempotency.cp";

// --- Zero-Alloc RFC 9110 §13 Conditional Request & ETag Precondition Evaluator ---
export {
    PreconditionMethod,
    PreconditionVerdict,
    PreconditionTrigger,
    EntityTag,
    PreconditionRequest,
    ResourceValidators,
    PreconditionResult,
    parseEntityTag,
    etagStrongEqual,
    etagWeakEqual,
    etagListMatchesStrong,
    etagListMatchesWeak,
    validateEtagList,
    evaluatePreconditions,
} from "./etagPrecond.cp";

// --- Zero-Alloc W3C Fetch CORS Guard, Safe Subdomain Matcher & Header Synthesizer ---
export {
    CORS_MAX_ORIGINS,
    CORS_MAX_HEADERS,
    CORS_MAX_ORIGIN_LEN,
    CORS_MAX_HEADER_NAME_LEN,
    CORS_HEADER_LIST_BUF_LEN,
    CORS_METHOD_GET,
    CORS_METHOD_HEAD,
    CORS_METHOD_POST,
    CORS_METHOD_PUT,
    CORS_METHOD_DELETE,
    CORS_METHOD_PATCH,
    CORS_METHOD_OPTIONS,
    CORS_METHOD_ALL_STANDARD,
    hasControlOrCrlf,
    isValidOriginSyntax,
    matchOriginPattern,
    parseCorsMethodMask,
    CorsDecisionKind,
    CorsDecision,
    CorsGuard,
    formatCorsResponseHeaders,
} from "./corsGuard.cp";

// --- Zero-Alloc RFC 9111 HTTP Cache Freshness, Cache-Control Parser & SWR/SIE Evaluator ---
export {
    CACHE_MAX_DELTA_SECONDS,
    CacheScope,
    CacheLookupVerdict,
    CacheControl,
    StoredResponseMeta,
    CacheLookupRequest,
    CacheLookupDecision,
    parseCacheControl,
    isResponseStorable,
    computeFreshnessLifetime,
    computeCurrentAge,
    evaluateCacheLookup,
} from "./cachePolicy.cp";
