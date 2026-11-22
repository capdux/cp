// Source: Absorbed from /data/dev/bun/src/runtime/api/bun/subprocess/Readable.cp | Synced: 2026-11-22T19:48:30Z (Subprocess stdout/stderr Readable variant state machine with pipe-to-buffer transition, ref tracking & memory cost)
// Pure-CP zero-allocation subprocess stdout/stderr readable state machine modeling all 7 upstream Readable variants.

/// Maximum inline buffered capture capacity in `SubprocessReadable`.
pub const SUBPROCESS_READABLE_BUF_CAP: usize = 4096;

/// Estimated fixed overhead of an active pipe reader handle (used by `memoryCost()`).
pub const SUBPROCESS_PIPE_OVERHEAD_BYTES: usize = 64;

/// Active variant of a `SubprocessReadable` (aligned with Bun `subprocess/Readable.cp`).
pub enum SubprocessReadableKind: u8 {
    Fd = 0,
    Memfd = 1,
    Pipe = 2,
    Inherit = 3,
    Ignore = 4,
    Closed = 5,
    Buffer = 6,
}

/// Error set for `SubprocessReadable` operations.
pub err SubprocessReadableError {
    NotPipe,
    BufferOverflow,
    AlreadyClosed,
}

/// Zero-allocation subprocess stdout/stderr readable state machine.
pub struct SubprocessReadable {
    kind: SubprocessReadableKind = SubprocessReadableKind.Ignore,
    fd: i32 = -1,
    ref_active: bool = false,
    pending_activity: bool = false,
    max_bytes: usize = 0,
    buf: [SUBPROCESS_READABLE_BUF_CAP]u8 = [0; SUBPROCESS_READABLE_BUF_CAP],
    buf_len: usize = 0,
    truncated: bool = false,

    static fn initInherit(): SubprocessReadable {
        return SubprocessReadable{
            kind: SubprocessReadableKind.Inherit,
        };
    }

    static fn initIgnore(): SubprocessReadable {
        return SubprocessReadable{
            kind: SubprocessReadableKind.Ignore,
        };
    }

    static fn initFd(fd: i32): SubprocessReadable {
        return SubprocessReadable{
            kind: SubprocessReadableKind.Fd,
            fd: fd,
        };
    }

    static fn initMemfd(fd: i32): SubprocessReadable {
        return SubprocessReadable{
            kind: SubprocessReadableKind.Memfd,
            fd: fd,
        };
    }

    /// Initializes a buffered pipe readable with optional `max_bytes` cap (`0` = full `SUBPROCESS_READABLE_BUF_CAP`).
    static fn initPipe(fd: i32, max_bytes: usize): SubprocessReadable {
        return SubprocessReadable{
            kind: SubprocessReadableKind.Pipe,
            fd: fd,
            ref_active: true,
            pending_activity: true,
            max_bytes: max_bytes,
        };
    }

    /// Initializes a completed buffer readable directly from `data`.
    static fn initBuffer(data: []const u8): SubprocessReadable ! SubprocessReadableError {
        if (data.len > SUBPROCESS_READABLE_BUF_CAP) return SubprocessReadableError.BufferOverflow;
        let r = SubprocessReadable{
            kind: SubprocessReadableKind.Buffer,
            buf_len: data.len,
        };
        let i: usize = 0;
        while (i < data.len) {
            r.buf[i] = data[i];
            i += 1;
        }
        return r;
    }

    /// Returns tracked memory cost in bytes (aligned with Bun `Readable.memoryCost`).
    fn memoryCost(): usize {
        return switch (_.kind) {
            .Pipe => SUBPROCESS_PIPE_OVERHEAD_BYTES + _.buf_len,
            .Buffer => _.buf_len,
            else => 0,
        };
    }

    /// Returns true if this readable has pending asynchronous pipe activity (aligned with Bun `Readable.hasPendingActivity`).
    fn hasPendingActivity(): bool {
        return _.kind == SubprocessReadableKind.Pipe && _.pending_activity;
    }

    /// Marks the active pipe as keeping the event loop alive (no-op for non-pipe variants).
    fn ref(*_) {
        if (_.kind == SubprocessReadableKind.Pipe) {
            _.ref_active = true;
        }
    }

    /// Releases the event loop ref on an active pipe (no-op for non-pipe variants).
    fn unref(*_) {
        if (_.kind == SubprocessReadableKind.Pipe) {
            _.ref_active = false;
        }
    }

    /// Appends a chunk received from the pipe into the inline capture buffer.
    /// Enforces both `max_bytes` (if non-zero) and `SUBPROCESS_READABLE_BUF_CAP`.
    fn appendPipeChunk(*_, chunk: []const u8): usize ! SubprocessReadableError {
        if (_.kind == SubprocessReadableKind.Closed) return SubprocessReadableError.AlreadyClosed;
        if (_.kind != SubprocessReadableKind.Pipe) return SubprocessReadableError.NotPipe;

        const effective_cap = if (_.max_bytes > 0 && _.max_bytes < SUBPROCESS_READABLE_BUF_CAP)
            _.max_bytes
        else
            SUBPROCESS_READABLE_BUF_CAP;

        if (_.buf_len + chunk.len > effective_cap) {
            _.truncated = true;
            return SubprocessReadableError.BufferOverflow;
        }

        let i: usize = 0;
        while (i < chunk.len) {
            _.buf[_.buf_len + i] = chunk[i];
            i += 1;
        }
        _.buf_len += chunk.len;
        return _.buf_len;
    }

    /// Transitions a completed `.Pipe` readable into `.Buffer` (aligned with Bun buffered pipe EOF conversion).
    fn finishPipeToBuffer(*_): []const u8 ! SubprocessReadableError {
        if (_.kind == SubprocessReadableKind.Closed) return SubprocessReadableError.AlreadyClosed;
        if (_.kind != SubprocessReadableKind.Pipe) return SubprocessReadableError.NotPipe;

        _.kind = SubprocessReadableKind.Buffer;
        _.fd = -1;
        _.pending_activity = false;
        _.ref_active = false;
        return _.buf[0.._.buf_len];
    }

    /// Returns the captured bytes slice (`""` for non-buffered variants).
    fn slice(): []const u8 {
        if (_.kind == SubprocessReadableKind.Pipe || _.kind == SubprocessReadableKind.Buffer) {
            return _.buf[0.._.buf_len];
        }
        return "";
    }

    /// Closes this readable and transitions to `.Closed` (aligned with Bun `Readable.close` / `onClose`).
    fn close(*_) {
        _.kind = SubprocessReadableKind.Closed;
        _.fd = -1;
        _.pending_activity = false;
        _.ref_active = false;
        _.buf_len = 0;
    }

    fn isClosed(): bool {
        return _.kind == SubprocessReadableKind.Closed;
    }
}
