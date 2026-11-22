// Source: Absorbed from /data/dev/bun/src/http/h2_client/dispatch.cp | Synced: 2026-11-21T23:52:58Z
// Pure-CP zero-allocation RFC 7540 HTTP/2 Frame Protocol Validator & Connection Dispatcher:
// - Wire 9-byte frame header parser (with 31-bit stream ID masking)
// - PADDED flag stripping (`stripH2Padding`) & PRIORITY 5-byte skipping
// - Strict RFC 7540 state machine: initial SETTINGS enforcement, CONTINUATION
//   sequence lock, SETTINGS bounds checks, WINDOW_UPDATE 2^31-1 overflow guard,
//   PING/GOAWAY/RST_STREAM/HEADERS/DATA validation.

pub const H2_DISPATCH_HEADER_SIZE: usize = 9;
pub const H2_DISPATCH_DEFAULT_MAX_FRAME_SIZE: u32 = 16384;
pub const H2_DISPATCH_MAX_FRAME_SIZE_LIMIT: u32 = 16777215;
pub const H2_DISPATCH_DEFAULT_WINDOW_SIZE: u32 = 65535;
pub const H2_DISPATCH_MAX_WINDOW_SIZE: u32 = 0x7fffffff;
pub const H2_DISPATCH_DEFAULT_MAX_HEADER_LIST: u32 = 65536;

// RFC 7540 frame type codes
const FT_DATA: u8 = 0x0;
const FT_HEADERS: u8 = 0x1;
const FT_PRIORITY: u8 = 0x2;
const FT_RST_STREAM: u8 = 0x3;
const FT_SETTINGS: u8 = 0x4;
const FT_PING: u8 = 0x6;
const FT_GOAWAY: u8 = 0x7;
const FT_WINDOW_UPDATE: u8 = 0x8;
const FT_CONTINUATION: u8 = 0x9;

// RFC 7540 frame flags
const FLAG_ACK: u8 = 0x1;
const FLAG_END_HEADERS: u8 = 0x4;
const FLAG_PADDED: u8 = 0x8;
const FLAG_PRIORITY: u8 = 0x20;

// RFC 7540 SETTINGS parameter IDs
const SET_MAX_CONCURRENT_STREAMS: u16 = 0x3;
const SET_INITIAL_WINDOW_SIZE: u16 = 0x4;
const SET_MAX_FRAME_SIZE: u16 = 0x5;

/// HTTP/2 帧协议分发错误集（对齐 Bun `dispatch.cp` 致命错误）
pub err H2DispatchError {
    FrameSizeError,
    ProtocolError,
    FlowControlError,
    HeaderListTooLarge,
}

/// 分发后产出的结构化 HTTP/2 协议事件类型
pub enum H2DispatchEventKind: u8 {
    settings_applied = 0,
    settings_ack = 1,
    ping_received = 2,
    ping_ack = 3,
    window_update_conn = 4,
    window_update_stream = 5,
    goaway_received = 6,
    rst_stream_received = 7,
    headers_fragment = 8,
    continuation_fragment = 9,
    data_received = 10,
    priority_received = 11,
    unknown_frame = 12,
}

/// 解析后的 9 字节 HTTP/2 帧头
pub struct H2ParsedFrameHeader {
    length: u32,
    frame_type: u8,
    flags: u8,
    stream_id: u32,

    /// 从大端字节流解析 9 字节帧头（自动屏蔽 stream_id 最高保留位）
    static fn parse(buf: []const u8): H2ParsedFrameHeader? {
        if (buf.len < H2_DISPATCH_HEADER_SIZE) {
            return null;
        }
        const length: u32 = (@as(u32, buf[0]) << 16) |
            (@as(u32, buf[1]) << 8) |
            @as(u32, buf[2]);
        const frame_type = buf[3];
        const flags = buf[4];
        const raw_sid: u32 = (@as(u32, buf[5]) << 24) |
            (@as(u32, buf[6]) << 16) |
            (@as(u32, buf[7]) << 8) |
            @as(u32, buf[8]);
        return H2ParsedFrameHeader{
            length: length,
            frame_type: frame_type,
            flags: flags,
            stream_id: raw_sid & 0x7fffffff,
        };
    }
}

/// 剥离 DATA / HEADERS 帧的 `PADDED (0x8)` 填充字节；长度非法返回 `null`
pub fn stripH2Padding(payload: []const u8): ([]const u8)? {
    if (payload.len == 0) {
        return null;
    }
    const pad_len: usize = @as(usize, payload[0]);
    if (1 + pad_len > payload.len) {
        return null;
    }
    return payload[1 .. payload.len - pad_len];
}

fn readBeU16(buf: []const u8, off: usize): u16 {
    return (@as(u16, buf[off]) << 8) | @as(u16, buf[off + 1]);
}

fn readBeU32(buf: []const u8, off: usize): u32 {
    return (@as(u32, buf[off]) << 24) |
        (@as(u32, buf[off + 1]) << 16) |
        (@as(u32, buf[off + 2]) << 8) |
        @as(u32, buf[off + 3]);
}

/// 单帧分发结果（零堆分配，切片零拷贝指向原始帧缓冲区）
pub struct H2DispatchEvent {
    kind: H2DispatchEventKind,
    stream_id: u32 = 0,
    flags: u8 = 0,
    error_code: u32 = 0,
    window_increment: u32 = 0,
    last_stream_id: u32 = 0,
    ping_data: [8]u8 = [0; 8],
    payload_slice: []const u8 = "",
    needs_settings_ack: bool = false,
    needs_ping_ack: bool = false,
}

/// 连接级 HTTP/2 帧协议校验与分发状态机（对齐 Bun `h2_client/dispatch.cp`）
pub struct H2FrameDispatcher {
    settings_received: bool = false,
    expecting_continuation: u32 = 0,
    local_max_frame_size: u32 = H2_DISPATCH_DEFAULT_MAX_FRAME_SIZE,
    remote_max_frame_size: u32 = H2_DISPATCH_DEFAULT_MAX_FRAME_SIZE,
    remote_initial_window_size: u32 = H2_DISPATCH_DEFAULT_WINDOW_SIZE,
    remote_max_concurrent_streams: u32 = 100,
    conn_send_window: i64 = 65535,
    local_max_header_list_size: u32 = H2_DISPATCH_DEFAULT_MAX_HEADER_LIST,
    goaway_received: bool = false,
    goaway_last_stream_id: u32 = 0,
    goaway_error_code: u32 = 0,

    static fn init(): H2FrameDispatcher {
        return H2FrameDispatcher{
            settings_received: false,
            expecting_continuation: 0,
            local_max_frame_size: H2_DISPATCH_DEFAULT_MAX_FRAME_SIZE,
            remote_max_frame_size: H2_DISPATCH_DEFAULT_MAX_FRAME_SIZE,
            remote_initial_window_size: H2_DISPATCH_DEFAULT_WINDOW_SIZE,
            remote_max_concurrent_streams: 100,
            conn_send_window: @as(i64, H2_DISPATCH_DEFAULT_WINDOW_SIZE),
            local_max_header_list_size: H2_DISPATCH_DEFAULT_MAX_HEADER_LIST,
            goaway_received: false,
            goaway_last_stream_id: 0,
            goaway_error_code: 0,
        };
    }

    /// 校验并分发单个完整 HTTP/2 帧
    fn dispatchOne(*_, header: H2ParsedFrameHeader, payload: []const u8): H2DispatchEvent! {
        if (header.length > _.local_max_frame_size || payload.len != @as(usize, header.length)) {
            return err.FrameSizeError;
        }
        if (_.expecting_continuation != 0) {
            if (header.frame_type != FT_CONTINUATION || header.stream_id != _.expecting_continuation) {
                return err.ProtocolError;
            }
        }
        if (!_.settings_received && header.frame_type != FT_SETTINGS) {
            return err.ProtocolError;
        }

        if (header.frame_type == FT_SETTINGS) {
            if (header.stream_id != 0) {
                return err.ProtocolError;
            }
            if ((header.flags & FLAG_ACK) != 0) {
                if (header.length != 0) {
                    return err.FrameSizeError;
                }
                return H2DispatchEvent{ kind: .settings_ack, flags: header.flags };
            }
            if ((header.length % 6) != 0) {
                return err.FrameSizeError;
            }
            let off: usize = 0;
            while (off + 6 <= payload.len) {
                const param_id = readBeU16(payload, off);
                const val = readBeU32(payload, off + 2);
                if (param_id == SET_MAX_FRAME_SIZE) {
                    if (val < H2_DISPATCH_DEFAULT_MAX_FRAME_SIZE || val > H2_DISPATCH_MAX_FRAME_SIZE_LIMIT) {
                        return err.ProtocolError;
                    }
                    _.remote_max_frame_size = val;
                } else if (param_id == SET_INITIAL_WINDOW_SIZE) {
                    if (val > H2_DISPATCH_MAX_WINDOW_SIZE) {
                        return err.FlowControlError;
                    }
                    _.remote_initial_window_size = val;
                } else if (param_id == SET_MAX_CONCURRENT_STREAMS) {
                    _.remote_max_concurrent_streams = val;
                }
                off += 6;
            }
            _.settings_received = true;
            return H2DispatchEvent{
                kind: .settings_applied,
                flags: header.flags,
                needs_settings_ack: true,
            };
        }

        if (header.frame_type == FT_WINDOW_UPDATE) {
            if (header.length != 4) {
                return err.FrameSizeError;
            }
            const inc = readBeU32(payload, 0) & 0x7fffffff;
            if (inc == 0) {
                return err.ProtocolError;
            }
            if (header.stream_id == 0) {
                const next = _.conn_send_window + @as(i64, inc);
                if (next > @as(i64, H2_DISPATCH_MAX_WINDOW_SIZE)) {
                    return err.FlowControlError;
                }
                _.conn_send_window = next;
                return H2DispatchEvent{
                    kind: .window_update_conn,
                    stream_id: 0,
                    window_increment: inc,
                };
            }
            return H2DispatchEvent{
                kind: .window_update_stream,
                stream_id: header.stream_id,
                window_increment: inc,
            };
        }

        if (header.frame_type == FT_PING) {
            if (header.length != 8) {
                return err.FrameSizeError;
            }
            if (header.stream_id != 0) {
                return err.ProtocolError;
            }
            let ev = H2DispatchEvent{
                kind: if ((header.flags & FLAG_ACK) != 0) .ping_ack else .ping_received,
                flags: header.flags,
                needs_ping_ack: (header.flags & FLAG_ACK) == 0,
            };
            let k: usize = 0;
            while (k < 8) {
                ev.ping_data[k] = payload[k];
                k += 1;
            }
            return ev;
        }

        if (header.frame_type == FT_PRIORITY) {
            if (header.stream_id == 0) {
                return err.ProtocolError;
            }
            if (header.length != 5) {
                return err.FrameSizeError;
            }
            return H2DispatchEvent{
                kind: .priority_received,
                stream_id: header.stream_id,
                payload_slice: payload,
            };
        }

        if (header.frame_type == FT_RST_STREAM) {
            if (header.stream_id == 0) {
                return err.ProtocolError;
            }
            if (header.length != 4) {
                return err.FrameSizeError;
            }
            return H2DispatchEvent{
                kind: .rst_stream_received,
                stream_id: header.stream_id,
                error_code: readBeU32(payload, 0),
            };
        }

        if (header.frame_type == FT_GOAWAY) {
            if (header.stream_id != 0) {
                return err.ProtocolError;
            }
            if (header.length < 8) {
                return err.FrameSizeError;
            }
            const last_sid = readBeU32(payload, 0) & 0x7fffffff;
            const ec = readBeU32(payload, 4);
            _.goaway_received = true;
            _.goaway_last_stream_id = last_sid;
            _.goaway_error_code = ec;
            return H2DispatchEvent{
                kind: .goaway_received,
                last_stream_id: last_sid,
                error_code: ec,
                payload_slice: payload[8..],
            };
        }

        if (header.frame_type == FT_HEADERS) {
            if (header.stream_id == 0) {
                return err.ProtocolError;
            }
            let frag = payload;
            if ((header.flags & FLAG_PADDED) != 0) {
                if (stripped := stripH2Padding(frag)) {
                    frag = stripped;
                } else {
                    return err.ProtocolError;
                }
            }
            if ((header.flags & FLAG_PRIORITY) != 0) {
                if (frag.len < 5) {
                    return err.ProtocolError;
                }
                frag = frag[5..];
            }
            if (frag.len > @as(usize, _.local_max_header_list_size)) {
                return err.HeaderListTooLarge;
            }
            if ((header.flags & FLAG_END_HEADERS) == 0) {
                _.expecting_continuation = header.stream_id;
            }
            return H2DispatchEvent{
                kind: .headers_fragment,
                stream_id: header.stream_id,
                flags: header.flags,
                payload_slice: frag,
            };
        }

        if (header.frame_type == FT_CONTINUATION) {
            if (header.stream_id == 0 || _.expecting_continuation != header.stream_id) {
                return err.ProtocolError;
            }
            if (payload.len > @as(usize, _.local_max_header_list_size)) {
                return err.HeaderListTooLarge;
            }
            if ((header.flags & FLAG_END_HEADERS) != 0) {
                _.expecting_continuation = 0;
            }
            return H2DispatchEvent{
                kind: .continuation_fragment,
                stream_id: header.stream_id,
                flags: header.flags,
                payload_slice: payload,
            };
        }

        if (header.frame_type == FT_DATA) {
            if (header.stream_id == 0) {
                return err.ProtocolError;
            }
            let body = payload;
            if ((header.flags & FLAG_PADDED) != 0) {
                if (stripped := stripH2Padding(body)) {
                    body = stripped;
                } else {
                    return err.ProtocolError;
                }
            }
            return H2DispatchEvent{
                kind: .data_received,
                stream_id: header.stream_id,
                flags: header.flags,
                payload_slice: body,
            };
        }

        // RFC 7540 §4.1: unknown extension frame types are ignored
        return H2DispatchEvent{
            kind: .unknown_frame,
            stream_id: header.stream_id,
            flags: header.flags,
            payload_slice: payload,
        };
    }

    /// 从连续字节流解析并分发下一帧；若数据不足一帧返回 `null`（`consumed_out.* = 0`）
    fn parseNext(*_, buf: []const u8, consumed_out: *usize): H2DispatchEvent?! {
        consumed_out.* = 0;
        const hdr = H2ParsedFrameHeader.parse(buf) ?? return null;
        if (hdr.length > _.local_max_frame_size) {
            return err.FrameSizeError;
        }
        const total = H2_DISPATCH_HEADER_SIZE + @as(usize, hdr.length);
        if (buf.len < total) {
            return null;
        }
        const ev = _.dispatchOne(hdr, buf[H2_DISPATCH_HEADER_SIZE..total])?;
        consumed_out.* = total;
        return ev;
    }
}
