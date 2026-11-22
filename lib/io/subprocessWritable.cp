// Source: Absorbed from /data/dev/bun/src/runtime/api/bun/subprocess/Writable.cp | Synced: 2026-11-22T19:55:19Z (Subprocess stdin Writable 6-variant state machine with pipe/buffer/fd/memfd/inherit/ignore lifecycle, ref tracking, close/finalize transitions & memory cost)
// Pure-CP zero-allocation subprocess stdin writable state machine modeling all 6 upstream Writable variants.

/// Maximum inline write buffer capacity in `SubprocessWritable`.
pub const SUBPROCESS_WRITABLE_BUF_CAP: usize = 4096;

/// Estimated fixed overhead of an active pipe sink handle (used by `memoryCost()`).
pub const SUBPROCESS_WRITABLE_PIPE_OVERHEAD_BYTES: usize = 64;

/// Estimated fixed overhead of an active static buffer writer handle (used by `memoryCost()`).
pub const SUBPROCESS_WRITABLE_BUFFER_OVERHEAD_BYTES: usize = 48;

/// Active variant of a `SubprocessWritable` (aligned with Bun `subprocess/Writable.cp`).
pub enum SubprocessWritableKind: u8 {
    Pipe = 0,
    Fd = 1,
    Buffer = 2,
    Memfd = 3,
    Inherit = 4,
    Ignore = 5,
}

/// Error set for `SubprocessWritable` operations.
pub err SubprocessWritableError {
    NotPipe,
    NotWritable,
    BufferOverflow,
    AlreadyEnded,
    InvalidFd,
}

/// Zero-allocation subprocess stdin writable state machine.
pub struct SubprocessWritable {
    kind: SubprocessWritableKind = SubprocessWritableKind.Ignore,
    fd: i32 = -1,
    ref_active: bool = false,
    subprocess_ref_held: bool = false,
    detached_to_caller: bool = false,
    ended: bool = false,
    flushed_bytes: usize = 0,
    buf: [SUBPROCESS_WRITABLE_BUF_CAP]u8 = [0; SUBPROCESS_WRITABLE_BUF_CAP],
    buf_len: usize = 0,
    buf_pos: usize = 0,

    static fn initInherit(): SubprocessWritable {
        return SubprocessWritable{
            kind: SubprocessWritableKind.Inherit,
        };
    }

    static fn initIgnore(): SubprocessWritable {
        return SubprocessWritable{
            kind: SubprocessWritableKind.Ignore,
        };
    }

    static fn initFd(fd: i32): SubprocessWritable ! SubprocessWritableError {
        if (fd < 0) return SubprocessWritableError.InvalidFd;
        return SubprocessWritable{
            kind: SubprocessWritableKind.Fd,
            fd: fd,
        };
    }

    static fn initMemfd(fd: i32): SubprocessWritable ! SubprocessWritableError {
        if (fd < 0) return SubprocessWritableError.InvalidFd;
        return SubprocessWritable{
            kind: SubprocessWritableKind.Memfd,
            fd: fd,
        };
    }

    /// Initializes an interactive writable pipe sink (aligned with Bun `Writable.init` for `.pipe`).
    /// Acquires both the event loop ref and the subprocess lifecycle guard ref.
    static fn initPipe(fd: i32): SubprocessWritable ! SubprocessWritableError {
        if (fd < 0) return SubprocessWritableError.InvalidFd;
        return SubprocessWritable{
            kind: SubprocessWritableKind.Pipe,
            fd: fd,
            ref_active: true,
            subprocess_ref_held: true,
        };
    }

    /// Initializes a static buffer writer fed into stdin (aligned with Bun `.blob` / `.array_buffer` -> `StaticPipeWriter`).
    static fn initBuffer(data: []const u8): SubprocessWritable ! SubprocessWritableError {
        if (data.len > SUBPROCESS_WRITABLE_BUF_CAP) return SubprocessWritableError.BufferOverflow;
        let w = SubprocessWritable{
            kind: SubprocessWritableKind.Buffer,
            ref_active: true,
            buf_len: data.len,
        };
        let i: usize = 0;
        while (i < data.len) {
            w.buf[i] = data[i];
            i += 1;
        }
        return w;
    }

    /// Returns remaining buffered bytes not yet flushed to the child stdin.
    fn pendingBytes(): usize {
        if (_.kind != SubprocessWritableKind.Pipe && _.kind != SubprocessWritableKind.Buffer) {
            return 0;
        }
        return _.buf_len - _.buf_pos;
    }

    /// Returns a read-only view of buffered bytes awaiting flush (`""` for non-buffered variants).
    fn pendingSlice(): []const u8 {
        if (_.kind != SubprocessWritableKind.Pipe && _.kind != SubprocessWritableKind.Buffer) {
            return "";
        }
        return _.buf[_.buf_pos.._.buf_len];
    }

    /// Returns tracked memory cost in bytes (aligned with Bun `Writable.memoryCost`).
    fn memoryCost(): usize {
        return switch (_.kind) {
            .Pipe => SUBPROCESS_WRITABLE_PIPE_OVERHEAD_BYTES + (_.buf_len - _.buf_pos),
            .Buffer => SUBPROCESS_WRITABLE_BUFFER_OVERHEAD_BYTES + (_.buf_len - _.buf_pos),
            else => 0,
        };
    }

    /// Returns true if this writable keeps the subprocess alive due to pending buffer I/O.
    /// Aligned with Bun `Writable.hasPendingActivity`: `.buffer` is active until transitioned to `.ignore`.
    fn hasPendingActivity(): bool {
        return _.kind == SubprocessWritableKind.Buffer;
    }

    /// Marks an active `.Pipe` or `.Buffer` as keeping the event loop alive (no-op for other variants).
    fn ref(*_) {
        if (_.kind == SubprocessWritableKind.Pipe || _.kind == SubprocessWritableKind.Buffer) {
            _.ref_active = true;
        }
    }

    /// Releases the event loop ref on an active `.Pipe` or `.Buffer` (no-op for other variants).
    fn unref(*_) {
        if (_.kind == SubprocessWritableKind.Pipe || _.kind == SubprocessWritableKind.Buffer) {
            _.ref_active = false;
        }
    }

    /// Appends bytes to an active `.Pipe` stdin buffer (compacts consumed prefix before overflow check).
    fn writePipe(*_, data: []const u8): usize ! SubprocessWritableError {
        if (_.kind != SubprocessWritableKind.Pipe) return SubprocessWritableError.NotPipe;
        if (_.ended) return SubprocessWritableError.AlreadyEnded;

        const pending = _.buf_len - _.buf_pos;
        if (pending + data.len > SUBPROCESS_WRITABLE_BUF_CAP) {
            return SubprocessWritableError.BufferOverflow;
        }

        if (_.buf_pos > 0) {
            let j: usize = 0;
            while (j < pending) {
                _.buf[j] = _.buf[_.buf_pos + j];
                j += 1;
            }
            _.buf_pos = 0;
            _.buf_len = pending;
        }

        let i: usize = 0;
        while (i < data.len) {
            _.buf[_.buf_len + i] = data[i];
            i += 1;
        }
        _.buf_len += data.len;
        return data.len;
    }

    /// Flushes up to `out.len` pending bytes from `.Pipe` or `.Buffer` into `out`.
    /// When a `.Buffer` is completely drained, automatically invokes `onClose()` to transition to `.Ignore`.
    /// When an ended `.Pipe` is completely drained, automatically invokes `onClose()` to transition to `.Ignore`.
    fn flushTo(*_, out: []u8): usize ! SubprocessWritableError {
        if (_.kind != SubprocessWritableKind.Pipe && _.kind != SubprocessWritableKind.Buffer) {
            return SubprocessWritableError.NotWritable;
        }

        const pending = _.buf_len - _.buf_pos;
        const to_copy = if (out.len < pending) out.len else pending;
        let i: usize = 0;
        while (i < to_copy) {
            out[i] = _.buf[_.buf_pos + i];
            i += 1;
        }
        _.buf_pos += to_copy;
        _.flushed_bytes += to_copy;

        if (_.buf_pos == _.buf_len) {
            _.buf_pos = 0;
            _.buf_len = 0;
            if (_.kind == SubprocessWritableKind.Buffer) {
                _.onClose();
            } else if (_.kind == SubprocessWritableKind.Pipe && _.ended) {
                _.onClose();
            }
        }
        return to_copy;
    }

    /// Detaches an interactive `.Pipe` handle to the caller (aligned with Bun `Writable.toJS` ownership transfer).
    /// Transitions internal state to `.Ignore`; if `process_exited` is true, also releases the subprocess ref immediately.
    fn takePipeHandle(*_, process_exited: bool): i32 ! SubprocessWritableError {
        if (_.kind != SubprocessWritableKind.Pipe) return SubprocessWritableError.NotPipe;
        const handle_fd = _.fd;
        _.kind = SubprocessWritableKind.Ignore;
        _.fd = -1;
        _.detached_to_caller = true;
        _.buf_pos = 0;
        _.buf_len = 0;
        if (process_exited) {
            _.ref_active = false;
            _.subprocess_ref_held = false;
        }
        return handle_fd;
    }

    /// Notification invoked when the underlying sink/writer has closed (aligned with Bun `Writable.onClose`).
    /// Releases refs and transitions to `.Ignore` to prevent dangling handle reuse.
    fn onClose(*_) {
        _.kind = SubprocessWritableKind.Ignore;
        _.fd = -1;
        _.ref_active = false;
        _.subprocess_ref_held = false;
        _.ended = true;
        _.buf_pos = 0;
        _.buf_len = 0;
    }

    /// Initiates graceful closure (aligned with Bun `Writable.close`).
    /// For `.Pipe`, marks `ended = true` and transitions to `.Ignore` immediately if no pending bytes remain.
    /// For `.Memfd`, `.Fd`, and `.Buffer`, closes immediately and transitions to `.Ignore`.
    fn close(*_) {
        switch (_.kind) {
            .Pipe => {
                _.ended = true;
                if (_.buf_len == _.buf_pos) {
                    _.onClose();
                }
            },
            .Memfd => {
                _.onClose();
            },
            .Fd => {
                _.kind = SubprocessWritableKind.Ignore;
                _.fd = -1;
            },
            .Buffer => {
                _.onClose();
            },
            .Ignore => {},
            .Inherit => {},
        }
    }

    /// Finalizes resources during subprocess teardown (aligned with Bun `Writable.finalize`).
    /// Transitions `.Pipe`, `.Buffer`, and `.Memfd` to `.Ignore`; leaves `.Fd` and `.Inherit` untouched.
    fn finalize(*_) {
        switch (_.kind) {
            .Pipe => {
                _.onClose();
            },
            .Buffer => {
                _.onClose();
            },
            .Memfd => {
                _.onClose();
            },
            .Ignore => {},
            .Fd => {},
            .Inherit => {},
        }
    }

    fn isIgnored(): bool {
        return _.kind == SubprocessWritableKind.Ignore;
    }
}
