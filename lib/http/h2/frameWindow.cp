// Source: cp/lib/http2_frame/window.cp (RFC 9113 HTTP/2 Stream State Machine & Flow Control Window)
const std = @import("std");
import {
    H2_DEFAULT_INITIAL_WINDOW_SIZE,
    H2_MAX_WINDOW_SIZE,
    H2Error,
} from "./frameTypes.cp";

pub enum H2StreamState {
    idle,
    open,
    half_closed_local,
    half_closed_remote,
    closed,
}

pub struct H2StreamInfo {
    pub id: u32,
    pub state: H2StreamState,
    pub send_window: i64,
    pub recv_window: i64,
}

pub struct H2ConnectionWindow {
    alloc: std.mem.Allocator,
    pub conn_send_window: i64,
    pub conn_recv_window: i64,
    pub initial_stream_window: i64,
    streams: std.AutoHashMap(u32, H2StreamInfo),

    pub static fn init(alloc: std.mem.Allocator, initial_window: i64): H2Error!H2ConnectionWindow {
        if (initial_window < 0 or initial_window > H2_MAX_WINDOW_SIZE) {
            return H2Error.InvalidSettings;
        }
        return H2ConnectionWindow{
            .alloc = alloc,
            .conn_send_window = H2_DEFAULT_INITIAL_WINDOW_SIZE,
            .conn_recv_window = H2_DEFAULT_INITIAL_WINDOW_SIZE,
            .initial_stream_window = initial_window,
            .streams = std.AutoHashMap(u32, H2StreamInfo).init(alloc),
        };
    }

    pub fn deinit(*_) {
        _.streams.deinit();
    }

    pub fn openStream(*_, stream_id: u32): H2Error!void {
        if (stream_id == 0 or (stream_id & 0x80000000) != 0) {
            return H2Error.InvalidStreamId;
        }
        if (_.streams.get(stream_id) != null) {
            return;
        }
        _.streams.put(stream_id, H2StreamInfo{
            .id = stream_id,
            .state = .open,
            .send_window = _.initial_stream_window,
            .recv_window = _.initial_stream_window,
        }) catch return H2Error.OutOfMemory;
    }

    pub fn getStreamState(stream_id: u32): H2StreamState {
        if (info := _.streams.get(stream_id)) {
            return info.state;
        }
        return .idle;
    }

    pub fn transitionOnSendHeaders(*_, stream_id: u32, end_stream: bool): H2Error!void {
        if (stream_id == 0 or (stream_id & 0x80000000) != 0) {
            return H2Error.InvalidStreamId;
        }
        const cur = _.getStreamState(stream_id);
        switch (cur) {
            .idle => {
                try _.openStream(stream_id);
                if (end_stream) {
                    if (ptr := _.streams.getPtr(stream_id)) {
                        ptr.state = .half_closed_local;
                    }
                }
            },
            .open => {
                if (end_stream) {
                    if (ptr := _.streams.getPtr(stream_id)) {
                        ptr.state = .half_closed_local;
                    }
                }
            },
            .half_closed_remote => {
                if (end_stream) {
                    if (ptr := _.streams.getPtr(stream_id)) {
                        ptr.state = .closed;
                    }
                }
            },
            .half_closed_local, .closed => {
                return H2Error.InvalidStreamState;
            },
        }
    }

    pub fn transitionOnRecvHeaders(*_, stream_id: u32, end_stream: bool): H2Error!void {
        if (stream_id == 0 or (stream_id & 0x80000000) != 0) {
            return H2Error.InvalidStreamId;
        }
        const cur = _.getStreamState(stream_id);
        switch (cur) {
            .idle => {
                try _.openStream(stream_id);
                if (end_stream) {
                    if (ptr := _.streams.getPtr(stream_id)) {
                        ptr.state = .half_closed_remote;
                    }
                }
            },
            .open => {
                if (end_stream) {
                    if (ptr := _.streams.getPtr(stream_id)) {
                        ptr.state = .half_closed_remote;
                    }
                }
            },
            .half_closed_local => {
                if (end_stream) {
                    if (ptr := _.streams.getPtr(stream_id)) {
                        ptr.state = .closed;
                    }
                }
            },
            .half_closed_remote, .closed => {
                return H2Error.InvalidStreamState;
            },
        }
    }

    pub fn transitionOnSendEndStream(*_, stream_id: u32): H2Error!void {
        let ptr = _.streams.getPtr(stream_id) orelse return H2Error.InvalidStreamState;
        switch (ptr.state) {
            .open => ptr.state = .half_closed_local,
            .half_closed_remote => ptr.state = .closed,
            else => return H2Error.InvalidStreamState,
        }
    }

    pub fn transitionOnRecvEndStream(*_, stream_id: u32): H2Error!void {
        let ptr = _.streams.getPtr(stream_id) orelse return H2Error.InvalidStreamState;
        switch (ptr.state) {
            .open => ptr.state = .half_closed_remote,
            .half_closed_local => ptr.state = .closed,
            else => return H2Error.InvalidStreamState,
        }
    }

    pub fn resetStream(*_, stream_id: u32): H2Error!void {
        if (stream_id == 0 or (stream_id & 0x80000000) != 0) {
            return H2Error.InvalidStreamId;
        }
        if (ptr := _.streams.getPtr(stream_id)) {
            ptr.state = .closed;
        } else {
            _.streams.put(stream_id, H2StreamInfo{
                .id = stream_id,
                .state = .closed,
                .send_window = _.initial_stream_window,
                .recv_window = _.initial_stream_window,
            }) catch return H2Error.OutOfMemory;
        }
    }

    pub fn consumeSendWindow(*_, stream_id: u32, bytes: u32): H2Error!void {
        if (stream_id == 0) {
            return H2Error.InvalidStreamId;
        }
        let ptr = _.streams.getPtr(stream_id) orelse return H2Error.InvalidStreamState;
        if (ptr.state != .open and ptr.state != .half_closed_remote) {
            return H2Error.InvalidStreamState;
        }
        const delta: i64 = @as(i64, bytes);
        if (_.conn_send_window < delta or ptr.send_window < delta) {
            return H2Error.FlowControlError;
        }
        _.conn_send_window -= delta;
        ptr.send_window -= delta;
    }

    pub fn consumeRecvWindow(*_, stream_id: u32, bytes: u32): H2Error!void {
        if (stream_id == 0) {
            return H2Error.InvalidStreamId;
        }
        let ptr = _.streams.getPtr(stream_id) orelse return H2Error.InvalidStreamState;
        if (ptr.state != .open and ptr.state != .half_closed_local) {
            return H2Error.InvalidStreamState;
        }
        const delta: i64 = @as(i64, bytes);
        if (_.conn_recv_window < delta or ptr.recv_window < delta) {
            return H2Error.FlowControlError;
        }
        _.conn_recv_window -= delta;
        ptr.recv_window -= delta;
    }

    pub fn applyWindowUpdate(*_, stream_id: u32, increment: u32): H2Error!void {
        if (increment == 0 or increment > @as(u32, @intCast(H2_MAX_WINDOW_SIZE))) {
            return H2Error.InvalidIncrement;
        }
        const delta: i64 = @as(i64, increment);
        if (stream_id == 0) {
            const next = _.conn_send_window + delta;
            if (next > H2_MAX_WINDOW_SIZE) {
                return H2Error.FlowControlError;
            }
            _.conn_send_window = next;
            return;
        }
        let ptr = _.streams.getPtr(stream_id) orelse return H2Error.InvalidStreamState;
        const next = ptr.send_window + delta;
        if (next > H2_MAX_WINDOW_SIZE) {
            return H2Error.FlowControlError;
        }
        ptr.send_window = next;
    }

    pub fn getSendWindow(stream_id: u32): i64 {
        if (stream_id == 0) {
            return _.conn_send_window;
        }
        if (info := _.streams.get(stream_id)) {
            return info.send_window;
        }
        return _.initial_stream_window;
    }

    pub fn getRecvWindow(stream_id: u32): i64 {
        if (stream_id == 0) {
            return _.conn_recv_window;
        }
        if (info := _.streams.get(stream_id)) {
            return info.recv_window;
        }
        return _.initial_stream_window;
    }
}
