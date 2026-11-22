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
} from "./openapi_spec.cp";

export {
    KeyValuePair,
    MatchStatus,
    RouteMatch,
    RouteMatchResult,
    ValidationErrorItem,
    ValidationReport,
    OpenApiValidator,
} from "./openapi_router.cp";

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
} from "./msgpack_rpc.cp";

export {
    WebhookError,
    StandardWebhook,
    GitHubWebhook,
    StripeWebhook,
} from "./webhook_sign.cp";

export {
    IdempotencyStore,
} from "./webhook_idempotency.cp";

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
} from "./circuit_breaker.cp";

export {
    JitterMode,
    BackoffPolicy,
    Bulkhead,
} from "./circuit_policy.cp";
