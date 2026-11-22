// Source: Absorbed from /data/dev/bun/src/http/h3_client/ | Synced: 2026-11-22T11:12:13Z
// lib/http/h3/index.cp
// HTTP/3 protocol client subsystem (QUIC)

export {
    AltSvcEntry,
    AltSvc
} from "./alt_svc";

export {
    H3StreamType,
    H3Stream,
    H3ClientSession
} from "./client";

export {
    H3EncodeError,
    H3InputHeader,
    H3Header,
    H3RequestMeta,
    H3EncodedRequest,
    encodeH3RequestHeaders,
} from "./encode";

export {
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
} from "./qpack";

export {
    H3_POOL_MAX_SESSIONS,
    H3_SESSION_MAX_STREAMS,
    H3PoolError,
    H3ConnectOutcomeKind,
    H3ConnectOutcome,
    H3PendingRequest,
    H3PooledSession,
    H3SessionPool,
} from "./session_pool";

export {
    H3_DISPATCH_MAX_STREAMS,
    H3_DISPATCH_MAX_HEADERS,
    H3_DISPATCH_NO_STREAM,
    H3DispatchError,
    H3ConnCloseReason,
    H3HeaderEventKind,
    H3RawHeader,
    H3StreamSlot,
    H3HeaderDispatchResult,
    H3ConnCloseSummary,
    H3CallbackDispatcher,
} from "./stream_dispatch";

export {
    H3_DNS_PENDING_MAX_CONNECTS,
    H3_DNS_MAX_HOST_LEN,
    H3_DNS_DEFAULT_TTL_SECS,
    H3_DNS_MAX_TTL_SECS,
    H3PendingConnectError,
    H3DnsConnectState,
    H3DnsDrainAction,
    H3SessionFailReason,
    H3RegisterResult,
    H3DrainOutcome,
    H3DnsPendingStats,
    parseH3DnsTtlSeconds,
    H3PendingDnsEntry,
    H3PendingConnectQueue,
} from "./pending_connect";
