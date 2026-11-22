// Source: Absorbed from /data/dev/bun/src/http/h2_client/Stream.cp | Synced: 2026-11-22T00:14:33Z

pub const H2_MAX_WINDOW_SIZE: i32 = 2147483647;
pub const H2_DEFAULT_INITIAL_WINDOW: i32 = 65535;
pub const H2_RST_STREAM_FRAME_LEN: usize = 13;
pub const H2_WINDOW_UPDATE_FRAME_LEN: usize = 13;

const H2_FRAME_RST_STREAM: u8 = 0x3;
const H2_FRAME_WINDOW_UPDATE: u8 = 0x8;

/// RFC 9113 §5.1 HTTP/2 stream lifecycle state (absorbed from Bun `src/http/h2_client/Stream.cp`).
pub enum StreamState {
    Open,
    HalfClosedLocal,
    HalfClosedRemote,
    Closed,
}

/// RFC 9113 §7 HTTP/2 error codes for `RST_STREAM` and `GOAWAY`.
pub enum H2ErrorCode {
    NoError,
    ProtocolError,
    InternalError,
    FlowControlError,
    SettingsTimeout,
    StreamClosed,
    FrameSizeError,
    RefusedStream,
    Cancel,
    CompressionError,
    ConnectError,
    EnhanceYourCalm,
    InadequateSecurity,
    Http11Required,
}

/// Converts `H2ErrorCode` to its RFC 9113 wire u32 value.
pub fn errorCodeToU32(code: H2ErrorCode): u32 {
    match (code) {
        H2ErrorCode.NoError => return 0,
        H2ErrorCode.ProtocolError => return 1,
        H2ErrorCode.InternalError => return 2,
        H2ErrorCode.FlowControlError => return 3,
        H2ErrorCode.SettingsTimeout => return 4,
        H2ErrorCode.StreamClosed => return 5,
        H2ErrorCode.FrameSizeError => return 6,
        H2ErrorCode.RefusedStream => return 7,
        H2ErrorCode.Cancel => return 8,
        H2ErrorCode.CompressionError => return 9,
        H2ErrorCode.ConnectError => return 10,
        H2ErrorCode.EnhanceYourCalm => return 11,
        H2ErrorCode.InadequateSecurity => return 12,
        H2ErrorCode.Http11Required => return 13,
    }
}

fn writeFrameHdr(payload_len: u32, ftype: u8, flags: u8, stream_id: u32, out_buf: *[]u8) {
    out_buf.*[0] = ((payload_len >> 16) & 0xFF) as u8;
    out_buf.*[1] = ((payload_len >> 8) & 0xFF) as u8;
    out_buf.*[2] = (payload_len & 0xFF) as u8;
    out_buf.*[3] = ftype;
    out_buf.*[4] = flags;
    out_buf.*[5] = ((stream_id >> 24) & 0x7F) as u8;
    out_buf.*[6] = ((stream_id >> 16) & 0xFF) as u8;
    out_buf.*[7] = ((stream_id >> 8) & 0xFF) as u8;
    out_buf.*[8] = (stream_id & 0xFF) as u8;
}

/// Encodes a 13-byte RFC 9113 §6.4 `RST_STREAM` frame into `out_buf`.
/// Requires `stream_id` in `1..0x7FFFFFFF` and `out_buf.len >= 13`.
pub fn encodeRstStreamFrame(stream_id: u32, code: H2ErrorCode, out_buf: *[]u8): usize? {
    if (stream_id == 0 || stream_id > 0x7FFFFFFF || out_buf.*.len < H2_RST_STREAM_FRAME_LEN) {
        return null;
    }
    writeFrameHdr(4, H2_FRAME_RST_STREAM, 0, stream_id, out_buf);
    const raw = errorCodeToU32(code);
    out_buf.*[9] = ((raw >> 24) & 0xFF) as u8;
    out_buf.*[10] = ((raw >> 16) & 0xFF) as u8;
    out_buf.*[11] = ((raw >> 8) & 0xFF) as u8;
    out_buf.*[12] = (raw & 0xFF) as u8;
    return H2_RST_STREAM_FRAME_LEN;
}

/// Encodes a 13-byte RFC 9113 §6.9 `WINDOW_UPDATE` frame into `out_buf`.
/// `stream_id == 0` targets the connection window; `increment` must be in `1..0x7FFFFFFF`.
pub fn encodeWindowUpdateFrame(stream_id: u32, increment: u32, out_buf: *[]u8): usize? {
    if (
        stream_id > 0x7FFFFFFF ||
        increment == 0 ||
        increment > 0x7FFFFFFF ||
        out_buf.*.len < H2_WINDOW_UPDATE_FRAME_LEN
    ) {
        return null;
    }
    writeFrameHdr(4, H2_FRAME_WINDOW_UPDATE, 0, stream_id, out_buf);
    out_buf.*[9] = ((increment >> 24) & 0x7F) as u8;
    out_buf.*[10] = ((increment >> 16) & 0xFF) as u8;
    out_buf.*[11] = ((increment >> 8) & 0xFF) as u8;
    out_buf.*[12] = (increment & 0xFF) as u8;
    return H2_WINDOW_UPDATE_FRAME_LEN;
}

/// Zero-alloc RFC 9113 HTTP/2 multiplexed stream state machine & flow-control window tracker
/// (absorbed from Bun `src/http/h2_client/Stream.cp`).
pub struct H2Stream {
    id: u32,
    state: StreamState,
    rst_done: bool,
    rst_error: u32,
    headers_ready: bool,
    headers_end_stream: bool,
    awaiting_continue: bool,
    status_code: u16,
    send_window: i32,
    recv_window: i32,
    unacked_bytes: u32,
    data_bytes_sent: u64,
    data_bytes_received: u64,

    /// Creates a new `Open` HTTP/2 stream with validated stream ID and non-negative initial windows.
    fn init(id: u32, initial_send_window: i32, initial_recv_window: i32): H2Stream? {
        if (id == 0 || id > 0x7FFFFFFF || initial_send_window < 0 || initial_recv_window < 0) {
            return null;
        }
        return H2Stream {
            id: id,
            state: StreamState.Open,
            rst_done: false,
            rst_error: 0,
            headers_ready: false,
            headers_end_stream: false,
            awaiting_continue: false,
            status_code: 0,
            send_window: initial_send_window,
            recv_window: initial_recv_window,
            unacked_bytes: 0,
            data_bytes_sent: 0,
            data_bytes_received: 0,
        };
    }

    /// Returns true if the local side has closed sending (`HalfClosedLocal` or `Closed`).
    fn localClosed(): bool {
        return _.state == StreamState.HalfClosedLocal || _.state == StreamState.Closed;
    }

    /// Returns true if the remote peer has closed sending (`HalfClosedRemote` or `Closed`).
    fn remoteClosed(): bool {
        return _.state == StreamState.HalfClosedRemote || _.state == StreamState.Closed;
    }

    /// Returns true if the stream is fully `Closed`.
    fn isClosed(): bool {
        return _.state == StreamState.Closed;
    }

    /// Returns true if outbound DATA frames can currently be sent on this stream.
    fn canSendData(): bool {
        return !_.localClosed() && !_.awaiting_continue && _.send_window > 0;
    }

    /// Transitions state after sending `END_STREAM` (absorbed from Bun `Stream.sentEndStream`).
    fn sentEndStream(*_) {
        match (_.state) {
            StreamState.Open => _.state = StreamState.HalfClosedLocal,
            StreamState.HalfClosedRemote => _.state = StreamState.Closed,
            StreamState.HalfClosedLocal => {}
            StreamState.Closed => {}
        }
    }

    /// Transitions state after receiving `END_STREAM` (absorbed from Bun `Stream.recvEndStream`).
    fn recvEndStream(*_) {
        match (_.state) {
            StreamState.Open => _.state = StreamState.HalfClosedRemote,
            StreamState.HalfClosedLocal => _.state = StreamState.Closed,
            StreamState.HalfClosedRemote => {}
            StreamState.Closed => {}
        }
    }

    /// Records received response headers; clears `awaiting_continue` on `100 Continue`,
    /// and closes the remote side if `end_stream` is set.
    fn markHeadersReceived(*_, status_code: u16, end_stream: bool): bool {
        if (_.remoteClosed()) {
            return false;
        }
        if (status_code == 100) {
            _.awaiting_continue = false;
            return true;
        }
        _.status_code = status_code;
        _.headers_ready = true;
        _.headers_end_stream = end_stream;
        _.awaiting_continue = false;
        if (end_stream) {
            _.recvEndStream();
        }
        return true;
    }

    /// Idempotently resets the stream (absorbed from Bun `Stream.rst`) and encodes a 13-byte `RST_STREAM` frame.
    /// Returns `0` if already reset/closed, `13` when a frame was emitted, or `null` if `out_rst_frame` is too small.
    fn reset(*_, code: H2ErrorCode, out_rst_frame: *[]u8): usize? {
        if (_.rst_done || _.state == StreamState.Closed) {
            return 0;
        }
        const n = encodeRstStreamFrame(_.id, code, out_rst_frame) ?? return null;
        _.rst_done = true;
        _.rst_error = errorCodeToU32(code);
        _.state = StreamState.Closed;
        return n;
    }

    /// Consumes `bytes` from the outbound send window. Returns `false` if locally closed or insufficient window.
    fn consumeSendWindow(*_, bytes: u32): bool {
        if (_.localClosed() || _.send_window < 0) {
            return false;
        }
        if ((bytes as i64) > (_.send_window as i64)) {
            return false;
        }
        _.send_window -= bytes as i32;
        _.data_bytes_sent += bytes as u64;
        return true;
    }

    /// Applies an inbound `WINDOW_UPDATE` increment to `send_window`.
    /// Returns `false` on zero increment or RFC 9113 §6.9.1 flow-control overflow (`> 2^31 - 1`).
    fn applySendWindowUpdate(*_, increment: u32): bool {
        if (increment == 0 || increment > 0x7FFFFFFF) {
            return false;
        }
        const next_val: i64 = (_.send_window as i64) + (increment as i64);
        if (next_val > (H2_MAX_WINDOW_SIZE as i64)) {
            return false;
        }
        _.send_window = next_val as i32;
        return true;
    }

    /// Applies a `SETTINGS_INITIAL_WINDOW_SIZE` change delta (may legally drive `send_window` negative per RFC 9113 §6.9.2).
    /// Returns `false` if the resulting window overflows `i32` bounds (`[-2^31, 2^31 - 1]`).
    fn applySettingsInitialWindowDelta(*_, delta: i64): bool {
        const next_val: i64 = (_.send_window as i64) + delta;
        if (next_val > (H2_MAX_WINDOW_SIZE as i64) || next_val < -2147483648) {
            return false;
        }
        _.send_window = next_val as i32;
        return true;
    }

    /// Records inbound `DATA` payload bytes, consuming `recv_window` and tracking `unacked_bytes`.
    /// Returns `false` if remote is already closed or flow-control window is exceeded.
    fn recordReceivedData(*_, bytes: u32, end_stream: bool): bool {
        if (_.remoteClosed()) {
            return false;
        }
        if ((bytes as i64) > (_.recv_window as i64)) {
            return false;
        }
        _.recv_window -= bytes as i32;
        _.unacked_bytes += bytes;
        _.data_bytes_received += bytes as u64;
        if (end_stream) {
            _.recvEndStream();
        }
        return true;
    }

    /// Emits a 13-byte `WINDOW_UPDATE` frame into `out_frame` when `unacked_bytes >= threshold`
    /// and the remote side is still open. Replenishes `recv_window` and clears `unacked_bytes`.
    fn maybeEmitWindowUpdate(*_, threshold: u32, out_frame: *[]u8): usize? {
        if (_.remoteClosed() || _.unacked_bytes == 0 || _.unacked_bytes < threshold) {
            return 0;
        }
        const inc = _.unacked_bytes;
        const next_recv: i64 = (_.recv_window as i64) + (inc as i64);
        if (next_recv > (H2_MAX_WINDOW_SIZE as i64)) {
            return null;
        }
        const n = encodeWindowUpdateFrame(_.id, inc, out_frame) ?? return null;
        _.recv_window = next_recv as i32;
        _.unacked_bytes = 0;
        return n;
    }
}
