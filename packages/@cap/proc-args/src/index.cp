// @cap/proc-args — Zero-allocation POSIX/GNU CLI Argument Parser
// Clean-room MIT implementation in pure CP.

pub const ARG_POSITIONAL: u8 = 0;
pub const ARG_LONG_FLAG: u8 = 1;
pub const ARG_LONG_VALUE: u8 = 2;
pub const ARG_SHORT_FLAG: u8 = 3;

pub struct ParsedArg {
    pub kind: u8,
    pub name: string,
    pub value: string,
}

fn strEq(a: string, b: string) -> bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

/// Parses `argv` into structured `ParsedArg` entries in `out` (zero-copy).
/// Supports `--flag`, `--key=val`, clustered short flags `-abc` (split into 3 `ARG_SHORT_FLAG`),
/// lone `"-"` as positional stdin, and `"--"` end-of-options terminator.
/// Returns the number of entries written into `out`.
pub fn parseArgs(argv: []const string, out: []ParsedArg) -> usize {
    let count: usize = 0;
    let after_ddash = false;
    let i: usize = 0;

    while (i < argv.len && count < out.len) {
        const arg = argv[i];

        if (after_ddash) {
            out[count] = ParsedArg { kind: ARG_POSITIONAL, name: "", value: arg };
            count += 1;
            i += 1;
            continue;
        }

        if (arg.len == 2 && arg[0] == 45 && arg[1] == 45) {
            // "--" terminates option parsing
            after_ddash = true;
            i += 1;
            continue;
        }

        if (arg.len >= 3 && arg[0] == 45 && arg[1] == 45) {
            // Long option: --name or --name=val
            let eq: usize = 2;
            while (eq < arg.len && arg[eq] != 61) {
                eq += 1;
            }
            if (eq < arg.len) {
                out[count] = ParsedArg {
                    kind: ARG_LONG_VALUE,
                    name: arg[2..eq],
                    value: arg[eq + 1..arg.len],
                };
            } else {
                out[count] = ParsedArg {
                    kind: ARG_LONG_FLAG,
                    name: arg[2..arg.len],
                    value: "",
                };
            }
            count += 1;
            i += 1;
            continue;
        }

        if (arg.len >= 2 && arg[0] == 45) {
            // Short option cluster: -a or -abc
            let k: usize = 1;
            while (k < arg.len && count < out.len) {
                out[count] = ParsedArg {
                    kind: ARG_SHORT_FLAG,
                    name: arg[k..k + 1],
                    value: "",
                };
                count += 1;
                k += 1;
            }
            i += 1;
            continue;
        }

        // Positional argument (including lone "-" for stdin)
        out[count] = ParsedArg { kind: ARG_POSITIONAL, name: "", value: arg };
        count += 1;
        i += 1;
    }

    return count;
}

/// Checks if a long flag (`--long_name`) or short flag (`-<short_char>`, including inside `-abc` clusters)
/// appears in `argv` before `--`. Pass `""` for `long_name` or `0` for `short_char` to skip either.
pub fn hasFlag(argv: []const string, long_name: string, short_char: u8) -> bool {
    let i: usize = 0;
    while (i < argv.len) {
        const arg = argv[i];
        if (arg.len == 2 && arg[0] == 45 && arg[1] == 45) {
            break;
        }
        if (long_name.len > 0 && arg.len >= 3 && arg[0] == 45 && arg[1] == 45) {
            const rest = arg[2..arg.len];
            if (strEq(rest, long_name)) {
                return true;
            }
            if (rest.len > long_name.len && strEq(rest[0..long_name.len], long_name) && rest[long_name.len] == 61) {
                const v = rest[long_name.len + 1..rest.len];
                if (strEq(v, "true") || strEq(v, "1")) {
                    return true;
                }
            }
        } else if (short_char != 0 && arg.len >= 2 && arg[0] == 45 && arg[1] != 45) {
            let k: usize = 1;
            while (k < arg.len) {
                if (arg[k] == short_char) {
                    return true;
                }
                k += 1;
            }
        }
        i += 1;
    }
    return false;
}

/// Extracts an option value for `--long_name=val`, `--long_name val`, or `-<short_char> val`
/// appearing before `--`. Returns `""` if not found.
pub fn getOption(argv: []const string, long_name: string, short_char: u8) -> string {
    let i: usize = 0;
    while (i < argv.len) {
        const arg = argv[i];
        if (arg.len == 2 && arg[0] == 45 && arg[1] == 45) {
            break;
        }
        if (long_name.len > 0 && arg.len >= 3 && arg[0] == 45 && arg[1] == 45) {
            const rest = arg[2..arg.len];
            if (strEq(rest, long_name)) {
                if (i + 1 < argv.len) {
                    return argv[i + 1];
                }
                return "";
            }
            if (rest.len > long_name.len && strEq(rest[0..long_name.len], long_name) && rest[long_name.len] == 61) {
                return rest[long_name.len + 1..rest.len];
            }
        } else if (short_char != 0 && arg.len == 2 && arg[0] == 45 && arg[1] == short_char) {
            if (i + 1 < argv.len) {
                return argv[i + 1];
            }
            return "";
        }
        i += 1;
    }
    return "";
}

/// Counts non-option positional arguments in `argv` (including lone `"-"` and all arguments after `"--"`).
pub fn countPositionals(argv: []const string) -> usize {
    let count: usize = 0;
    let after_ddash = false;
    let i: usize = 0;
    while (i < argv.len) {
        const arg = argv[i];
        if (after_ddash) {
            count += 1;
        } else if (arg.len == 2 && arg[0] == 45 && arg[1] == 45) {
            after_ddash = true;
        } else if (arg.len == 0 || arg[0] != 45 || arg.len == 1) {
            count += 1;
        }
        i += 1;
    }
    return count;
}
