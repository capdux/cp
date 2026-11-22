// Source: Absorbed from /data/dev/bun/src/runtime/api/bun/subprocess/SubprocessPipeReader.cp | Synced: 2026-11-22T19:59:13Z (Subprocess buffered PipeReader state machine: Pending/Done/Errored lifecycle, ref-counting, MaxBuf cap, detach & stream/buffer consumption)
// Pure-CP zero-allocation subprocess buffered pipe reader state machine aligned with Bun `SubprocessPipeReader.cp`.

/// Maximum inline buffered capture capacity in `SubprocessPipeReader`.
pub const SUBPROCESS_PIPE_READER_BUF_CAP: usize = 4096;

/// Estimated fixed overhead of an active buffered pipe reader handle (used by `memoryCost()`).
pub const SUBPROCESS_PIPE_READER_OVERHEAD_BYTES: usize = 64;

/// Stdio stream role of a `SubprocessPipeReader` (aligned with Bun `PipeReader.kind`).
pub enum SubprocessPipeStreamKind: u8 {
    Stdout = 0,
    Stderr = 1,
}

/// Lifecycle state of `SubprocessPipeReader` (aligned with Bun `State`: pending / done / err).
pub enum SubprocessPipeReaderState: u8 {
    Pending = 0,
    Done = 1,
    Errored = 2,
}

/// Error set for `SubprocessPipeReader` operations.
pub err SubprocessPipeReaderError {
    AlreadyStarted,
    NotPending,
    NotDone,
    BufferOverflow,
    MaxBufExceeded,
    InvalidFd,
    AlreadyConsumed,
}

/// Zero-allocation subprocess buffered pipe reader state machine.
pub struct SubprocessPipeReader {
    stream_kind: SubprocessPipeStreamKind = SubprocessPipeStreamKind.Stdout,
    state: SubprocessPipeReaderState = SubprocessPipeReaderState.Pending,
    fd: i32 = -1,
    started: bool = false,
    watching: bool = false,
    attached: bool = false,
    ref_count: u32 = 1,
    loop_ref: bool = false,
    max_buf: usize = 0,
    err_code: i32 = 0,
    consumed: bool = false,
    buf: [SUBPROCESS_PIPE_READER_BUF_CAP]u8 = [0; SUBPROCESS_PIPE_READER_BUF_CAP],
    buf_len: usize = 0,

    /// Initializes a new subprocess pipe reader attached to `fd` with optional `max_buf` limit (`0` = no extra limit).
    static fn init(
        stream_kind: SubprocessPipeStreamKind,
        fd: i32,
        max_buf: usize,
    ): SubprocessPipeReader ! SubprocessPipeReaderError {
        if (fd < 0) return SubprocessPipeReaderError.InvalidFd;
        return SubprocessPipeReader{
            stream_kind: stream_kind,
            state: SubprocessPipeReaderState.Pending,
            fd: fd,
            attached: true,
            ref_count: 1,
            max_buf: max_buf,
        };
    }

    /// Increments the internal reference count.
    fn ref(*_) {
        _.ref_count += 1;
    }

    /// Decrements the internal reference count with saturation at 0 and returns the remaining count.
    fn deref(*_): u32 {
        if (_.ref_count > 0) {
            _.ref_count -= 1;
        }
        return _.ref_count;
    }

    /// Updates whether the underlying reader keeps the event loop alive (aligned with Bun `updateRef`).
    fn updateRef(*_, add: bool) {
        _.loop_ref = add;
    }

    /// Returns tracked memory cost in bytes (aligned with Bun `PipeReader.memoryCost`).
    fn memoryCost(): usize {
        if (_.state == SubprocessPipeReaderState.Errored || _.consumed) {
            return 0;
        }
        return SUBPROCESS_PIPE_READER_OVERHEAD_BYTES + _.buf_len;
    }

    /// Returns true if asynchronous reading is still pending (aligned with Bun `PipeReader.hasPendingActivity`).
    fn hasPendingActivity(): bool {
        return _.state == SubprocessPipeReaderState.Pending || _.watching;
    }

    /// Starts non-blocking reading on the pipe (aligned with Bun `PipeReader.start`).
    /// Acquires an active reader ref and enables event loop watching.
    fn start(*_): bool ! SubprocessPipeReaderError {
        if (_.started) return SubprocessPipeReaderError.AlreadyStarted;
        if (_.state != SubprocessPipeReaderState.Pending) return SubprocessPipeReaderError.NotPending;
        _.started = true;
        _.watching = true;
        _.loop_ref = true;
        _.ref();
        return true;
    }

    /// Resumes watching if the reader is still pending and started (aligned with Bun `PipeReader.watch`).
    fn watch(*_) {
        if (_.started && _.state == SubprocessPipeReaderState.Pending) {
            _.watching = true;
        }
    }

    /// Detaches from the parent subprocess and releases the attachment ref (idempotent).
    fn detach(*_) {
        if (_.attached) {
            _.attached = false;
            * = _.deref();
        }
    }

    /// Appends a chunk read from the pipe while in `.Pending` state.
    /// Enforces both `max_buf` (if non-zero) and `SUBPROCESS_PIPE_READER_BUF_CAP`.
    fn onReadChunk(*_, chunk: []const u8): usize ! SubprocessPipeReaderError {
        if (_.state != SubprocessPipeReaderState.Pending) return SubprocessPipeReaderError.NotPending;
        if (_.max_buf > 0 && _.buf_len + chunk.len > _.max_buf) {
            return SubprocessPipeReaderError.MaxBufExceeded;
        }
        if (_.buf_len + chunk.len > SUBPROCESS_PIPE_READER_BUF_CAP) {
            return SubprocessPipeReaderError.BufferOverflow;
        }
        let i: usize = 0;
        while (i < chunk.len) {
            _.buf[_.buf_len + i] = chunk[i];
            i += 1;
        }
        _.buf_len += chunk.len;
        return _.buf_len;
    }

    /// Completes reading on EOF (aligned with Bun `PipeReader.onReaderDone`).
    fn onReaderDone(*_) {
        if (_.state != SubprocessPipeReaderState.Pending) return;
        _.state = SubprocessPipeReaderState.Done;
        _.watching = false;
        _.loop_ref = false;
        _.fd = -1;
        _.detach();
    }

    /// Transitions to `.Errored` on I/O failure, discarding buffered bytes (aligned with Bun `PipeReader.onReaderError`).
    fn onReaderError(*_, err_code: i32) {
        _.buf_len = 0;
        _.err_code = err_code;
        _.state = SubprocessPipeReaderState.Errored;
        _.watching = false;
        _.loop_ref = false;
        _.fd = -1;
        _.detach();
    }

    /// Closes the pipe reader if still pending (aligned with Bun `PipeReader.close`).
    fn close(*_) {
        if (_.state == SubprocessPipeReaderState.Pending) {
            _.onReaderDone();
        }
    }

    /// Returns a read-only view of currently buffered bytes (`""` when errored or already consumed).
    fn peekSlice(): []const u8 {
        if (_.state == SubprocessPipeReaderState.Errored || _.consumed) {
            return "";
        }
        return _.buf[0.._.buf_len];
    }

    /// Consumes the completed buffer into `out` (aligned with Bun `PipeReader.toBuffer`).
    /// Only succeeds in `.Done` state and marks the reader as consumed to prevent double reads.
    fn takeBuffer(*_, out: []u8): usize ! SubprocessPipeReaderError {
        if (_.state != SubprocessPipeReaderState.Done) return SubprocessPipeReaderError.NotDone;
        if (_.consumed) return SubprocessPipeReaderError.AlreadyConsumed;
        if (out.len < _.buf_len) return SubprocessPipeReaderError.BufferOverflow;

        const n = _.buf_len;
        let i: usize = 0;
        while (i < n) {
            out[i] = _.buf[i];
            i += 1;
        }
        _.buf_len = 0;
        _.consumed = true;
        return n;
    }

    /// Detaches and consumes any buffered bytes for stream handoff (aligned with Bun `PipeReader.toReadableStream`).
    /// For `.Errored`, detaches and returns 0 (empty cancelled stream); for `.Pending`/`.Done`, copies bytes and marks consumed.
    fn takeStreamSnapshot(*_, out: []u8): usize ! SubprocessPipeReaderError {
        _.detach();
        if (_.consumed) return SubprocessPipeReaderError.AlreadyConsumed;

        if (_.state == SubprocessPipeReaderState.Errored) {
            _.consumed = true;
            return 0;
        }

        if (out.len < _.buf_len) return SubprocessPipeReaderError.BufferOverflow;
        const n = _.buf_len;
        let i: usize = 0;
        while (i < n) {
            out[i] = _.buf[i];
            i += 1;
        }
        _.buf_len = 0;
        _.watching = false;
        _.loop_ref = false;
        _.state = SubprocessPipeReaderState.Done;
        _.consumed = true;
        return n;
    }

    fn isTerminal(): bool {
        return _.state != SubprocessPipeReaderState.Pending;
    }
}
