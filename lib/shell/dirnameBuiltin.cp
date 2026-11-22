// Source: Absorbed from /data/dev/bun/src/shell/builtin/dirname.cp | Synced: 2026-11-22T20:10:50Z (POSIX/Bun shell dirname builtin state machine, multi-path POSIX dirname evaluation & async I/O writer completion)
// Pure-CP zero-allocation shell `dirname` builtin command state machine and POSIX path dirname evaluator.

/// Usage diagnostic written to stderr when `dirname` is invoked with zero arguments.
pub const DIRNAME_USAGE: []const u8 = "usage: dirname path ...\n";

/// Target stream for `dirname` builtin output.
pub enum DirnameOutputKind: u8 {
    Stdout = 0,
    Stderr = 1,
}

/// Execution state machine for `DirnameBuiltin` (aligned with Bun `shell/builtin/dirname.cp`).
pub enum DirnameState: u8 {
    Idle = 0,
    WaitingStdout = 1,
    WaitingStderr = 2,
    Err = 3,
    Done = 4,
}

/// Error set for `dirname` builtin execution.
pub err DirnameError {
    BufferTooSmall,
    InvalidState,
}

/// Execution result of a `DirnameBuiltin` step.
pub struct DirnameResult {
    output_kind: DirnameOutputKind = DirnameOutputKind.Stdout,
    bytes_written: usize = 0,
    exit_code: u8 = 0,
    paths_processed: usize = 0,
    needs_async_io: bool = false,
}

/// Evaluates POSIX `dirname` for a single path without heap allocation.
/// - `""` -> `"."`
/// - `"/"`, `"///"` -> `"/"`
/// - `"foo"`, `"foo//"` -> `"."`
/// - `"/foo"`, `"/foo/"` -> `"/"`
/// - `"/a/b/c/"` -> `"/a/b"`
/// - `"a//b"` -> `"a"`
pub fn evalPosixDirname(path: []const u8): []const u8 {
    if (path.len == 0) return ".";

    // Strip trailing '/' characters
    let end: usize = path.len;
    while (end > 0 && path[end - 1] == '/') {
        end -= 1;
    }
    if (end == 0) {
        return "/";
    }

    // Find last '/' before the final component
    let sep: usize = end;
    while (sep > 0 && path[sep - 1] != '/') {
        sep -= 1;
    }
    if (sep == 0) {
        return ".";
    }

    // Strip trailing '/' from the directory prefix, preserving leading root '/'
    let dir_end: usize = sep;
    while (dir_end > 1 && path[dir_end - 1] == '/') {
        dir_end -= 1;
    }
    return path[0..dir_end];
}

fn appendBytes(out: []u8, pos: *usize, src: []const u8): usize ! DirnameError {
    if (pos.* + src.len > out.len) return DirnameError.BufferTooSmall;
    let i: usize = 0;
    while (i < src.len) {
        out[pos.* + i] = src[i];
        i += 1;
    }
    pos.* += src.len;
    return src.len;
}

/// Zero-allocation POSIX/Bun shell `dirname` builtin state machine.
pub struct DirnameBuiltin {
    state: DirnameState = DirnameState.Idle,
    exit_code: u8 = 0,
    paths_processed: usize = 0,

    static fn init(): DirnameBuiltin {
        return DirnameBuiltin{};
    }

    fn isTerminal(): bool {
        return _.state == DirnameState.Done || _.state == DirnameState.Err;
    }

    fn formatInto(*_, args: []const []const u8, out: []u8, async_io: bool): DirnameResult ! DirnameError {
        if (_.state != DirnameState.Idle) return DirnameError.InvalidState;

        let pos: usize = 0;
        if (args.len == 0) {
            * = appendBytes(out, &pos, DIRNAME_USAGE)?;
            _.exit_code = 1;
            _.paths_processed = 0;
            _.state = if (async_io) DirnameState.WaitingStderr else DirnameState.Err;
            return DirnameResult{
                output_kind: DirnameOutputKind.Stderr,
                bytes_written: pos,
                exit_code: 1,
                paths_processed: 0,
                needs_async_io: async_io,
            };
        }

        let i: usize = 0;
        while (i < args.len) {
            const dir = evalPosixDirname(args[i]);
            * = appendBytes(out, &pos, dir)?;
            * = appendBytes(out, &pos, "\n")?;
            i += 1;
        }

        _.exit_code = 0;
        _.paths_processed = args.len;
        _.state = if (async_io) DirnameState.WaitingStdout else DirnameState.Done;
        return DirnameResult{
            output_kind: DirnameOutputKind.Stdout,
            bytes_written: pos,
            exit_code: 0,
            paths_processed: args.len,
            needs_async_io: async_io,
        };
    }

    /// Runs `dirname` synchronously into caller-provided `out` buffer.
    fn start(*_, args: []const []const u8, out: []u8): DirnameResult ! DirnameError {
        return _.formatInto(args, out, false);
    }

    /// Runs `dirname` in async I/O mode, transitioning to `WaitingStdout` / `WaitingStderr`.
    fn startAsync(*_, args: []const []const u8, out: []u8): DirnameResult ! DirnameError {
        return _.formatInto(args, out, true);
    }

    /// Completes pending async I/O writer notification (aligned with Bun `Dirname.onIOWriterChunk`).
    fn onIOWriterChunk(*_, io_failed: bool): u8 ! DirnameError {
        if (_.state != DirnameState.WaitingStdout && _.state != DirnameState.WaitingStderr) {
            return DirnameError.InvalidState;
        }
        if (io_failed) {
            _.state = DirnameState.Err;
            _.exit_code = 1;
            return 1;
        }
        if (_.state == DirnameState.WaitingStdout) {
            _.state = DirnameState.Done;
            _.exit_code = 0;
            return 0;
        }
        _.state = DirnameState.Err;
        _.exit_code = 1;
        return 1;
    }
}

/// One-shot convenience helper executing `DirnameBuiltin.start`.
pub fn runDirnameBuiltin(args: []const []const u8, out: []u8): DirnameResult ! DirnameError {
    let b = DirnameBuiltin.init();
    return b.start(args, out);
}
