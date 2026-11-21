// Source: Absorbed from /data/dev/bun/src/clap/streaming.cp | Synced: 2026-11-21T13:43:36Z
// Pure-CP zero-allocation streaming CLI argument parser supporting --long,
// --long=val, chained short flags (-abc), -o=val/-oval/-o val, and `--` separator.

pub enum TakesValue: u8 {
    none = 0,
    one = 1,
    one_optional = 2,
}

pub struct ParamSpec {
    /// Numeric identifier returned on match.
    id: u16,
    /// Optional short flag character (0 means none).
    short: u8,
    /// Optional long flag name without leading `--` (empty means none).
    long: []const u8,
    takes_value: TakesValue,
    is_positional: bool,
}

pub enum ParseError: u8 {
    none = 0,
    doesnt_take_value = 1,
    missing_value = 2,
    unknown_flag = 3,
    unexpected_positional = 4,
}

pub struct ParsedArg {
    id: u16,
    value: ([]const u8)?,
    err: ParseError,
    raw: []const u8,

    pub fn isOk(): bool {
        return _.err == .none;
    }
}

enum StreamMode: u8 {
    normal = 0,
    chaining = 1,
    rest_positional = 2,
}

/// Zero-allocation streaming CLI argument parser.
pub struct StreamingClap {
    params: []const ParamSpec,
    argv: []const []const u8,
    arg_idx: usize,
    mode: StreamMode,
    chain_arg: []const u8,
    chain_idx: usize,
    ignore_unknown: bool,

    pub static fn init(params: []const ParamSpec, argv: []const []const u8): StreamingClap {
        return StreamingClap{
            params: params,
            argv: argv,
            arg_idx: 0,
            mode: .normal,
            chain_arg: "",
            chain_idx: 0,
            ignore_unknown: false,
        };
    }

    fn nextRaw(*_): ([]const u8)? {
        if (_.arg_idx >= _.argv.len) return null;
        const a = _.argv[_.arg_idx];
        _.arg_idx += 1;
        return a;
    }

    fn findPositionalId(): u16? {
        let i: usize = 0;
        while (i < _.params.len) : (i += 1) {
            if (_.params[i].is_positional) return _.params[i].id;
        }
        return null;
    }

    fn findLong(name: []const u8): ParamSpec? {
        let i: usize = 0;
        while (i < _.params.len) : (i += 1) {
            const p = _.params[i];
            if (!p.is_positional && p.long.len > 0 && strEq(p.long, name)) {
                return p;
            }
        }
        return null;
    }

    fn findShort(ch: u8): ParamSpec? {
        if (ch == 0) return null;
        let i: usize = 0;
        while (i < _.params.len) : (i += 1) {
            const p = _.params[i];
            if (!p.is_positional && p.short == ch) {
                return p;
            }
        }
        return null;
    }

    /// Parse the next argument token. Returns `null` when all arguments are exhausted.
    pub fn next(*_): ParsedArg? {
        while (true) {
            if (_.mode == .chaining) {
                return _.parseChainStep();
            }
            if (_.mode == .rest_positional) {
                const raw = _.nextRaw() ?? return null;
                if (pid := _.findPositionalId()) {
                    return ParsedArg{ id: pid, value: raw, err: .none, raw: raw };
                }
                return ParsedArg{ id: 0, value: null, err: .unexpected_positional, raw: raw };
            }

            const raw = _.nextRaw() ?? return null;
            if (strEq(raw, "--")) {
                _.mode = .rest_positional;
                continue;
            }

            // Long flag: `--name` or `--name=value`
            if (raw.len > 2 && raw[0] == '-' && raw[1] == '-') {
                const body = raw[2..];
                let eq_pos: usize? = null;
                let k: usize = 0;
                while (k < body.len) : (k += 1) {
                    if (body[k] == '=') {
                        eq_pos = k;
                        break;
                    }
                }
                let name = body;
                let inline_val: ([]const u8)? = null;
                if (ep := eq_pos) {
                    name = body[0..ep];
                    inline_val = body[ep + 1 ..];
                }

                const spec_opt = _.findLong(name);
                if (spec_opt == null) {
                    if (_.ignore_unknown) continue;
                    return ParsedArg{ id: 0, value: null, err: .unknown_flag, raw: raw };
                }
                const spec = spec_opt.?;

                if (spec.takes_value == .none) {
                    if (inline_val != null) {
                        return ParsedArg{ id: spec.id, value: null, err: .doesnt_take_value, raw: raw };
                    }
                    return ParsedArg{ id: spec.id, value: null, err: .none, raw: raw };
                }
                if (spec.takes_value == .one_optional) {
                    return ParsedArg{ id: spec.id, value: inline_val, err: .none, raw: raw };
                }
                // takes_value == .one
                if (iv := inline_val) {
                    return ParsedArg{ id: spec.id, value: iv, err: .none, raw: raw };
                }
                if (nv := _.nextRaw()) {
                    return ParsedArg{ id: spec.id, value: nv, err: .none, raw: raw };
                }
                return ParsedArg{ id: spec.id, value: null, err: .missing_value, raw: raw };
            }

            // Short flag(s): `-a`, `-abc`, `-o=val`, `-oval`, `-o val`
            if (raw.len >= 2 && raw[0] == '-') {
                _.mode = .chaining;
                _.chain_arg = raw[1..];
                _.chain_idx = 0;
                return _.parseChainStep();
            }

            // Positional argument (including single "-")
            if (pid := _.findPositionalId()) {
                return ParsedArg{ id: pid, value: raw, err: .none, raw: raw };
            }
            return ParsedArg{ id: 0, value: null, err: .unexpected_positional, raw: raw };
        }
    }

    fn parseChainStep(*_): ParsedArg {
        const arg = _.chain_arg;
        const idx = _.chain_idx;
        const ch = arg[idx];
        const next_idx = idx + 1;

        const spec_opt = _.findShort(ch);
        if (spec_opt == null) {
            if (next_idx >= arg.len) {
                _.mode = .normal;
            } else {
                _.chain_idx = next_idx;
            }
            return ParsedArg{ id: 0, value: null, err: .unknown_flag, raw: arg[idx..next_idx] };
        }
        const spec = spec_opt.?;

        const next_is_eq = next_idx < arg.len && arg[next_idx] == '=';
        if (spec.takes_value == .none) {
            if (next_is_eq) {
                _.mode = .normal;
                return ParsedArg{ id: spec.id, value: null, err: .doesnt_take_value, raw: arg };
            }
            if (next_idx >= arg.len) {
                _.mode = .normal;
            } else {
                _.chain_idx = next_idx;
            }
            return ParsedArg{ id: spec.id, value: null, err: .none, raw: arg[idx..next_idx] };
        }

        // Consumes value -> ends chaining
        _.mode = .normal;
        if (next_is_eq) {
            return ParsedArg{ id: spec.id, value: arg[next_idx + 1 ..], err: .none, raw: arg };
        }
        if (next_idx < arg.len) {
            return ParsedArg{ id: spec.id, value: arg[next_idx..], err: .none, raw: arg };
        }
        if (spec.takes_value == .one_optional) {
            return ParsedArg{ id: spec.id, value: null, err: .none, raw: arg[idx..next_idx] };
        }
        if (nv := _.nextRaw()) {
            return ParsedArg{ id: spec.id, value: nv, err: .none, raw: arg };
        }
        return ParsedArg{ id: spec.id, value: null, err: .missing_value, raw: arg };
    }
}

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}
