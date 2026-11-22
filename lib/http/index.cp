// Source: Absorbed from /data/dev/bun/src/http/ | Synced: 2026-11-21T16:52:53Z
// lib/http/index.cp
// CP Standard HTTP Library (cap:http) - Industrial-grade unified HTTP subsystem

export { Method } from "./method";
export { Headers, HeaderEntry } from "./headers";
export { ETag, ParsedETag } from "./etag";
export { MimeCategory, MimeEntry, MIME_DATABASE_COUNT, MIME_DATABASE, MimeType } from "./mime";
export { HttpStatusEntry, HTTP_STATUS_TABLE, HTTPStatus } from "./status";
export { MAX_BUILDER_HEADERS, HeaderSpan, HeaderBuilder } from "./header_builder";
export {
    CacheControl,
    DispositionType,
    ContentDisposition,
    AcceptItem,
    AcceptParser,
    ResolvedByteRange,
    ByteRangeSpec,
    MAX_RANGE_SPECS,
    RangeHeader,
    ContentRange
} from "./header_parsers";
export { Request } from "./request";
export { Response } from "./response";
export { HttpServer, fetch, json, serve } from "./server";

// --- Step 1: HTTP Infrastructure Types & Enums ---
export { Encoding, AcceptEncodingItem, ContentEncodingNegotiator } from "./encoding";
export { FetchCacheMode, FetchRedirect, FetchRequestMode } from "./fetch_modes";
export { HTTPCertError, CertificateInfo, InitError } from "./cert";
export { URLPath } from "./url_path";
export { SignalStore, Signals } from "./signals";
export { HeaderValueIterator } from "./header_iterator";
export { HTTPRequestBody, HTTPRequestBodyTag } from "./request_body";
export {
    HTTPVerboseLevel,
    HTTPUpgradeState,
    Protocol,
    ShouldContinue,
    HTTPStage,
    Stage,
    Flags,
    InternalStateFlags,
    InternalState
} from "./internal_state";
export {
    MAX_CHUNK_TRAILERS,
    ChunkedDecoderState,
    ChunkTrailer,
    ChunkTrailerSpan,
    ChunkedDecodeResult,
    ChunkedDecoder,
    ChunkedEncoder
} from "./chunked";
export {
    MAX_REDIRECT_HOPS,
    MAX_REDIRECT_HISTORY,
    RedirectAction,
    ParsedHttpOrigin,
    RedirectDecision,
    RedirectPolicy
} from "./redirect";

// --- Step 2: HTTP/2 & HTTP/3 Subsystems ---
export {
    CLIENT_PREFACE,
    MAX_WINDOW_SIZE,
    MAX_HEADER_TABLE_SIZE,
    MAX_STREAM_ID,
    MAX_FRAME_SIZE,
    DEFAULT_WINDOW_SIZE,
    DEFAULT_MAX_FRAME_SIZE,
    FrameType,
    PingFrameFlags,
    DataFrameFlags,
    HeadersFrameFlags,
    SettingsFlags,
    ErrorCode,
    SettingsType,
    UInt31WithReserved,
    StreamPriority,
    FrameHeader,
    SettingsPayloadUnit,
    RstStreamFrame,
    WindowUpdateFrame,
    PingFrame,
    GoawayFrame,
    H2StreamState,
    H2Stream,
    H2FlowControl,
    H2FrameActionKind,
    H2FrameAction,
    H2ClientSession,
    HpackIntResult,
    HpackStaticEntry,
    HpackDecodedHeader,
    HpackHuffCode,
    HpackHuffBucket,
    HPACK_STATIC_TABLE,
    HPACK_HUFFMAN_TABLE,
    HPACK_HUFF_BUCKETS,
    HPACK_HUFF_SORTED_SYMS,
    HpackCodec,
    HpackIndexMode,
    HpackDynamicEntry,
    HpackDynamicTable,
    HpackSessionCodec,
    H2_CLIENT_PREFACE,
    H2_FRAME_HEADER_LEN,
    H2_DEFAULT_WINDOW_SIZE,
    H2_DEFAULT_MAX_FRAME_SIZE,
    H2_MAX_FRAME_SIZE_LIMIT,
    H2_FRAME_DATA,
    H2_FRAME_HEADERS,
    H2_FRAME_SETTINGS,
    H2_FRAME_WINDOW_UPDATE,
    H2_FRAME_CONTINUATION,
    H2_FLAG_END_STREAM,
    H2_FLAG_END_HEADERS,
    H2_SETTINGS_ENABLE_PUSH,
    H2_SETTINGS_INITIAL_WINDOW_SIZE,
    H2_SETTINGS_MAX_HEADER_LIST_SIZE,
    RequestHeaderAction,
    HeaderBlockEncodeResult,
    DataWindowState,
    DataWriteResult,
    classifyRequestHeader,
    shouldSkipRequestHeader,
    isSensitiveHeader,
    lowercaseHeaderName,
    encodeHpackTableSizeUpdate,
    encodeFrameHeader,
    encodeClientPreface,
    encodeHeaderBlockFrames,
    encodeDataWindowed,
    H2_MAX_WINDOW_SIZE,
    H2_DEFAULT_INITIAL_WINDOW,
    H2_RST_STREAM_FRAME_LEN,
    H2_WINDOW_UPDATE_FRAME_LEN,
    StreamState,
    H2ErrorCode,
    errorCodeToU32,
    encodeRstStreamFrame,
    encodeWindowUpdateFrame,
    H2StreamMachine,
} from "./h2/index";

export {
    AltSvcEntry,
    AltSvc,
    H3StreamType,
    H3Stream,
    H3ClientSession,
    H3EncodeError,
    H3InputHeader,
    H3Header,
    H3RequestMeta,
    H3EncodedRequest,
    encodeH3RequestHeaders,
    QPACK_NO_INDEX,
    QPACK_IDX_AUTHORITY,
    QPACK_IDX_PATH,
    QPACK_IDX_CONTENT_DISPOSITION,
    QPACK_IDX_CONTENT_LENGTH,
    QPACK_IDX_COOKIE,
    QPACK_IDX_DATE,
    QPACK_IDX_ETAG,
    QPACK_IDX_IF_MODIFIED_SINCE,
    QPACK_IDX_IF_NONE_MATCH,
    QPACK_IDX_LAST_MODIFIED,
    QPACK_IDX_LINK,
    QPACK_IDX_LOCATION,
    QPACK_IDX_REFERER,
    QPACK_IDX_SET_COOKIE,
    QPACK_IDX_METHOD_GET,
    QPACK_IDX_SCHEME_HTTPS,
    QPACK_IDX_ACCEPT,
    QPACK_IDX_ACCEPT_ENCODING,
    QPACK_IDX_ACCEPT_RANGES,
    QPACK_IDX_CACHE_CONTROL,
    QPACK_IDX_CONTENT_ENCODING,
    QPACK_IDX_CONTENT_TYPE,
    QPACK_IDX_RANGE,
    QPACK_IDX_VARY,
    QPACK_IDX_ACCEPT_LANGUAGE,
    QPACK_IDX_AUTHORIZATION,
    QPACK_IDX_FORWARDED,
    QPACK_IDX_IF_RANGE,
    QPACK_IDX_ORIGIN,
    QPACK_IDX_SERVER,
    QPACK_IDX_USER_AGENT,
    QPACK_IDX_X_FORWARDED_FOR,
    QpackHeader,
    QpackClassKind,
    QpackClass,
    IntDecodeResult,
    classifyHeader,
    staticIndexToName,
    encodePrefixedInt,
    decodePrefixedInt,
    buildHttp3RequestHeaders,
    encodeFieldSection,
    decodeFieldSection,
} from "./h3/index";

// --- Step 3: Industrial Client Engine & Lifecycle ---
export {
    DecompressorType,
    Decompressor,
    StreamingDecompressor
} from "./decompressor";

export {
    ProxyTunnelSocketType,
    ProxyConnectResponse,
    NoProxyMatcher,
    ProxyTunnel,
    HttpProxyTunnel
} from "./proxy";

export {
    SendFileStatus,
    SendFile
} from "./sendfile";

export {
    MAX_POOLED_SOCKETS,
    PooledSocket,
    HTTPContext,
    PoolEntryState,
    PooledConnection,
    PoolStats,
    HttpConnectionPool
} from "./context";

export {
    HTTPClientResult,
    HTTPClient
} from "./client";

export {
    MAX_WIRE_HEADERS,
    HttpRequestBuilder,
    HttpResponseStage,
    ParsedHttpResponse,
    HttpResponseParser
} from "./wire_http";

// --- Step 4: Low-level PicoHTTPParser C-FFI & Zero-Copy Wire Parser ---
export {
    phr_header,
    ChunkedEncodingState,
    phr_chunked_decoder,
    phr_parse_request,
    phr_parse_response,
    phr_parse_headers,
    phr_decode_chunked,
    phr_decode_chunked_is_in_data,
    PicoHeader,
    PicoRequest,
    PicoResponse,
    PicoHttpParser
} from "./_picohttp";

// --- Step 5: RFC 6265 CookieMap & RFC 7578 FormData ---
export * from "./cookie.cp";
export * from "./form_data.cp";

// --- Step 6: RFC Web Primitives (SSE, Multipart FormData, RFC 6265 CookieJar) ---
export * from "./sse.cp";
export * from "./multipart.cp";

// --- Step 7: Consolidated RFC 9110/9111/7838/W3C HTTP Header Codecs (Wave 3 Batch 1) ---
export {
    parseQValue,
    MediaRange,
    AcceptIter,
    EncodingPref,
    AcceptEncodingIter as AcceptEncPrefIter,
    LangPref,
    AcceptLanguageIter as AcceptLangPrefIter,
    negotiateMediaType,
    negotiateEncoding as negotiateEncPref,
    negotiateLanguage as negotiateLangPref,
} from "./accept.cp";

export {
    EncodingKind,
    EncodingItem,
    AcceptEncodingIter,
    negotiateEncoding,
    isEncodingAcceptable,
} from "./acceptenc.cp";

export {
    LanguageRange,
    AcceptLanguageIter,
    negotiateLanguage,
} from "./acceptlang.cp";

export {
    AcceptRangesKind,
    AcceptRanges,
    AcceptRangesIter,
    parseAcceptRanges,
    supportsByteRanges,
    supportsRangeUnit,
} from "./acceptranges.cp";

export {
    HttpMethodBit,
    AllowIter,
    AllowMethodSet,
    parseAllowHeader,
    isMethodAllowed,
    isSafeMethod,
    isIdempotentMethod,
} from "./allow.cp";

export {
    AltSvcEntry as AltSvcHeaderEntry,
    isAltSvcClear,
    AltSvcIter,
    formatAltSvcEntry,
} from "./altsvc.cp";

export {
    AuthScheme,
    AuthHeader,
    BasicCredentials,
    decodeBasicAuth,
    encodeBasicAuth,
    encodeBearerAuth,
} from "./auth.cp";

export {
    HttpCacheError,
    FreshnessStatus,
    CacheHeader,
    CacheControl as CachePolicyControl,
    EntityTag,
    HttpCacheEvaluator,
} from "./cache.cp";

export {
    CacheDirective,
    CacheControlIter,
    CacheControl as ParsedCacheControl,
} from "./cachectrl.cp";

export {
    isTrackingQueryParam,
    normalizeQueryWithoutTracking,
    buildCacheKey,
    appendVaryDimension,
} from "./cachekey.cp";

export {
    ClearSiteData,
} from "./clearsite.cp";

export {
    BrandVersion,
    BrandListIter,
    findBrandVersion,
    parseMobileHint,
    parsePlatformHint,
} from "./clienthints.cp";

export {
    ContentCoding,
    parseContentCoding,
    ContentEncodingIter,
    encodingLayerCount,
    isCompressed,
    hasContentEncoding,
    isValidContentEncoding,
} from "./contentenc.cp";

export {
    LanguageTagParts,
    isValidBcp47Tag,
    parseLanguageTagParts,
    ContentLanguageIter,
    canonicalizeLanguageTag,
    hasContentLanguage,
} from "./contentlang.cp";

export {
    ContentRangeKind,
    ContentRange as ParsedContentRange,
    parseContentRange,
    formatContentRange,
} from "./contentrange.cp";

// --- Step 8: Consolidated RFC/W3C HTTP Header Codecs (Wave 4 Batch 2) ---
export {
    ContentTypeParam,
    ContentTypeParamIter,
    ContentType,
} from "./contenttype.cp";

export {
    SameSite as CookieJarSameSite,
    Cookie,
    CookiePair as CookieJarPair,
    CookieRequestIterator,
    getCookie,
} from "./cookiejar.cp";

export {
    defaultCookiePath,
    cookieDomainMatches,
    cookiePathMatches,
    CookiePrefixKind,
    detectCookiePrefix,
    validateCookiePrefix,
    canSendCookie,
} from "./cookiematch.cp";

export {
    CspDirective,
    CspSourceIter,
    CspDirectiveIter,
    findDirective,
    effectiveFetchDirective,
    appendCspDirective,
} from "./csp.cp";

export {
    HttpDateTime,
    formatHttpDate,
    parseHttpDate,
} from "./date.cp";

export {
    DigestEntry,
    DigestIter,
    findDigest,
    formatDigest,
    WantDigestPref,
    WantDigestIter,
    negotiateDigestAlg,
} from "./digest.cp";

export {
    DigestChallenge,
    DigestCredentials,
    formatDigestHeader,
} from "./digestauth.cp";

export {
    EarlyHintRel,
    EarlyHintLink,
    EarlyHintIter,
    formatEarlyHintLink,
    formatEarlyHints103Block,
} from "./earlyhints.cp";

export {
    fnv1a64,
    generateWeakStatEtag,
    generateStrongHashEtag,
    generateWeakHashEtag,
    isValidEtagSyntax,
} from "./etaggen.cp";

export {
    ExpectItem,
    ExpectIter,
    ExpectDecision,
    hasContinueExpectation,
    hasUnknownExpectation,
    evaluateExpectHeader,
} from "./expect.cp";

export {
    ENC_MASK_IDENTITY,
    ENC_MASK_GZIP,
    ENC_MASK_DEFLATE,
    ENC_MASK_BR,
    ENC_MASK_ZSTD,
    ENC_MASK_ALL,
    ContentEncoding,
    negotiateEncoding as negotiateEncodingMask,
    FetchCacheMode as MetaFetchCacheMode,
    FetchRedirect as MetaFetchRedirect,
    FetchRequestMode as MetaFetchRequestMode,
    UrlPath,
} from "./fetchmeta.cp";

export {
    encodeFormComponent,
    decodeFormComponent,
    FormPairRaw,
    FormIter,
    getFormParam,
    appendFormParam,
} from "./form.cp";

export {
    ForwardedHop,
    ForwardedIterator,
    parseFirstHop,
    extractClientIpFromXff,
    stripPortFromForwardedFor,
} from "./forwarded.cp";

export {
    CacheFreshness,
    FreshnessInput,
} from "./freshness.cp";

export {
    HostHeader,
} from "./host.cp";

// --- Step 9: Consolidated RFC/W3C HTTP Header Codecs & Router (Wave 5 Batch 3) ---
export {
    EntityTag as ConditionalEntityTag,
    etagListMatches,
    PreconditionResult,
    ConditionalRequest,
    evaluatePreconditions,
} from "./ifmatch.cp";

export {
    KeepAliveParams,
    shouldKeepAlive,
} from "./keepalive.cp";

export {
    WebLink,
    LinkHeaderIterator,
    findLinkByRel,
    formatWebLink,
} from "./link.cp";

export {
    MaxForwardsAction,
    MaxForwardsResult,
    parseMaxForwards,
    evaluateMaxForwards,
    formatMaxForwards,
} from "./maxforwards.cp";

export {
    HttpMethod,
    MethodSet,
} from "./methodparse.cp";

export {
    Origin,
    isSameOriginUrl,
} from "./origin.cp";

export {
    PermissionDirective,
    PermissionsPolicyIter,
    findPermission,
} from "./permissions.cp";

export {
    Preference,
    PreferIter,
    ReturnPref,
    HandlingPref,
    PreferHeader,
} from "./prefer.cp";

export {
    ProxyScheme,
    parseProxyScheme,
    defaultProxyPort,
    ProxyUrl,
    ProxyEnv,
    selectProxy,
} from "./proxyurl.cp";

export {
    RangeSpecKind,
    ByteRangeSpec as ParsedByteRangeSpec,
    ResolvedByteRange as ResolvedRangeSlice,
    ByteRangeIter,
    resolveSingleByteRange,
} from "./range.cp";

export {
    RateLimitHeader,
    RateLimitPolicy,
} from "./ratelimit.cp";

export {
    RetryAfterKind,
    RetryAfter,
    formatRetryAfterSeconds,
} from "./retryafter.cp";

export {
    MAX_PARAMS,
    RouterError,
    RouterMethod,
    PathParam,
    RouteMatch,
    AllowInfo,
    RouteResult,
    RouteGroup,
    HttpRouter,
} from "./router.cp";

// --- Step 10: Consolidated Final HTTP Modules (Wave 6 Batch 4) ---
export {
    FetchSite,
    FetchMode,
    FetchDest,
    parseFetchSite,
    parseFetchMode,
    parseFetchDest,
    FetchMetadata,
} from "./secfetch.cp";

export {
    HstsPolicy,
    ReferrerPolicy,
    parseReferrerPolicy,
    FrameOptions,
    parseFrameOptions,
    CoopPolicy,
    parseCoopPolicy,
    CoepPolicy,
    parseCoepPolicy,
    CorpPolicy,
    parseCorpPolicy,
    isCrossOriginIsolated,
} from "./security.cp";

export {
    ServerTimingMetric,
    ServerTimingIter,
    findServerTiming,
    appendServerTiming,
} from "./servertiming.cp";

export {
    SignatureInput,
    SignatureInputIter,
    ComponentIter,
    findSignatureInput,
    findSignatureValue,
} from "./siginput.cp";

export {
    StatusClass,
    statusLine,
    statusText,
    statusTextOrEmpty,
    classify,
    isInformational,
    isSuccess,
    isRedirect,
    isClientError,
    isServerError,
    isError,
    canHaveBody,
    isCacheableByDefault,
    isRetryable,
} from "./statusparse.cp";

export {
    TransferCoding,
    TransferEncodingValidation,
    TransferEncodingIter,
    validateTransferEncoding,
    isChunkedTransfer,
    hasConflictingClAndTe,
    teAcceptsTrailers,
} from "./te.cp";

export {
    isForbiddenTrailerField,
    isValidTrailerFieldName,
    TrailerFieldIter,
    validateTrailerHeader,
    hasTrailerField,
} from "./trailer.cp";

export {
    HttpEncoding,
    OptEncoding,
    HttpFetchCacheMode,
    OptFetchCacheMode,
    HttpFetchRedirect,
    OptFetchRedirect,
    HttpFetchRequestMode,
    OptFetchRequestMode,
    HttpTypesMethod,
    OptMethod,
    HttpParsedETag,
    parseETag,
    computeETag,
    ifNoneMatch,
    HttpURLPath,
} from "./types.cp";

export {
    UpgradeProtocol,
    UpgradeIter,
    ConnectionTokenIter,
    connectionHasToken,
    upgradeHasProtocol,
    isHopByHopHeader,
    isWebSocketUpgrade,
} from "./upgrade.cp";

export {
    ProductToken,
    UserAgentIter,
    BrowserFamily,
    OsFamily,
    isBotUserAgent,
    isMobileUserAgent,
    detectBrowser,
    detectOs,
    findProductVersion,
} from "./useragent.cp";

export {
    VaryIter,
    isVaryWildcard,
    varyContains,
    appendVary,
    mergeVary,
} from "./vary.cp";

export {
    ViaEntry,
    ViaIter,
    viaHopCount,
    viaContainsProxy,
    appendVia,
    parseMaxForwards as parseViaMaxForwards,
    decrementMaxForwards,
} from "./via.cp";

export {
    WarningItem,
    WarningIter,
    isStaleWarning,
    formatWarning,
} from "./warning.cp";

export {
    ByteRange,
    RangeStatus,
    RangeResult,
    RawRangeKind,
    RawRange,
    MultiRangeResult,
    ParsedContentRange as ByteRangeContentRange,
    parseRaw,
    parseRange,
    parseMultiRange,
    formatContentRange as formatByteContentRange,
    formatUnsatisfiableContentRange,
    parseContentRange as parseByteContentRange,
} from "./byterange.cp";
