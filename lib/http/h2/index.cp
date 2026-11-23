// Source: Absorbed from /data/dev/bun/src/http/h2_client/ | Synced: 2026-11-22T11:12:13Z
// lib/http/h2/index.cp
// HTTP/2 protocol client subsystem

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
    GoawayFrame
} from "./frame";

export {
    H2StreamState,
    H2Stream,
    H2FlowControl,
    H2FrameActionKind,
    H2FrameAction,
    H2ClientSession
} from "./client";

export {
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
} from "./hpack";

export {
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
} from "./encode";

export {
    H2_MAX_WINDOW_SIZE,
    H2_DEFAULT_INITIAL_WINDOW,
    H2_RST_STREAM_FRAME_LEN,
    H2_WINDOW_UPDATE_FRAME_LEN,
    StreamState,
    H2ErrorCode,
    errorCodeToU32,
    encodeRstStreamFrame,
    encodeWindowUpdateFrame,
    H2Stream as H2StreamMachine,
} from "./stream";

export {
    H2_PENDING_MAX_CONNECTS,
    H2_PENDING_MAX_WAITERS,
    H2_PENDING_MAX_HOST_LEN,
    H2PendingConnectError,
    H2ConnectJoinKind,
    H2ConnectJoinResult,
    H2PendingConnectEntry,
    H2PendingConnectTable,
} from "./pendingConn";

export {
    H2_DISPATCH_HEADER_SIZE,
    H2_DISPATCH_DEFAULT_MAX_FRAME_SIZE,
    H2_DISPATCH_MAX_FRAME_SIZE_LIMIT,
    H2_DISPATCH_DEFAULT_WINDOW_SIZE,
    H2_DISPATCH_MAX_WINDOW_SIZE,
    H2_DISPATCH_DEFAULT_MAX_HEADER_LIST,
    H2DispatchError,
    H2DispatchEventKind,
    H2ParsedFrameHeader,
    stripH2Padding,
    H2DispatchEvent,
    H2FrameDispatcher,
} from "./dispatch";

export {
    H2_LOCAL_INITIAL_WINDOW_SIZE,
    H2_LOCAL_MAX_HEADER_LIST_SIZE,
    H2_WRITE_BUFFER_HIGH_WATER,
    H2_WRITE_BUFFER_CONTROL_LIMIT,
    H2BackpressureError,
    H2WriteAdmission,
    H2WriteBufferBudget,
    H2ClientMetrics,
} from "./backpressure";

// --- Consolidated HTTP/2 Wire Frame Header & Full RFC 9113 Frame Codec ---
export {
    FRAME_HEADER_SIZE,
    SETTINGS_UNIT_SIZE,
    STREAM_PRIORITY_SIZE,
    H2FrameError,
    FLAG_ACK,
    FLAG_END_STREAM,
    FLAG_END_HEADERS,
    FLAG_PADDED,
    FLAG_PRIORITY,
    SettingsUnit,
    hasClientPreface,
    FrameType as H2WireFrameType,
    ErrorCode as H2WireErrorCode,
    SettingsType as H2WireSettingsType,
    UInt31WithReserved as H2WireUInt31,
    FrameHeader as H2WireFrameHeader,
    StreamPriority as H2WireStreamPriority,
} from "./frameWire.cp";

export {
    H2_PREFACE,
    H2_HEADER_LEN,
    H2_DEFAULT_INITIAL_WINDOW_SIZE,
    H2_MAX_WINDOW_SIZE as H2_FRAME_MAX_WINDOW_SIZE,
    H2_FLAG_ACK,
    H2_FLAG_PADDED,
    H2_FLAG_PRIORITY,
    H2Error,
    H2FrameType,
    H2ErrorCode as H2FrameErrorCode,
    H2FrameHeader,
    H2Setting,
    H2PrioritySpec,
    H2DataFrame,
    H2HeadersFrame,
    H2RstStreamFrame,
    H2SettingsFrame,
    H2PingFrame,
    H2GoAwayFrame,
    H2WindowUpdateFrame,
    H2RawFrame,
    H2Frame,
} from "./frameTypes.cp";

export {
    validatePreface,
    freeDecodedFrame,
    decodeFrame,
    encodeFrame,
} from "./frameCodec.cp";

export {
    H2DecodedFrame,
    H2StreamDecoder,
} from "./frameDecoder.cp";

export {
    H2StreamState as H2FrameStreamState,
    H2StreamInfo,
    H2ConnectionWindow,
} from "./frameWindow.cp";
