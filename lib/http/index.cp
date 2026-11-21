// Source: /data/dev/bun/src/http/
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
    HpackSessionCodec
} from "./h2/index";

export {
    AltSvcEntry,
    AltSvc,
    H3StreamType,
    H3Stream,
    H3ClientSession
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
