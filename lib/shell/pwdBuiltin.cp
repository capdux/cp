// Source: Absorbed from /data/dev/bun/src/shell/builtin/pwd.cp | Synced: 2026-11-22T19:40:30Z (POSIX/Bun shell pwd builtin state machine, option parser & zero-alloc output formatter)
// Pure-CP zero-allocation shell `pwd` builtin command state machine, POSIX -L/-P flag parser, and output formatter.

/// Target stream for `pwd` builtin output.
pub enum PwdOutputKind: u8 {
    Stdout = 0,
    Stderr = 1,
}

/// Execution state machine for `PwdBuiltin` (aligned with Bun `shell/builtin/pwd.cp`).
pub enum PwdState: u8 {
    Idle = 0,
    WaitingStdout = 1,
    WaitingStderr = 2,
    Err = 3,
    Done = 4,
}

/// Resolution mode for POSIX `pwd` (`-L` logical vs `-P` physical).
pub enum PwdMode: u8 {
    Logical = 0,
    Physical = 1,
}

/// Error set for `pwd` builtin argument parsing and execution.
pub err PwdError {
    TooManyArguments,
    InvalidOption,
    BufferTooSmall,
    InvalidState,
}

/// Result of executing `PwdBuiltin.start` or `runPwdBuiltin`.
pub struct PwdResult {
    exit_code: u8 = 0,
    bytes_written: usize = 0,
    output_kind: PwdOutputKind = PwdOutputKind.Stdout,
    mode: PwdMode = PwdMode.Logical,
}

const PWD_ERR_TOO_MANY: []const u8 = "pwd: too many arguments\n";
const PWD_ERR_INVALID_OPT: []const u8 = "pwd: invalid option\n";

fn writeSlice(dst: []u8, pos: usize, src: []const u8): usize ! PwdError {
    if (pos + src.len > dst.len) return PwdError.BufferTooSmall;
    let i: usize = 0;
    while (i < src.len) {
        dst[pos + i] = src[i];
        i += 1;
    }
    return pos + src.len;
}

/// Parses POSIX `pwd` arguments (`-L`, `-P`, combined `-LP`/`-PL`, `--`).
/// Rejects positional arguments with `PwdError.TooManyArguments` and unknown flags with `PwdError.InvalidOption`.
pub fn parsePwdArgs(args: []const ([]const u8)): PwdMode ! PwdError {
    let mode = PwdMode.Logical;
    let end_of_opts = false;
    let i: usize = 0;
    while (i < args.len) {
        const arg = args[i];
        if (!end_of_opts && arg.len == 2 && arg[0] == '-' && arg[1] == '-') {
            end_of_opts = true;
            i += 1;
            continue;
        }
        if (!end_of_opts && arg.len >= 2 && arg[0] == '-') {
            let j: usize = 1;
            while (j < arg.len) {
                const ch = arg[j];
                if (ch == 'L') {
                    mode = PwdMode.Logical;
                } else if (ch == 'P') {
                    mode = PwdMode.Physical;
                } else {
                    return PwdError.InvalidOption;
                }
                j += 1;
            }
            i += 1;
            continue;
        }
        return PwdError.TooManyArguments;
    }
    return mode;
}

/// Zero-allocation `pwd` builtin state machine (aligned with Bun `shell/builtin/pwd.cp`).
pub struct PwdBuiltin {
    state: PwdState = PwdState.Idle,
    exit_code: u8 = 0,
    mode: PwdMode = PwdMode.Logical,

    static fn init(): PwdBuiltin {
        return PwdBuiltin{};
    }

    /// Returns true when the builtin has reached a terminal state (`Done` or `Err`).
    fn isTerminal(): bool {
        return _.state == PwdState.Done || _.state == PwdState.Err;
    }

    /// Runs `pwd` synchronously into caller-provided `out` buffer.
    /// Formats `"<cwd>\n"` to stdout on success, or standard diagnostic to stderr on invalid arguments.
    fn start(
        *_,
        args: []const ([]const u8),
        logical_cwd: []const u8,
        physical_cwd: []const u8,
        out: []u8,
    ): PwdResult ! PwdError {
        const parsed = parsePwdArgs(args) catch e => {
            if (e == PwdError.TooManyArguments) {
                const n = writeSlice(out, 0, PWD_ERR_TOO_MANY)?;
                _.state = PwdState.Err;
                _.exit_code = 1;
                return PwdResult{
                    exit_code: 1,
                    bytes_written: n,
                    output_kind: PwdOutputKind.Stderr,
                    mode: _.mode,
                };
            }
            const n = writeSlice(out, 0, PWD_ERR_INVALID_OPT)?;
            _.state = PwdState.Err;
            _.exit_code = 1;
            return PwdResult{
                exit_code: 1,
                bytes_written: n,
                output_kind: PwdOutputKind.Stderr,
                mode: _.mode,
            };
        };

        _.mode = parsed;
        const chosen = if (parsed == PwdMode.Physical)
            (if (physical_cwd.len > 0) physical_cwd else logical_cwd)
        else
            (if (logical_cwd.len > 0) logical_cwd else physical_cwd);

        let pos = writeSlice(out, 0, chosen)?;
        pos = writeSlice(out, pos, "\n")?;
        _.state = PwdState.Done;
        _.exit_code = 0;
        return PwdResult{
            exit_code: 0,
            bytes_written: pos,
            output_kind: PwdOutputKind.Stdout,
            mode: parsed,
        };
    }

    /// Transitions the state machine into an async I/O wait state (`WaitingStdout` / `WaitingStderr`).
    fn startAsync(*_, kind: PwdOutputKind): PwdState ! PwdError {
        if (_.state != PwdState.Idle) return PwdError.InvalidState;
        _.state = if (kind == PwdOutputKind.Stdout) PwdState.WaitingStdout else PwdState.WaitingStderr;
        return _.state;
    }

    /// Completes an async I/O write (aligned with Bun `Pwd.onIOWriterChunk`) and returns final exit code (`0` or `1`).
    fn onIoComplete(*_, io_ok: bool): u8 ! PwdError {
        if (_.state != PwdState.WaitingStdout && _.state != PwdState.WaitingStderr) {
            return PwdError.InvalidState;
        }
        if (!io_ok) {
            _.state = PwdState.Err;
            _.exit_code = 1;
            return 1;
        }
        if (_.state == PwdState.WaitingStdout) {
            _.state = PwdState.Done;
            _.exit_code = 0;
            return 0;
        }
        _.state = PwdState.Err;
        _.exit_code = 1;
        return 1;
    }
}

/// Convenience helper: runs `pwd` with a single `cwd` path into `out`.
pub fn runPwdBuiltin(args: []const ([]const u8), cwd: []const u8, out: []u8): PwdResult ! PwdError {
    let builtin = PwdBuiltin.init();
    return builtin.start(args, cwd, cwd, out);
}
