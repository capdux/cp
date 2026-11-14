/**
 * CP HTTP Atomic Signals & Flags (cap:http)
 * Ported from Bun.js http/Signals.cp
 */


pub const Signals = struct {
    header_progress: (*std.atomic.Value)?(bool) = null,
    response_body_streaming: (*std.atomic.Value)?(bool) = null,
    aborted: (*std.atomic.Value)?(bool) = null,
    cert_errors: (*std.atomic.Value)?(bool) = null,
    upgraded: (*std.atomic.Value)?(bool) = null,

    pub fn isAborted(): bool {
        if (ptr := _.aborted) {
            return ptr.load(.monotonic);
        }
        return false;
    }

    pub fn isStreaming(): bool {
        if (ptr := _.response_body_streaming) {
            return ptr.load(.monotonic);
        }
        return false;
    }

    pub fn isUpgraded(): bool {
        if (ptr := _.upgraded) {
            return ptr.load(.monotonic);
        }
        return false;
    }

    pub fn abort() {
        if (ptr := _.aborted) {
            ptr.store(true, .monotonic);
        }
    }
};

pub const SignalStore = struct {
    header_progress: std.atomic.Value(bool) = std.atomic.Value(bool).init(false),
    response_body_streaming: std.atomic.Value(bool) = std.atomic.Value(bool).init(false),
    aborted: std.atomic.Value(bool) = std.atomic.Value(bool).init(false),
    cert_errors: std.atomic.Value(bool) = std.atomic.Value(bool).init(false),
    upgraded: std.atomic.Value(bool) = std.atomic.Value(bool).init(false),

    pub fn toSignals(*_) Signals {
        let s = Signals{};
        s.header_progress = &_.header_progress;
        s.response_body_streaming = &_.response_body_streaming;
        s.aborted = &_.aborted;
        s.cert_errors = &_.cert_errors;
        s.upgraded = &_.upgraded;
        return s;
    }
};
