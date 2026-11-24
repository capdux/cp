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
    HTTP_MAX_HEADERS,
    PicoError,
    FramingInfo,
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
    validateHttp1Framing,
    parseChunkSizeStrict,
    simdScanTargetBytes,
    simdScanHeaderNameBytes,
    simdScanHeaderValueBytes,
    simdFindCrlf,
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
    scalarWebHasControlOrCrlf,
    simdWebHasControlOrCrlf,
    scalarWebBytesEqual,
    simdWebBytesEqual,
    scalarWebAsciiLowerEql,
    simdWebAsciiLowerEql,
    hasControlOrCrlf,
    isValidOriginSyntax,
    isValidCorsHeaderToken,
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

// --- Zero-Alloc RFC 9110 §12 HTTP Content Negotiation & Vary Header Engine ---
export {
    NEG_Q_MAX,
    NEG_MAX_ITEMS,
    NEG_VARY_MAX_HEADERS,
    parseQValue,
    ContentEncoding,
    NegotiationMatchKind,
    MediaMatchResult,
    EncodingMatchResult,
    LanguageMatchResult,
    negotiateMediaType,
    negotiateEncoding,
    negotiateLanguage,
    formatVaryHeader,
} from "./contentNeg.cp";

// --- Zero-Alloc IETF RateLimit / RateLimit-Policy / Retry-After Codec & Retry Planner ---
export {
    RATE_HDR_MAX_NAME_LEN,
    RATE_HDR_MAX_PK_LEN,
    RateLimitHeader,
    RateLimitPolicyHeader,
    RetryAfterKind,
    RetryAfterValue,
    RetryDelaySource,
    RateLimitHeaderInfo,
    RetryPlan,
    parseRateLimitHeader,
    formatRateLimitHeader,
    parseRateLimitPolicyHeader,
    formatRateLimitPolicyHeader,
    parseRetryAfter,
    formatRetryAfterDelta,
    formatRetryAfterHttpDate,
    parseLegacyRateLimitHeaders,
    formatLegacyRateLimitHeaders,
    planRateLimitRetry,
} from "./rateHeader.cp";

// --- Zero-Alloc Stateless HMAC-SHA256 CSRF Token & Sec-Fetch-Site/Origin Guard ---
export {
    CSRF_NONCE_LEN,
    CSRF_TOKEN_LEN,
    CSRF_MAX_TRUSTED_ORIGINS,
    CSRF_MAX_ORIGIN_LEN,
    CSRF_DEFAULT_TTL_S,
    CsrfTokenStatus,
    CsrfVerdict,
    CsrfRequest,
    CsrfDecision,
    CsrfGuardStats,
    csrfConstantTimeEq,
    mintCsrfToken,
    verifyCsrfToken,
    CsrfGuard,
} from "./csrfGuard.cp";

// --- Zero-Alloc HMAC-SHA256 Pre-Signed URL & Multi-Key Rotation Verifier ---
export {
    SIGNED_URL_SIG_LEN,
    SIGNED_URL_MAX_KEYS,
    SIGNED_URL_MAX_KID_LEN,
    SIGNED_URL_MAX_SECRET_LEN,
    SIGNED_URL_DEFAULT_CLOCK_SKEW_S,
    SignedUrlStatus,
    SignedUrlVerifyResult,
    SignedUrlKeyringStats,
    signedUrlConstantTimeEq,
    formatSignedUrlStatus,
    signUrl,
    verifySignedUrl,
    SignedUrlKeyring,
} from "./signedUrl.cp";

// --- Zero-Alloc W3C Content-Security-Policy (CSP Level 3) Parser, Evaluator & Builder ---
export {
    CSP_MAX_DIRECTIVES,
    CSP_MAX_SOURCES_PER_DIR,
    CSP_MAX_TOKEN_LEN,
    CspDirectiveKind,
    CspSourceKind,
    CspEvalVerdict,
    CspSourceExpr,
    CspDirective,
    CspPolicy,
} from "./cspPolicy.cp";

// --- Zero-Alloc RFC 6797 HSTS Parser, Domain Store & HTTPS Upgrade Engine ---
export {
    HSTS_MAX_DOMAINS,
    HSTS_HOST_MAX_LEN,
    HSTS_PRELOAD_MIN_MAX_AGE,
    HstsError,
    HstsPolicy,
    HstsEntry,
    HstsLookupResult,
    HstsStoreStats,
    parseHstsHeader,
    formatHstsHeader,
    isHstsEligibleHost,
    HstsStore,
} from "./hstsStore.cp";

// === W29-Sync R5: Consolidated Web Route Params & File-System Router ===
export {
    Param,
    ParamMap,
    URLPath,
    parseURLPath,
} from "./routeParams.cp";

export {
    RouteKind,
    SegmentKind,
    RouteSegment,
    RouteParam,
    RouteMatch as FsRouteMatch,
    RoutePattern,
    FsRouter,
} from "./fsRouter.cp";
