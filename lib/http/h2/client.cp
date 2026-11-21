// Source: Absorbed from /data/dev/bun/src/http/h2_client/ClientSession.cp | Synced: 2026-11-21T22:49:01Z
// lib/http/h2/client.cp
// CP HTTP/2 Client Session, Stream Manager & Flow Control State Machine (cap:http)

const std = @import("std");
import {
    CLIENT_PREFACE,
    FrameType,
    FrameHeader,
    ErrorCode,
    SettingsType,
    SettingsFlags,
    SettingsPayloadUnit,
    PingFrameFlags,
    DataFrameFlags,
    HeadersFrameFlags,
    RstStreamFrame,
    WindowUpdateFrame,
    PingFrame,
    GoawayFrame,
    MAX_WINDOW_SIZE,
    MAX_FRAME_SIZE,
    DEFAULT_WINDOW_SIZE,
    DEFAULT_MAX_FRAME_SIZE,
} from "./frame";

pub enum H2StreamState {
    Idle,
    ReservedLocal,
    ReservedRemote,
    Open,
    HalfClosedLocal,
    HalfClosedRemote,
    Closed,
}

pub struct H2Stream {
    pub id: u32 = 0;
    pub state: H2StreamState = H2StreamState.Idle;
    pub window_size: i32 = DEFAULT_WINDOW_SIZE;
    pub recv_window: i32 = DEFAULT_WINDOW_SIZE;
    pub unacked_bytes: u32 = 0;
    pub rst_done: bool = false;
    pub last_error: ErrorCode = ErrorCode.NoError;

    pub fn isOpen(): bool {
        return _.state == .Open || _.state == .HalfClosedRemote;
    }

    pub fn rst(*_, code: ErrorCode) {
        if (_.rst_done || _.state == .Closed) return;
        _.last_error = code;
        _.rst_done = true;
        _.state = .Closed;
    }

    pub fn sentEndStream(*_) {
        _.state = switch (_.state) {
            .Open => .HalfClosedLocal,
            .HalfClosedRemote => .Closed,
            else => _.state,
        };
    }

    pub fn recvEndStream(*_) {
        _.state = switch (_.state) {
            .Open => .HalfClosedRemote,
            .HalfClosedLocal => .Closed,
            else => _.state,
        };
    }

    pub fn localClosed(): bool {
        return _.state == .HalfClosedLocal || _.state == .Closed;
    }

    pub fn remoteClosed(): bool {
        return _.state == .HalfClosedRemote || _.state == .Closed;
    }

    pub fn close(*_) {
        _.state = .Closed;
    }

    pub fn consumeSend(*_, amount: i32): bool {
        if (amount < 0 || amount > _.window_size) return false;
        _.window_size -= amount;
        return true;
    }

    pub fn creditSend(*_, increment: u32): bool {
        if (increment == 0) return false;
        const next: i64 = (_.window_size as i64) + (increment as i64);
        if (next > (MAX_WINDOW_SIZE as i64)) return false;
        _.window_size = next as i32;
        return true;
    }
}

pub struct H2FlowControl {
    pub conn_send_window: i32 = DEFAULT_WINDOW_SIZE;
    pub conn_recv_window: i32 = DEFAULT_WINDOW_SIZE;
    pub conn_unacked_bytes: u32 = 0;
    pub remote_initial_window_size: u32 = DEFAULT_WINDOW_SIZE;
    pub local_initial_window_size: u32 = DEFAULT_WINDOW_SIZE;

    pub fn availableSendWindow(stream: H2Stream): i32 {
        const cw = _.conn_send_window;
        const sw = stream.window_size;
        const m = if (cw < sw) cw else sw;
        return if (m < 0) 0 else m;
    }

    pub fn consumeSend(*_, stream: *H2Stream, len: u32): bool {
        const amt = len as i32;
        if (amt < 0 || amt > _.conn_send_window || amt > stream.window_size) return false;
        _.conn_send_window -= amt;
        stream.window_size -= amt;
        return true;
    }

    pub fn creditConnSend(*_, increment: u32): bool {
        if (increment == 0) return false;
        const next: i64 = (_.conn_send_window as i64) + (increment as i64);
        if (next > (MAX_WINDOW_SIZE as i64)) return false;
        _.conn_send_window = next as i32;
        return true;
    }

    pub fn consumeRecv(*_, stream: *H2Stream, len: u32): bool {
        const amt = len as i32;
        if (amt < 0 || amt > _.conn_recv_window || amt > stream.recv_window) return false;
        _.conn_recv_window -= amt;
        _.conn_unacked_bytes += len;
        stream.recv_window -= amt;
        stream.unacked_bytes += len;
        return true;
    }

    pub fn shouldReplenishConn(): bool {
        return _.conn_unacked_bytes >= (_.local_initial_window_size / 2);
    }

    pub fn takeConnReplenish(*_): u32 {
        const amt = _.conn_unacked_bytes;
        _.conn_unacked_bytes = 0;
        const next: i64 = (_.conn_recv_window as i64) + (amt as i64);
        _.conn_recv_window = if (next > (MAX_WINDOW_SIZE as i64)) MAX_WINDOW_SIZE else (next as i32);
        return amt;
    }

    pub fn shouldReplenishStream(stream: H2Stream): bool {
        return !stream.remoteClosed() && stream.unacked_bytes >= (_.local_initial_window_size / 2);
    }

    pub fn takeStreamReplenish(*_, stream: *H2Stream): u32 {
        const amt = stream.unacked_bytes;
        stream.unacked_bytes = 0;
        const next: i64 = (stream.recv_window as i64) + (amt as i64);
        stream.recv_window = if (next > (MAX_WINDOW_SIZE as i64)) MAX_WINDOW_SIZE else (next as i32);
        return amt;
    }
}

pub enum H2FrameActionKind: u8 {
    None = 0,
    ReplyReady = 1,
    HeadersComplete = 2,
    DataReceived = 3,
    StreamReset = 4,
    GoawayReceived = 5,
    PingReceived = 6,
    SettingsAcked = 7,
    WindowUpdated = 8,
    ProtocolError = 9,
}

pub struct H2FrameAction {
    pub kind: H2FrameActionKind = H2FrameActionKind.None;
    pub reply_len: usize = 0;
    pub stream_id: u32 = 0;
    pub error_code: ErrorCode = ErrorCode.NoError;
    pub end_stream: bool = false;
    pub window_increment: u32 = 0;
}

pub struct H2ClientSession {
    pub next_stream_id: u32 = 1;
    pub peer_window_size: i32 = DEFAULT_WINDOW_SIZE;
    pub local_window_size: i32 = DEFAULT_WINDOW_SIZE;
    pub max_frame_size: u32 = DEFAULT_MAX_FRAME_SIZE;
    pub max_concurrent_streams: u32 = 100;
    pub active_streams: usize = 0;
    pub is_goaway: bool = false;
    pub goaway_last_stream_id: u32 = 0;
    pub goaway_error: ErrorCode = ErrorCode.NoError;
    pub settings_received: bool = false;
    pub remote_header_table_size: u32 = 4096;
    pub expecting_continuation: u32 = 0;
    pub fatal_error: ErrorCode = ErrorCode.NoError;
    pub flow: H2FlowControl = H2FlowControl{},

    pub fn newStream(*_): H2Stream {
        const stream_id = _.next_stream_id;
        _.next_stream_id += 2; // Client stream IDs are odd (1, 3, 5...)
        _.active_streams += 1;
        return H2Stream{
            id: stream_id,
            state: H2StreamState.Open,
            window_size: _.peer_window_size,
            recv_window: _.local_window_size,
            unacked_bytes: 0,
            rst_done: false,
            last_error: ErrorCode.NoError,
        };
    }

    pub fn canCreateStream(): bool {
        return !_.is_goaway && _.fatal_error == ErrorCode.NoError && (_.active_streams < _.max_concurrent_streams);
    }

    pub fn onGoaway(*_) {
        _.is_goaway = true;
    }

    pub fn buildPrefaceAndSettings(*_, out: []u8, enable_push: bool = false): usize {
        const settings_count: usize = 3;
        const total = CLIENT_PREFACE.len + FrameHeader.byte_size + (settings_count * SettingsPayloadUnit.byte_size);
        if (out.len < total) return 0;

        @memcpy(out[0..CLIENT_PREFACE.len], CLIENT_PREFACE);
        let pos: usize = CLIENT_PREFACE.len;

        const fh = FrameHeader{
            length: @intCast(settings_count * SettingsPayloadUnit.byte_size),
            type: FrameType.Settings,
            flags: 0,
            stream_id: 0,
        };
        pos += fh.encodeSlice(out[pos..]);

        const s1 = SettingsPayloadUnit{ type: SettingsType.EnablePush, value: if (enable_push) 1 else 0 };
        const s2 = SettingsPayloadUnit{ type: SettingsType.InitialWindowSize, value: _.local_window_size as u32 };
        const s3 = SettingsPayloadUnit{ type: SettingsType.MaxFrameSize, value: _.max_frame_size };
        let unit_buf: [6]u8 = undefined;
        s1.encode(&unit_buf);
        @memcpy(out[pos .. pos + 6], &unit_buf);
        pos += 6;
        s2.encode(&unit_buf);
        @memcpy(out[pos .. pos + 6], &unit_buf);
        pos += 6;
        s3.encode(&unit_buf);
        @memcpy(out[pos .. pos + 6], &unit_buf);
        pos += 6;
        return pos;
    }

    pub static fn buildSettingsAck(out: []u8): usize {
        const fh = FrameHeader{ length: 0, type: FrameType.Settings, flags: @intFromEnum(SettingsFlags.Ack), stream_id: 0 };
        return fh.encodeSlice(out);
    }

    pub static fn buildWindowUpdate(out: []u8, stream_id: u32, increment: u32): usize {
        if (increment == 0 || out.len < FrameHeader.byte_size + WindowUpdateFrame.byte_size) return 0;
        const fh = FrameHeader{ length: 4, type: FrameType.WindowUpdate, flags: 0, stream_id: stream_id };
        let pos = fh.encodeSlice(out);
        let wu_buf: [4]u8 = undefined;
        const wu = WindowUpdateFrame{ increment: increment };
        wu.encode(&wu_buf);
        @memcpy(out[pos .. pos + 4], &wu_buf);
        return pos + 4;
    }

    pub static fn buildPing(out: []u8, opaque_data: [8]u8, ack: bool): usize {
        if (out.len < FrameHeader.byte_size + PingFrame.byte_size) return 0;
        const flags: u8 = if (ack) @intFromEnum(PingFrameFlags.Ack) else 0;
        const fh = FrameHeader{ length: 8, type: FrameType.Ping, flags: flags, stream_id: 0 };
        let pos = fh.encodeSlice(out);
        @memcpy(out[pos .. pos + 8], &opaque_data);
        return pos + 8;
    }

    pub static fn buildRstStream(out: []u8, stream_id: u32, code: ErrorCode): usize {
        if (stream_id == 0 || out.len < FrameHeader.byte_size + RstStreamFrame.byte_size) return 0;
        const fh = FrameHeader{ length: 4, type: FrameType.RstStream, flags: 0, stream_id: stream_id };
        let pos = fh.encodeSlice(out);
        let rst_buf: [4]u8 = undefined;
        const rst = RstStreamFrame{ error_code: code };
        rst.encode(&rst_buf);
        @memcpy(out[pos .. pos + 4], &rst_buf);
        return pos + 4;
    }

    pub static fn buildGoaway(out: []u8, last_stream_id: u32, code: ErrorCode, debug_data: []const u8): usize {
        const payload_len = 8 + debug_data.len;
        if (out.len < FrameHeader.byte_size + payload_len) return 0;
        const fh = FrameHeader{ length: @intCast(payload_len), type: FrameType.Goaway, flags: 0, stream_id: 0 };
        let pos = fh.encodeSlice(out);
        const gf = GoawayFrame{ last_stream_id: last_sid_clamp(last_stream_id), error_code: code, debug_data: debug_data };
        const n = gf.encode(out[pos..]);
        if (n == 0) return 0;
        return pos + n;
    }

    fn last_sid_clamp(sid: u32): u32 {
        return sid & 0x7FFFFFFF;
    }

    /// Encodes a header block into a HEADERS frame followed by zero or more CONTINUATION frames respecting `max_frame_size`.
    pub fn buildHeaderFrames(out: []u8, stream_id: u32, header_block: []const u8, end_stream: bool): usize {
        if (stream_id == 0) return 0;
        const max_chunk = if (_.max_frame_size < DEFAULT_MAX_FRAME_SIZE) (DEFAULT_MAX_FRAME_SIZE as usize) else (_.max_frame_size as usize);
        let pos: usize = 0;
        let offset: usize = 0;
        let first = true;

        while (true) {
            const rem = header_block.len - offset;
            const chunk_len = if (rem > max_chunk) max_chunk else rem;
            const last = (offset + chunk_len) == header_block.len;

            let flags: u8 = 0;
            if (last) flags |= @intFromEnum(HeadersFrameFlags.EndHeaders);
            if (first && end_stream) flags |= @intFromEnum(HeadersFrameFlags.EndStream);

            const ftype = if (first) FrameType.Headers else FrameType.Continuation;
            const fh = FrameHeader{
                length: @intCast(chunk_len),
                type: ftype,
                flags: flags,
                stream_id: stream_id,
            };
            if (pos + FrameHeader.byte_size + chunk_len > out.len) return 0;
            pos += fh.encodeSlice(out[pos..]);
            if (chunk_len > 0) {
                @memcpy(out[pos .. pos + chunk_len], header_block[offset .. offset + chunk_len]);
                pos += chunk_len;
            }
            offset += chunk_len;
            first = false;
            if (last) break;
        }
        return pos;
    }

    /// Processes an incoming HTTP/2 frame, validating RFC 7540 constraints and writing any immediate control reply (SETTINGS ACK / PING ACK / GOAWAY).
    pub fn processFrame(*_, header: FrameHeader, payload: []const u8, out_reply: []u8, stream: (*H2Stream)?): H2FrameAction {
        if ((header.length as usize) > payload.len || (header.length as u32) > _.max_frame_size && header.type != FrameType.Settings) {
            _.fatal_error = ErrorCode.FrameSizeError;
            return H2FrameAction{ kind: H2FrameActionKind.ProtocolError, error_code: ErrorCode.FrameSizeError };
        }

        if (_.expecting_continuation != 0 && header.type != FrameType.Continuation) {
            _.fatal_error = ErrorCode.ProtocolError;
            return H2FrameAction{ kind: H2FrameActionKind.ProtocolError, error_code: ErrorCode.ProtocolError };
        }

        switch (header.type) {
            .Settings => {
                if (header.stream_id != 0) {
                    _.fatal_error = ErrorCode.ProtocolError;
                    return H2FrameAction{ kind: H2FrameActionKind.ProtocolError, error_code: ErrorCode.ProtocolError };
                }
                if ((header.flags & @intFromEnum(SettingsFlags.Ack)) != 0) {
                    if (header.length != 0) {
                        _.fatal_error = ErrorCode.FrameSizeError;
                        return H2FrameAction{ kind: H2FrameActionKind.ProtocolError, error_code: ErrorCode.FrameSizeError };
                    }
                    return H2FrameAction{ kind: H2FrameActionKind.SettingsAcked };
                }
                if ((header.length % 6) != 0) {
                    _.fatal_error = ErrorCode.FrameSizeError;
                    return H2FrameAction{ kind: H2FrameActionKind.ProtocolError, error_code: ErrorCode.FrameSizeError };
                }

                let i: usize = 0;
                while (i + 6 <= payload.len) : (i += 6) {
                    const unit = SettingsPayloadUnit.decode(payload[i .. i + 6]) orelse break;
                    switch (unit.type) {
                        .HeaderTableSize => {
                            _.remote_header_table_size = unit.value;
                        },
                        .MaxConcurrentStreams => {
                            _.max_concurrent_streams = unit.value;
                        },
                        .MaxFrameSize => {
                            if (unit.value < (DEFAULT_MAX_FRAME_SIZE as u32) || unit.value > MAX_FRAME_SIZE) {
                                _.fatal_error = ErrorCode.ProtocolError;
                                return H2FrameAction{ kind: H2FrameActionKind.ProtocolError, error_code: ErrorCode.ProtocolError };
                            }
                            _.max_frame_size = unit.value;
                        },
                        .InitialWindowSize => {
                            if (unit.value > (MAX_WINDOW_SIZE as u32)) {
                                _.fatal_error = ErrorCode.FlowControlError;
                                return H2FrameAction{ kind: H2FrameActionKind.ProtocolError, error_code: ErrorCode.FlowControlError };
                            }
                            const delta: i64 = (unit.value as i64) - (_.flow.remote_initial_window_size as i64);
                            _.flow.remote_initial_window_size = unit.value;
                            _.peer_window_size = unit.value as i32;
                            if (s := stream) {
                                const next = (s.window_size as i64) + delta;
                                if (next > (MAX_WINDOW_SIZE as i64)) {
                                    _.fatal_error = ErrorCode.FlowControlError;
                                    return H2FrameAction{ kind: H2FrameActionKind.ProtocolError, error_code: ErrorCode.FlowControlError };
                                }
                                s.window_size = next as i32;
                            }
                        },
                        else => {},
                    }
                }
                _.settings_received = true;
                const rlen = H2ClientSession.buildSettingsAck(out_reply);
                return H2FrameAction{ kind: H2FrameActionKind.ReplyReady, reply_len: rlen };
            },
            .Ping => {
                if (header.stream_id != 0) {
                    _.fatal_error = ErrorCode.ProtocolError;
                    return H2FrameAction{ kind: H2FrameActionKind.ProtocolError, error_code: ErrorCode.ProtocolError };
                }
                if (header.length != 8) {
                    _.fatal_error = ErrorCode.FrameSizeError;
                    return H2FrameAction{ kind: H2FrameActionKind.ProtocolError, error_code: ErrorCode.FrameSizeError };
                }
                if ((header.flags & @intFromEnum(PingFrameFlags.Ack)) == 0) {
                    const pf = PingFrame.decode(payload[0..8]).?;
                    const rlen = H2ClientSession.buildPing(out_reply, pf.opaque_data, true);
                    return H2FrameAction{ kind: H2FrameActionKind.ReplyReady, reply_len: rlen };
                }
                return H2FrameAction{ kind: H2FrameActionKind.PingReceived };
            },
            .WindowUpdate => {
                if (header.length != 4) {
                    _.fatal_error = ErrorCode.FrameSizeError;
                    return H2FrameAction{ kind: H2FrameActionKind.ProtocolError, error_code: ErrorCode.FrameSizeError };
                }
                const wu = WindowUpdateFrame.decode(payload[0..4]).?;
                if (wu.increment == 0) {
                    _.fatal_error = ErrorCode.ProtocolError;
                    return H2FrameAction{ kind: H2FrameActionKind.ProtocolError, error_code: ErrorCode.ProtocolError, stream_id: header.stream_id };
                }
                if (header.stream_id == 0) {
                    if (!_.flow.creditConnSend(wu.increment)) {
                        _.fatal_error = ErrorCode.FlowControlError;
                        return H2FrameAction{ kind: H2FrameActionKind.ProtocolError, error_code: ErrorCode.FlowControlError };
                    }
                } else if (s := stream) {
                    if (!s.creditSend(wu.increment)) {
                        s.rst(ErrorCode.FlowControlError);
                        return H2FrameAction{ kind: H2FrameActionKind.StreamReset, error_code: ErrorCode.FlowControlError, stream_id: header.stream_id };
                    }
                }
                return H2FrameAction{ kind: H2FrameActionKind.WindowUpdated, stream_id: header.stream_id, window_increment: wu.increment };
            },
            .Goaway => {
                if (header.stream_id != 0 || header.length < 8) {
                    _.fatal_error = ErrorCode.ProtocolError;
                    return H2FrameAction{ kind: H2FrameActionKind.ProtocolError, error_code: ErrorCode.ProtocolError };
                }
                const gf = GoawayFrame.decode(payload).?;
                _.is_goaway = true;
                _.goaway_last_stream_id = gf.last_stream_id;
                _.goaway_error = gf.error_code;
                return H2FrameAction{ kind: H2FrameActionKind.GoawayReceived, stream_id: gf.last_stream_id, error_code: gf.error_code };
            },
            .RstStream => {
                if (header.stream_id == 0 || header.length != 4) {
                    _.fatal_error = ErrorCode.ProtocolError;
                    return H2FrameAction{ kind: H2FrameActionKind.ProtocolError, error_code: ErrorCode.ProtocolError };
                }
                const rf = RstStreamFrame.decode(payload[0..4]).?;
                if (s := stream) {
                    s.rst(rf.error_code);
                    if (_.active_streams > 0) _.active_streams -= 1;
                }
                return H2FrameAction{ kind: H2FrameActionKind.StreamReset, stream_id: header.stream_id, error_code: rf.error_code };
            },
            .Headers => {
                if (header.stream_id == 0) {
                    _.fatal_error = ErrorCode.ProtocolError;
                    return H2FrameAction{ kind: H2FrameActionKind.ProtocolError, error_code: ErrorCode.ProtocolError };
                }
                const end_stream = (header.flags & @intFromEnum(HeadersFrameFlags.EndStream)) != 0;
                const end_headers = (header.flags & @intFromEnum(HeadersFrameFlags.EndHeaders)) != 0;
                if (!end_headers) {
                    _.expecting_continuation = header.stream_id;
                    if (end_stream) {
                        if (s := stream) s.recvEndStream();
                    }
                    return H2FrameAction{ kind: H2FrameActionKind.None, stream_id: header.stream_id, end_stream: end_stream };
                }
                if (end_stream) {
                    if (s := stream) {
                        s.recvEndStream();
                        if (s.state == .Closed && _.active_streams > 0) _.active_streams -= 1;
                    }
                }
                return H2FrameAction{ kind: H2FrameActionKind.HeadersComplete, stream_id: header.stream_id, end_stream: end_stream };
            },
            .Continuation => {
                if (_.expecting_continuation == 0 || header.stream_id != _.expecting_continuation) {
                    _.fatal_error = ErrorCode.ProtocolError;
                    return H2FrameAction{ kind: H2FrameActionKind.ProtocolError, error_code: ErrorCode.ProtocolError };
                }
                const end_headers = (header.flags & @intFromEnum(HeadersFrameFlags.EndHeaders)) != 0;
                if (end_headers) {
                    _.expecting_continuation = 0;
                    return H2FrameAction{ kind: H2FrameActionKind.HeadersComplete, stream_id: header.stream_id };
                }
                return H2FrameAction{ kind: H2FrameActionKind.None, stream_id: header.stream_id };
            },
            .Data => {
                if (header.stream_id == 0) {
                    _.fatal_error = ErrorCode.ProtocolError;
                    return H2FrameAction{ kind: H2FrameActionKind.ProtocolError, error_code: ErrorCode.ProtocolError };
                }
                const end_stream = (header.flags & @intFromEnum(DataFrameFlags.EndStream)) != 0;
                if (s := stream) {
                    if (!_.flow.consumeRecv(s, header.length as u32)) {
                        _.fatal_error = ErrorCode.FlowControlError;
                        return H2FrameAction{ kind: H2FrameActionKind.ProtocolError, error_code: ErrorCode.FlowControlError, stream_id: header.stream_id };
                    }
                    if (end_stream) {
                        s.recvEndStream();
                        if (s.state == .Closed && _.active_streams > 0) _.active_streams -= 1;
                    }
                }
                return H2FrameAction{ kind: H2FrameActionKind.DataReceived, stream_id: header.stream_id, end_stream: end_stream };
            },
            else => return H2FrameAction{ kind: H2FrameActionKind.None, stream_id: header.stream_id },
        }
    }
}
