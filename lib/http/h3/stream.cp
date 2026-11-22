// Source: Absorbed from /data/dev/bun/src/http/h3_client/Stream.cp | Synced: 2026-11-22T21:19:05Z
// Pure-CP zero-allocation HTTP/3 in-flight request stream state machine (`H3RequestStream`).
// Tracks pending request body delivery, decoded response headers, buffered or streaming
// response body accumulation, 1xx informational status handling, and idempotent QUIC abort.

/// 零堆分配单条 HTTP/3 流最大已解码响应头数量
pub const H3_REQ_STREAM_MAX_HEADERS: usize = 32;

/// 零堆分配单条 HTTP/3 流内联响应体缓冲区容量（字节）
pub const H3_REQ_STREAM_MAX_BODY_BUF: usize = 4096;

/// `H3RequestStream` 状态机与容量错误集
pub err H3RequestStreamError {
    TooManyHeaders,
    BodyBufferOverflow,
    InvalidStateTransition,
    InvalidStatusCode,
    StreamAborted,
    HeadersAlreadyDelivered,
    RequestBodyAlreadyFinished,
}

/// HTTP/3 请求流生命周期阶段
pub enum H3StreamPhase {
    Open,
    RequestBodySending,
    HeadersReceived,
    ReceivingBody,
    Completed,
    Aborted,
}

/// 零拷贝 HTTP/3 已解码响应头键值视图
pub struct H3DecodedHeader {
    name: []const u8 = "",
    value: []const u8 = "",
}

fn asciiLower(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') {
        return c + 32;
    }
    return c;
}

fn asciiEqlIgnoreCase(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (asciiLower(a[i]) != asciiLower(b[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

/// 零堆分配单条在途 HTTP/3 请求流状态机（对齐 Bun `http/h3_client/Stream.cp`）
pub struct H3RequestStream {
    stream_id: u64 = 0,
    session_id: u32 = 0,
    phase: H3StreamPhase = H3StreamPhase.Open,
    status_code: u16 = 0,
    headers: [32]H3DecodedHeader = [H3DecodedHeader{}; 32],
    header_count: usize = 0,
    body_buf: [4096]u8 = [0; 4096],
    body_len: usize = 0,
    total_body_bytes: u64 = 0,
    pending_body: []const u8 = "",
    pending_body_offset: usize = 0,
    request_body_done: bool = false,
    is_streaming_body: bool = false,
    headers_delivered: bool = false,
    abort_error_code: u64 = 0,

    /// 初始化一条在途 HTTP/3 请求流
    static fn init(stream_id: u64, session_id: u32, is_streaming_body: bool): H3RequestStream {
        return H3RequestStream{
            stream_id: stream_id,
            session_id: session_id,
            phase: H3StreamPhase.Open,
            is_streaming_body: is_streaming_body,
        };
    }

    /// 判断流是否已进入终态（`Completed` 或 `Aborted`）
    fn isTerminal(): bool {
        return _.phase == H3StreamPhase.Completed || _.phase == H3StreamPhase.Aborted;
    }

    /// 判断当前响应状态码是否为 1xx 信息响应（如 `100 Continue` / `103 Early Hints`）
    fn isInformational(): bool {
        return _.status_code >= 100 && _.status_code < 200;
    }

    /// 绑定待发送的请求体切片（进入 `RequestBodySending` 阶段），返回待发送字节数
    fn setPendingBody(*_, body: []const u8): H3RequestStreamError!usize {
        if (_.phase == H3StreamPhase.Aborted) {
            return H3RequestStreamError.StreamAborted;
        }
        if (_.request_body_done) {
            return H3RequestStreamError.RequestBodyAlreadyFinished;
        }
        _.pending_body = body;
        _.pending_body_offset = 0;
        if (body.len == 0) {
            _.request_body_done = true;
        } else if (_.phase == H3StreamPhase.Open) {
            _.phase = H3StreamPhase.RequestBodySending;
        }
        return body.len;
    }

    /// 零拷贝查看当前尚未写出的请求体剩余切片
    fn pendingBodyRemaining(): []const u8 {
        if (_.pending_body_offset >= _.pending_body.len) {
            return _.pending_body[_.pending_body.len.._.pending_body.len];
        }
        return _.pending_body[_.pending_body_offset..];
    }

    /// 推进已成功写入 QUIC 流的请求体字节数，全部发完自动标记 `request_body_done = true`
    fn advancePendingBody(*_, sent: usize): H3RequestStreamError!usize {
        if (_.phase == H3StreamPhase.Aborted) {
            return H3RequestStreamError.StreamAborted;
        }
        const rem = _.pendingBodyRemaining().len;
        if (sent >= rem) {
            _.pending_body_offset = _.pending_body.len;
            _.request_body_done = true;
            return 0;
        }
        _.pending_body_offset += sent;
        return _.pending_body.len - _.pending_body_offset;
    }

    /// 显式标记请求体 FIN 已发送（适用于无请求体的 GET/HEAD 或流式上传结束）
    fn markRequestBodyDone(*_): H3RequestStreamError!bool {
        if (_.phase == H3StreamPhase.Aborted) {
            return H3RequestStreamError.StreamAborted;
        }
        _.request_body_done = true;
        _.pending_body_offset = _.pending_body.len;
        return true;
    }

    /// 记录响应 `:status` 伪头部（校验 RFC 9110 `100..599` 范围）
    fn onStatusCode(*_, code: u16): H3RequestStreamError!u16 {
        if (_.phase == H3StreamPhase.Aborted) {
            return H3RequestStreamError.StreamAborted;
        }
        if (_.phase == H3StreamPhase.Completed) {
            return H3RequestStreamError.InvalidStateTransition;
        }
        if (code < 100 || code > 599) {
            return H3RequestStreamError.InvalidStatusCode;
        }
        if (_.headers_delivered) {
            return H3RequestStreamError.HeadersAlreadyDelivered;
        }
        // 若此前收到过 1xx 信息响应头，收到最终响应状态码前重置头列表
        if (_.isInformational() && code >= 200) {
            _.header_count = 0;
        }
        _.status_code = code;
        return code;
    }

    /// 追加一条 QPACK 解码后的响应头，返回当前头总数
    fn addDecodedHeader(*_, name: []const u8, value: []const u8): H3RequestStreamError!usize {
        if (_.phase == H3StreamPhase.Aborted) {
            return H3RequestStreamError.StreamAborted;
        }
        if (_.headers_delivered) {
            return H3RequestStreamError.HeadersAlreadyDelivered;
        }
        if (_.header_count >= H3_REQ_STREAM_MAX_HEADERS) {
            return H3RequestStreamError.TooManyHeaders;
        }
        _.headers[_.header_count] = H3DecodedHeader{
            name: name,
            value: value,
        };
        _.header_count += 1;
        return _.header_count;
    }

    /// 大小写不敏感查找响应头值
    fn findHeader(name: []const u8): ([]const u8)? {
        let i: usize = 0;
        while (i < _.header_count) {
            if (asciiEqlIgnoreCase(_.headers[i].name, name)) {
                return _.headers[i].value;
            }
            i += 1;
        }
        return null;
    }

    /// 交付完整响应头至上层 HTTP 客户端（1xx 响应返回 `false` 保持等待最终响应头）
    fn deliverHeaders(*_): H3RequestStreamError!bool {
        if (_.phase == H3StreamPhase.Aborted) {
            return H3RequestStreamError.StreamAborted;
        }
        if (_.status_code < 100 || _.status_code > 599) {
            return H3RequestStreamError.InvalidStatusCode;
        }
        if (_.headers_delivered) {
            return H3RequestStreamError.HeadersAlreadyDelivered;
        }
        if (_.isInformational()) {
            return false;
        }
        _.headers_delivered = true;
        _.phase = H3StreamPhase.HeadersReceived;
        return true;
    }

    /// 追加响应体数据块：流式模式仅累加 `total_body_bytes`，缓冲模式同步写入内联 `body_buf`
    fn appendResponseBody(*_, chunk: []const u8): H3RequestStreamError!usize {
        if (_.phase == H3StreamPhase.Aborted) {
            return H3RequestStreamError.StreamAborted;
        }
        if (!_.headers_delivered || _.phase == H3StreamPhase.Completed) {
            return H3RequestStreamError.InvalidStateTransition;
        }
        _.phase = H3StreamPhase.ReceivingBody;
        if (!_.is_streaming_body) {
            if (chunk.len > H3_REQ_STREAM_MAX_BODY_BUF - _.body_len) {
                return H3RequestStreamError.BodyBufferOverflow;
            }
            let i: usize = 0;
            while (i < chunk.len) {
                _.body_buf[_.body_len + i] = chunk[i];
                i += 1;
            }
            _.body_len += chunk.len;
        }
        _.total_body_bytes += @as(u64, @intCast(chunk.len));
        return chunk.len;
    }

    /// 返回缓冲模式下已累积的响应体切片
    fn responseBody(): []const u8 {
        return _.body_buf[0.._.body_len];
    }

    /// 标记响应接收完成（流进入 `Completed` 终态）
    fn complete(*_): H3RequestStreamError!bool {
        if (_.phase == H3StreamPhase.Aborted) {
            return H3RequestStreamError.StreamAborted;
        }
        if (!_.headers_delivered) {
            return H3RequestStreamError.InvalidStateTransition;
        }
        _.phase = H3StreamPhase.Completed;
        return true;
    }

    /// 幂等中止当前 HTTP/3 请求流（对应 Bun `Stream.abort()` -> `qstream.close()`）
    fn abort(*_, quic_error_code: u64) {
        if (_.phase == H3StreamPhase.Aborted) {
            return;
        }
        _.phase = H3StreamPhase.Aborted;
        _.abort_error_code = quic_error_code;
        _.pending_body = "";
        _.pending_body_offset = 0;
        _.request_body_done = true;
    }
}
