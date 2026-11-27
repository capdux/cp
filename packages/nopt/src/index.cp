// @cap/nopt — Zero-allocation CLI option parser in pure CP
// Clean-room ISC implementation inspired by npm `nopt`.

pub const NOPT_MAX_FLAGS: usize = 64;
pub const NOPT_MAX_REMAIN: usize = 64;

/// A parsed CLI option flag.
pub struct NoptFlag {
    key: string,
    value: string,
    is_bool: bool,
    bool_val: bool,
}

/// Shorthand mapping from a short option (e.g. `"v"` or `"-v"`) to a long key (e.g. `"verbose"` or `"--verbose"`).
pub struct NoptShorthand {
    short_key: string,
    long_key: string,
}

fn nptStrEq(a: string, b: string): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) {
            return false;
        }
    }
    return true;
}

fn nptHasPrefix(s: string, prefix: string): bool {
    if (prefix.len > s.len) {
        return false;
    }
    let i: usize = 0;
    while (i < prefix.len) : (i += 1) {
        if (s[i] != prefix[i]) {
            return false;
        }
    }
    return true;
}

fn nptIndexOfByte(s: string, ch: u8): usize? {
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        if (s[i] == ch) {
            return i;
        }
    }
    return null;
}

fn nptStripLeadingDashes(s: string): string {
    if (nptHasPrefix(s, "--")) {
        return s[2..s.len];
    }
    if (nptHasPrefix(s, "-")) {
        return s[1..s.len];
    }
    return s;
}

fn nptFindShorthand(shorthands: []const NoptShorthand, short_name: string): string? {
    let i: usize = 0;
    while (i < shorthands.len) : (i += 1) {
        const candidate: string = nptStripLeadingDashes(shorthands[i].short_key);
        if (nptStrEq(candidate, short_name)) {
            return shorthands[i].long_key;
        }
    }
    return null;
}

/// Fixed-capacity stack-allocated result of `parseNopt`.
pub struct NoptResult {
    flags: [64]NoptFlag,
    flag_count: usize,
    remain: [64]string,
    remain_count: usize,

    /// Create an empty `NoptResult`.
    static fn new(): NoptResult {
        let res: NoptResult = NoptResult {
            flags: undefined,
            flag_count: 0,
            remain: undefined,
            remain_count: 0,
        };
        let i: usize = 0;
        while (i < NOPT_MAX_FLAGS) : (i += 1) {
            res.flags[i] = NoptFlag {
                key: "",
                value: "",
                is_bool: false,
                bool_val: false,
            };
        }
        let j: usize = 0;
        while (j < NOPT_MAX_REMAIN) : (j += 1) {
            res.remain[j] = "";
        }
        return res;
    }

    /// Look up a flag's string value by key, or `null` if absent.
    fn getString(key: string): string? {
        let i: usize = 0;
        while (i < _.flag_count) : (i += 1) {
            if (nptStrEq(_.flags[i].key, key)) {
                return _.flags[i].value;
            }
        }
        return null;
    }

    /// Look up a flag's boolean value by key, or `null` if absent or non-boolean.
    fn getBool(key: string): bool? {
        let i: usize = 0;
        while (i < _.flag_count) : (i += 1) {
            if (nptStrEq(_.flags[i].key, key)) {
                if (_.flags[i].is_bool) {
                    return _.flags[i].bool_val;
                }
                if (nptStrEq(_.flags[i].value, "true")) {
                    return true;
                }
                if (nptStrEq(_.flags[i].value, "false")) {
                    return false;
                }
                return null;
            }
        }
        return null;
    }

    /// Returns `true` if `key` was parsed as a flag.
    fn has(key: string): bool {
        let i: usize = 0;
        while (i < _.flag_count) : (i += 1) {
            if (nptStrEq(_.flags[i].key, key)) {
                return true;
            }
        }
        return false;
    }

    /// Number of parsed positional / remaining arguments.
    fn remainCount(): usize {
        return _.remain_count;
    }

    /// Return the positional argument at `idx`, or `""` if out of bounds.
    fn remainAt(idx: usize): string {
        if (idx >= _.remain_count) {
            return "";
        }
        return _.remain[idx];
    }

    /// Insert or overwrite a flag entry.
    fn setFlag(*_, key: string, value: string, is_bool: bool, bool_val: bool) {
        if (key.len == 0) {
            return;
        }
        let i: usize = 0;
        while (i < _.flag_count) : (i += 1) {
            if (nptStrEq(_.flags[i].key, key)) {
                _.flags[i].value = value;
                _.flags[i].is_bool = is_bool;
                _.flags[i].bool_val = bool_val;
                return;
            }
        }
        if (_.flag_count < NOPT_MAX_FLAGS) {
            _.flags[_.flag_count] = NoptFlag {
                key: key,
                value: value,
                is_bool: is_bool,
                bool_val: bool_val,
            };
            _.flag_count += 1;
        }
    }

    /// Append a positional argument to `remain`.
    fn pushRemain(*_, arg: string) {
        if (_.remain_count < NOPT_MAX_REMAIN) {
            _.remain[_.remain_count] = arg;
            _.remain_count += 1;
        }
    }
}

fn nptIsOptionToken(arg: string): bool {
    return arg.len >= 2 && arg[0] == '-';
}

/// Parse CLI arguments with optional shorthand expansion (zero heap allocations).
pub fn parseNopt(args: []const string, shorthands: []const NoptShorthand): NoptResult {
    let res: NoptResult = NoptResult.new();
    let i: usize = 0;
    let after_double_dash: bool = false;

    while (i < args.len) : (i += 1) {
        const arg: string = args[i];
        if (after_double_dash) {
            res.pushRemain(arg);
            continue;
        }
        if (nptStrEq(arg, "--")) {
            after_double_dash = true;
            continue;
        }
        if (nptHasPrefix(arg, "--")) {
            const body: string = arg[2..arg.len];
            if (eq_pos := nptIndexOfByte(body, '=')) {
                const key: string = body[0..eq_pos];
                const val: string = body[(eq_pos + 1)..body.len];
                res.setFlag(key, val, false, false);
            } else if (nptHasPrefix(body, "no-") && body.len > 3) {
                const key: string = body[3..body.len];
                res.setFlag(key, "false", true, false);
            } else {
                if (i + 1 < args.len && !nptIsOptionToken(args[i + 1])) {
                    i += 1;
                    res.setFlag(body, args[i], false, false);
                } else {
                    res.setFlag(body, "true", true, true);
                }
            }
            continue;
        }
        if (arg.len >= 2 && arg[0] == '-') {
            const body: string = arg[1..arg.len];
            if (eq_pos := nptIndexOfByte(body, '=')) {
                const skey: string = body[0..eq_pos];
                const val: string = body[(eq_pos + 1)..body.len];
                let resolved: string = skey;
                if (expanded := nptFindShorthand(shorthands, skey)) {
                    resolved = nptStripLeadingDashes(expanded);
                }
                res.setFlag(resolved, val, false, false);
                continue;
            }
            // Exact shorthand match (single or multi-char short key)
            if (expanded := nptFindShorthand(shorthands, body)) {
                const clean: string = nptStripLeadingDashes(expanded);
                if (nptHasPrefix(clean, "no-") && clean.len > 3) {
                    res.setFlag(clean[3..clean.len], "false", true, false);
                } else if (i + 1 < args.len && !nptIsOptionToken(args[i + 1])) {
                    i += 1;
                    res.setFlag(clean, args[i], false, false);
                } else {
                    res.setFlag(clean, "true", true, true);
                }
                continue;
            }
            // Single-char short flag not in shorthands
            if (body.len == 1) {
                if (i + 1 < args.len && !nptIsOptionToken(args[i + 1])) {
                    i += 1;
                    res.setFlag(body, args[i], false, false);
                } else {
                    res.setFlag(body, "true", true, true);
                }
                continue;
            }
            // Bundled short boolean flags like `-abc`
            let k: usize = 0;
            while (k < body.len) : (k += 1) {
                const ch_slice: string = body[k..(k + 1)];
                if (expanded := nptFindShorthand(shorthands, ch_slice)) {
                    const clean: string = nptStripLeadingDashes(expanded);
                    if (nptHasPrefix(clean, "no-") && clean.len > 3) {
                        res.setFlag(clean[3..clean.len], "false", true, false);
                    } else {
                        res.setFlag(clean, "true", true, true);
                    }
                } else {
                    res.setFlag(ch_slice, "true", true, true);
                }
            }
            continue;
        }
        res.pushRemain(arg);
    }

    return res;
}
