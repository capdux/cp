// Source: Bun path-not-recorded
// lib/http/signals.cp
// CP HTTP Atomic Signals & Flags (cap:http)
// Ported from Bun.js http/Signals.cp

const std = @import("std");

pub struct Signals {
    header_progress: (*std.atomic.Value(bool))? = null;
    response_body_streaming: (*std.atomic.Value(bool))? = null;
    aborted: (*std.atomic.Value(bool))? = null;
    cert_errors: (*std.atomic.Value(bool))? = null;
    upgraded: (*std.atomic.Value(bool))? = null;

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

    pub fn abort(*_) {
        if (ptr := _.aborted) {
            ptr.store(true, .monotonic);
        }
    }
}

pub struct SignalStore {
    header_progress: std.atomic.Value(bool) = std.atomic.Value(bool).init(false);
    response_body_streaming: std.atomic.Value(bool) = std.atomic.Value(bool).init(false);
    aborted: std.atomic.Value(bool) = std.atomic.Value(bool).init(false);
    cert_errors: std.atomic.Value(bool) = std.atomic.Value(bool).init(false);
    upgraded: std.atomic.Value(bool) = std.atomic.Value(bool).init(false);

    pub fn toSignals(*_): Signals {
        return Signals {
            header_progress: &_.header_progress,
            response_body_streaming: &_.response_body_streaming,
            aborted: &_.aborted,
            cert_errors: &_.cert_errors,
            upgraded: &_.upgraded,
        };
    }
}
