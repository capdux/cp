// Source: Absorbed from /data/dev/bun/src/http/h3_client/callbacks.cp | Synced: 2026-11-22T19:50:28Z
// Ported from Bun: /data/dev/bun/src/http/h3_client/callbacks.cp & /data/dev/bun/docs/guides/http/fetch.mdx
// Synced: 2026-11-22
// Pure-CP zero-allocation HTTP/3 QUIC Client Callback Event Dispatcher:
// - Models all lsquic/QUIC client lifecycle callbacks (onHandshakeDone / onGoaway / onConnClose / onStreamOpen / onStreamHeaders / onStreamData / onStreamClose)
// - Filters HTTP/3 pseudo-headers (`:*`), parses `:status`, ignores 1xx informational responses and post-status trailers, and flags missing `:status` protocol errors
// - Distinguishes pre-handshake failures (`HandshakeFailed`) from mid-session drops (`ConnectionClosed`) during connection close draining
// - Binds client-initiated QUIC streams to pending requests and reclaims closed slots with O(1) swap-remove and zero heap allocation.

/// 单个 HTTP/3 会话调度器最多同时跟踪的流数量
pub const H3_DISPATCH_MAX_STREAMS: usize = 32;

/// 单条 HTTP/3 响应最多缓存的非伪头部数量
pub const H3_DISPATCH_MAX_HEADERS: usize = 32;

/// 未绑定 QUIC 流的哨兵 ID
pub const H3_DISPATCH_NO_STREAM: u64 = 0;

/// HTTP/3 回调调度错误集
pub err H3DispatchError {
    SessionClosed,
    TooManyStreams,
    TooManyHeaders,
    StreamNotFound,
    DuplicateRequestId,
    UnexpectedServerStream,
    NoPendingUnboundStream,
    InvalidQuicStreamId,
}

fn eqlBytes(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) { return false; }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) { return false; }
        i += 1;
    }
    return true;
}

fn parseStatusU16(raw: []const u8): u16 {
    if (raw.len != 3) { return 0; }
    let val: u16 = 0;
    let i: usize = 0;
    while (i < 3) {
        const c = raw[i];
        if (c < '0' || c > '9') { return 0; }
        val = val * 10 + @as(u16, c - '0');
        i += 1;
    }
    return val;
}

/// HTTP/3 连接关闭原因分类（对齐 Bun `callbacks.cp:55-58`）
pub enum H3ConnCloseReason: u8 {
    HandshakeFailed = 0,
    ConnectionClosed = 1,
    GoawayReceived = 2,
}

/// `onStreamHeaders` 事件处理结论
pub enum H3HeaderEventKind: u8 {
    DeliveredFinalHeaders = 0,
    IgnoredInformational1xx = 1,
    IgnoredTrailingHeaders = 2,
    ProtocolErrorMissingStatus = 3,
}

/// 原始 HTTP/3 解码头部键值对
pub struct H3RawHeader {
    name: []const u8 = "",
    value: []const u8 = "",
}

/// 调度器中单个 HTTP/3 流状态槽位
pub struct H3StreamSlot {
    request_id: u64 = 0,
    quic_stream_id: u64 = H3_DISPATCH_NO_STREAM,
    status_code: u16 = 0,
    header_count: usize = 0,
    headers: [H3_DISPATCH_MAX_HEADERS]H3RawHeader = [H3RawHeader{}; H3_DISPATCH_MAX_HEADERS],
    received_bytes: u64 = 0,
    fin_received: bool = false,

    fn headersSlice(): []const H3RawHeader {
        return _.headers[0.._.header_count];
    }
}

/// `onStreamHeaders` 返回摘要
pub struct H3HeaderDispatchResult {
    kind: H3HeaderEventKind = .DeliveredFinalHeaders,
    status_code: u16 = 0,
    regular_header_count: usize = 0,
}

/// `onConnClose` 批量排空结果
pub struct H3ConnCloseSummary {
    reason: H3ConnCloseReason = .ConnectionClosed,
    failed_count: usize = 0,
    failed_request_ids: [H3_DISPATCH_MAX_STREAMS]u64 = [0; H3_DISPATCH_MAX_STREAMS],

    fn failedIdsSlice(): []const u64 {
        return _.failed_request_ids[0.._.failed_count];
    }
}

/// 零堆分配 HTTP/3 客户端回调调度状态机
pub struct H3CallbackDispatcher {
    handshake_done: bool = false,
    closed: bool = false,
    goaway_received: bool = false,
    stream_count: usize = 0,
    streams: [H3_DISPATCH_MAX_STREAMS]H3StreamSlot = [H3StreamSlot{}; H3_DISPATCH_MAX_STREAMS],

    static fn init(): H3CallbackDispatcher {
        return H3CallbackDispatcher{};
    }

    fn activeStreamCount(): usize {
        return _.stream_count;
    }

    fn findSlotByQuicStream(quic_stream_id: u64): usize? {
        if (quic_stream_id == H3_DISPATCH_NO_STREAM) { return null; }
        let i: usize = 0;
        while (i < _.stream_count) {
            if (_.streams[i].quic_stream_id == quic_stream_id) {
                return i;
            }
            i += 1;
        }
        return null;
    }

    fn findSlotByRequestId(request_id: u64): usize? {
        let i: usize = 0;
        while (i < _.stream_count) {
            if (_.streams[i].request_id == request_id) {
                return i;
            }
            i += 1;
        }
        return null;
    }

    /// 入队一个新的待发送 HTTP/3 请求（返回分配槽位索引）
    fn enqueueRequest(*_, request_id: u64): H3DispatchError!usize {
        if (_.closed) { return H3DispatchError.SessionClosed; }
        if (_.findSlotByRequestId(request_id) != null) {
            return H3DispatchError.DuplicateRequestId;
        }
        if (_.stream_count >= H3_DISPATCH_MAX_STREAMS) {
            return H3DispatchError.TooManyStreams;
        }
        const slot = _.stream_count;
        _.streams[slot] = H3StreamSlot{
            request_id: request_id,
            quic_stream_id: H3_DISPATCH_NO_STREAM,
        };
        _.stream_count += 1;
        return slot;
    }

    /// 处理 QUIC 握手完成事件（对齐 `callbacks.cp:23-36`）：返回需要调用 `makeStream()` 的挂起流数量
    fn onHandshakeDone(*_, ok: bool): usize {
        if (!ok) {
            _.closed = true;
            return 0;
        }
        _.handshake_done = true;
        let unbound: usize = 0;
        let i: usize = 0;
        while (i < _.stream_count) {
            if (_.streams[i].quic_stream_id == H3_DISPATCH_NO_STREAM) {
                unbound += 1;
            }
            i += 1;
        }
        return unbound;
    }

    /// 处理服务端 GOAWAY 帧（对齐 `callbacks.cp:38-42`）：拒绝后续新请求，现有在途流继续完成
    fn onGoaway(*_) {
        _.goaway_received = true;
        _.closed = true;
    }

    /// 处理 QUIC 流打开事件（对齐 `callbacks.cp:66-97`）：将首个未绑定请求与 `quic_stream_id` 关联
    fn onStreamOpen(*_, quic_stream_id: u64, is_client: bool): H3DispatchError!u64 {
        if (!is_client) {
            return H3DispatchError.UnexpectedServerStream;
        }
        if (quic_stream_id == H3_DISPATCH_NO_STREAM) {
            return H3DispatchError.InvalidQuicStreamId;
        }
        let i: usize = 0;
        while (i < _.stream_count) {
            if (_.streams[i].quic_stream_id == H3_DISPATCH_NO_STREAM) {
                _.streams[i].quic_stream_id = quic_stream_id;
                return _.streams[i].request_id;
            }
            i += 1;
        }
        return H3DispatchError.NoPendingUnboundStream;
    }

    /// 处理流响应头帧（对齐 `callbacks.cp:99-131`）：提取 `:status`、过滤伪头、忽略 1xx 与 trailer
    fn onStreamHeaders(
        *_,
        quic_stream_id: u64,
        raw_headers: []const H3RawHeader,
    ): H3DispatchError!H3HeaderDispatchResult {
        const slot = _.findSlotByQuicStream(quic_stream_id) ?? return H3DispatchError.StreamNotFound;

        let status: u16 = 0;
        let reg_count: usize = 0;
        let tmp_headers: [H3_DISPATCH_MAX_HEADERS]H3RawHeader = [H3RawHeader{}; H3_DISPATCH_MAX_HEADERS];

        let i: usize = 0;
        while (i < raw_headers.len) {
            const h = raw_headers[i];
            if (h.name.len > 0 && h.name[0] == ':') {
                if (eqlBytes(h.name, ":status")) {
                    status = parseStatusU16(h.value);
                }
                i += 1;
                continue;
            }
            if (reg_count >= H3_DISPATCH_MAX_HEADERS) {
                return H3DispatchError.TooManyHeaders;
            }
            tmp_headers[reg_count] = h;
            reg_count += 1;
            i += 1;
        }

        if (status == 0) {
            if (_.streams[slot].status_code != 0) {
                return H3HeaderDispatchResult{
                    kind: .IgnoredTrailingHeaders,
                    status_code: _.streams[slot].status_code,
                    regular_header_count: reg_count,
                };
            }
            return H3HeaderDispatchResult{
                kind: .ProtocolErrorMissingStatus,
                status_code: 0,
                regular_header_count: 0,
            };
        }

        if (status >= 100 && status < 200) {
            return H3HeaderDispatchResult{
                kind: .IgnoredInformational1xx,
                status_code: status,
                regular_header_count: reg_count,
            };
        }

        _.streams[slot].status_code = status;
        _.streams[slot].header_count = reg_count;
        let k: usize = 0;
        while (k < reg_count) {
            _.streams[slot].headers[k] = tmp_headers[k];
            k += 1;
        }

        return H3HeaderDispatchResult{
            kind: .DeliveredFinalHeaders,
            status_code: status,
            regular_header_count: reg_count,
        };
    }

    /// 处理流数据分片事件（对齐 `callbacks.cp:133-140`）
    fn onStreamData(*_, quic_stream_id: u64, chunk_len: usize, fin: bool): H3DispatchError!bool {
        const slot = _.findSlotByQuicStream(quic_stream_id) ?? return H3DispatchError.StreamNotFound;
        _.streams[slot].received_bytes += @as(u64, @intCast(chunk_len));
        if (fin) {
            _.streams[slot].fin_received = true;
        }
        return _.streams[slot].fin_received;
    }

    /// 处理流关闭事件（对齐 `callbacks.cp:148-154`）：O(1) swap-remove 回收槽位并返回对应 `request_id`
    fn onStreamClose(*_, quic_stream_id: u64): H3DispatchError!u64 {
        const slot = _.findSlotByQuicStream(quic_stream_id) ?? return H3DispatchError.StreamNotFound;
        const req_id = _.streams[slot].request_id;
        const last = _.stream_count - 1;
        if (slot != last) {
            _.streams[slot] = _.streams[last];
        }
        _.stream_count -= 1;
        return req_id;
    }

    /// 处理连接关闭事件（对齐 `callbacks.cp:44-64`）：区分握手失败与断连并排空全部在途请求
    fn onConnClose(*_): H3ConnCloseSummary {
        _.closed = true;
        const reason = if (!_.handshake_done)
            H3ConnCloseReason.HandshakeFailed
        else if (_.goaway_received)
            H3ConnCloseReason.GoawayReceived
        else
            H3ConnCloseReason.ConnectionClosed;

        let summary = H3ConnCloseSummary{ reason: reason };
        let i: usize = 0;
        while (i < _.stream_count) {
            summary.failed_request_ids[summary.failed_count] = _.streams[i].request_id;
            summary.failed_count += 1;
            i += 1;
        }
        _.stream_count = 0;
        return summary;
    }
}
