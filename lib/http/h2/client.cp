// Source: Bun path-not-recorded
/**
 * CP HTTP/2 Client Session & Stream Manager (cap:http)
 * Ported from Bun.js http/H2Client.cp & http/h2_client/
 */

import { FrameType, FrameHeader, ErrorCode, SettingsType, DEFAULT_WINDOW_SIZE, DEFAULT_MAX_FRAME_SIZE } from "./frame";

pub const H2StreamState = enum {
    Idle,
    ReservedLocal,
    ReservedRemote,
    Open,
    HalfClosedLocal,
    HalfClosedRemote,
    Closed,
};

pub const H2Stream = struct {
    id: u32 = 0,
    state: H2StreamState = H2StreamState.Idle,
    window_size: i32 = DEFAULT_WINDOW_SIZE,
    rst_done: bool = false,

    pub fn isOpen(*_): bool {
        return _.state == .Open or _.state == .HalfClosedRemote;
    }

    pub fn rst(*this, code: ErrorCode) {
        if (_.rst_done or _.state == .Closed) return;
        _.rst_done = true;
        _.state = .Closed;
    }

    pub fn sentEndStream(*this) {
        _.state = switch (_.state) {
            .Open => .HalfClosedLocal,
            .HalfClosedRemote => .Closed,
            else => _.state,
        };
    }

    pub fn recvEndStream(*this) {
        _.state = switch (_.state) {
            .Open => .HalfClosedRemote,
            .HalfClosedLocal => .Closed,
            else => _.state,
        };
    }

    pub fn localClosed(*_): bool {
        return _.state == .HalfClosedLocal or _.state == .Closed;
    }

    pub fn remoteClosed(*_): bool {
        return _.state == .HalfClosedRemote or _.state == .Closed;
    }

    pub fn close(*_) {
        _.state = .Closed;
    }
};

pub const H2ClientSession = struct {
    next_stream_id: u32 = 1,
    peer_window_size: i32 = DEFAULT_WINDOW_SIZE,
    local_window_size: i32 = DEFAULT_WINDOW_SIZE,
    max_frame_size: u32 = DEFAULT_MAX_FRAME_SIZE,
    max_concurrent_streams: u32 = 100,
    active_streams: usize = 0,
    is_goaway: bool = false,

    pub fn newStream(*_) H2Stream {
        const stream_id = _.next_stream_id;
        _.next_stream_id += 2; // Client stream IDs are odd (1, 3, 5...)
        _.active_streams += 1;
        let s = H2Stream{};
        s.id = stream_id;
        s.state = .Open;
        s.window_size = _.peer_window_size;
        return s;
    }

    pub fn canCreateStream(*_): bool {
        return !_.is_goaway and (_.active_streams < _.max_concurrent_streams);
    }

    pub fn onGoaway(*_) {
        _.is_goaway = true;
    }
};
