// Source: /data/dev/bun/src/http/h2_client/ClientSession.cp
// lib/http/h2/client.cp
// CP HTTP/2 Client Session & Stream Manager (cap:http)

import { FrameType, FrameHeader, ErrorCode, SettingsType, DEFAULT_WINDOW_SIZE, DEFAULT_MAX_FRAME_SIZE } from "./frame";

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
}

pub struct H2ClientSession {
    pub next_stream_id: u32 = 1;
    pub peer_window_size: i32 = DEFAULT_WINDOW_SIZE;
    pub local_window_size: i32 = DEFAULT_WINDOW_SIZE;
    pub max_frame_size: u32 = DEFAULT_MAX_FRAME_SIZE;
    pub max_concurrent_streams: u32 = 100;
    pub active_streams: usize = 0;
    pub is_goaway: bool = false;

    pub fn newStream(*_): H2Stream {
        const stream_id = _.next_stream_id;
        _.next_stream_id += 2; // Client stream IDs are odd (1, 3, 5...)
        _.active_streams += 1;
        return H2Stream{
            id: stream_id,
            state: H2StreamState.Open,
            window_size: _.peer_window_size,
            rst_done: false,
            last_error: ErrorCode.NoError,
        };
    }

    pub fn canCreateStream(): bool {
        return !_.is_goaway && (_.active_streams < _.max_concurrent_streams);
    }

    pub fn onGoaway(*_) {
        _.is_goaway = true;
    }
}
