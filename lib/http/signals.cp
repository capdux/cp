// Source: Absorbed from /data/dev/bun/src/http/Signals.cp | Synced: 2026-11-22T11:12:13Z
// lib/http/signals.cp
// CP HTTP Atomic Signals & Flags (cap:http)

const std = @import("std");

pub struct Signals {
    pub header_progress: (*std.atomic.Value(bool))? = null;
    pub response_body_streaming: (*std.atomic.Value(bool))? = null;
    pub aborted: (*std.atomic.Value(bool))? = null;
    pub cert_errors: (*std.atomic.Value(bool))? = null;
    pub upgraded: (*std.atomic.Value(bool))? = null;

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
}

pub struct SignalStore {
    pub header_progress: std.atomic.Value(bool) = std.atomic.Value(bool).init(false);
    pub response_body_streaming: std.atomic.Value(bool) = std.atomic.Value(bool).init(false);
    pub aborted: std.atomic.Value(bool) = std.atomic.Value(bool).init(false);
    pub cert_errors: std.atomic.Value(bool) = std.atomic.Value(bool).init(false);
    pub upgraded: std.atomic.Value(bool) = std.atomic.Value(bool).init(false);

    pub fn toSignals(*_): Signals {
        return Signals{
            header_progress: &_.header_progress,
            response_body_streaming: &_.response_body_streaming,
            aborted: &_.aborted,
            cert_errors: &_.cert_errors,
            upgraded: &_.upgraded,
        };
    }
}
