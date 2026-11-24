// Source: Absorbed from /data/dev/bun/src/runtime/api/bun/subprocess/StaticPipeWriter.cp | Synced: 2026-11-23T22:07:22Z (Zero-alloc subprocess static stdin pipe writer state machine with partial-write tracking & ref lifecycle)
// Pure-CP zero-allocation static-slice non-blocking pipe writer state machine with partial-write advancement and source detach lifecycle.

/// Lifecycle state of a `StaticPipeWriter`.
pub enum StaticPipeState: u8 {
    Idle = 0,
    Writing = 1,
    Closed = 2,
    Errored = 3,
}

/// Write completion hint passed to `StaticPipeWriter.onWrite` (aligned with Bun `io.WriteStatus`).
pub enum StaticPipeWriteStatus: u8 {
    Pending = 0,
    Drained = 1,
    EndOfFile = 2,
}

/// Error set for `StaticPipeWriter` state transitions.
pub err StaticPipeWriterError {
    AlreadyStarted,
    NotWriting,
    InvalidState,
}

/// Step result returned by `start()` and `onWrite()`.
pub struct StaticPipeStep {
    bytes_advanced: usize = 0,
    remaining_bytes: usize = 0,
    should_close: bool = false,
    state: StaticPipeState = StaticPipeState.Idle,
}

/// Zero-allocation non-blocking static-buffer pipe writer state machine (aligned with Bun `StaticPipeWriter`).
pub struct StaticPipeWriter {
    source: []const u8 = "",
    buffer: []const u8 = "",
    state: StaticPipeState = StaticPipeState.Idle,
    ref_count: u32 = 1,
    bytes_written: usize = 0,
    write_calls: usize = 0,
    last_errno: i32 = 0,
    source_detached: bool = false,

    /// Initializes a new writer bound to `source` with `ref_count = 1`.
    static fn init(source: []const u8): StaticPipeWriter {
        return StaticPipeWriter{
            source: source,
            buffer: "",
            state: StaticPipeState.Idle,
            ref_count: 1,
        };
    }

    /// Increments the internal reference count (saturating).
    fn ref(*_) {
        _.ref_count +|= 1;
    }

    /// Decrements the internal reference count and returns true if it reached zero.
    fn deref(*_): bool {
        if (_.ref_count > 0) {
            _.ref_count -= 1;
        }
        return _.ref_count == 0;
    }

    /// Returns the remaining unsent slice (aligned with Bun `StaticPipeWriter.getBuffer`).
    fn getBuffer(): []const u8 {
        return _.buffer;
    }

    /// Returns the number of bytes remaining to be written.
    fn remaining(): usize {
        return _.buffer.len;
    }

    /// Returns the total byte length of the original source slice.
    fn totalBytes(): usize {
        return _.source.len;
    }

    /// Returns write completion progress in permille (`0..=1000`). Empty sources return `1000`.
    fn progressPermille(): u16 {
        if (_.source.len == 0) return 1000;
        const p = (@as(u128, _.bytes_written) * 1000) / @as(u128, _.source.len);
        if (p >= 1000) return 1000;
        return @as(u16, @intCast(p));
    }

    /// Returns true if the writer has reached a terminal state (`Closed` or `Errored`).
    fn isTerminal(): bool {
        return _.state == StaticPipeState.Closed || _.state == StaticPipeState.Errored;
    }

    /// Starts streaming `source` (aligned with Bun `StaticPipeWriter.start`).
    /// Increments ref count; if `source` is empty, immediately closes and signals `should_close = true`.
    fn start(*_): StaticPipeStep ! StaticPipeWriterError {
        if (_.state != StaticPipeState.Idle) return StaticPipeWriterError.AlreadyStarted;
        _.ref();
        _.buffer = _.source;
        if (_.buffer.len == 0) {
            _.close();
            return StaticPipeStep{
                bytes_advanced: 0,
                remaining_bytes: 0,
                should_close: true,
                state: StaticPipeState.Closed,
            };
        }
        _.state = StaticPipeState.Writing;
        return StaticPipeStep{
            bytes_advanced: 0,
            remaining_bytes: _.buffer.len,
            should_close: false,
            state: StaticPipeState.Writing,
        };
    }

    /// Advances the remaining buffer by `min(amount, buffer.len)` (aligned with Bun `StaticPipeWriter.onWrite`).
    /// Automatically closes the writer when `status == EndOfFile` or all bytes have been written.
    fn onWrite(*_, amount: usize, status: StaticPipeWriteStatus): StaticPipeStep ! StaticPipeWriterError {
        if (_.state != StaticPipeState.Writing) return StaticPipeWriterError.NotWriting;
        const advanced = if (amount < _.buffer.len) amount else _.buffer.len;
        _.buffer = _.buffer[advanced.._.buffer.len];
        _.bytes_written +|= advanced;
        _.write_calls +|= 1;

        if (status == StaticPipeWriteStatus.EndOfFile || _.buffer.len == 0) {
            _.close();
            return StaticPipeStep{
                bytes_advanced: advanced,
                remaining_bytes: 0,
                should_close: true,
                state: StaticPipeState.Closed,
            };
        }

        return StaticPipeStep{
            bytes_advanced: advanced,
            remaining_bytes: _.buffer.len,
            should_close: false,
            state: StaticPipeState.Writing,
        };
    }

    /// Zero-allocation helper that copies up to `dst.len` pending bytes into `dst` and advances via `onWrite`.
    fn drainToBuf(*_, dst: []u8): usize ! StaticPipeWriterError {
        if (_.state != StaticPipeState.Writing) return StaticPipeWriterError.NotWriting;
        const n = if (dst.len < _.buffer.len) dst.len else _.buffer.len;
        let i: usize = 0;
        while (i < n) {
            dst[i] = _.buffer[i];
            i += 1;
        }
        const status = if (n == _.buffer.len) StaticPipeWriteStatus.Drained else StaticPipeWriteStatus.Pending;
        const step = _.onWrite(n, status)?;
        return step.bytes_advanced;
    }

    /// Records a write error, detaches the backing source, and transitions to `Errored` (aligned with Bun `onError`).
    fn onError(*_, errno_code: i32) {
        _.last_errno = errno_code;
        _.source_detached = true;
        _.buffer = "";
        _.state = StaticPipeState.Errored;
    }

    /// Closes the writer and detaches the backing source (aligned with Bun `onClose`).
    fn close(*_) {
        _.source_detached = true;
        _.buffer = "";
        if (_.state != StaticPipeState.Errored) {
            _.state = StaticPipeState.Closed;
        }
    }
}
