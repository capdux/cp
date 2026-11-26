// @cap/envsubst — Zero-allocation POSIX / Docker / K8s environment variable expansion engine for CP
// Clean-room MIT implementation inspired by npm `dotenv-expand`, `envsub`, and Rust `shellexpand`.
//
// Supported POSIX parameter expansion syntax:
// - $VAR, ${VAR}            : basic substitution
// - ${VAR:-default}         : default value when unset or empty
// - ${VAR-default}          : default value only when unset
// - ${VAR:+alt}             : alternate value when set and non-empty
// - ${VAR+alt}              : alternate value when set (even if empty)
// - ${VAR:?err_msg}         : error (MissingRequired) when unset or empty
// - ${VAR?err_msg}          : error (MissingRequired) when unset
// - $$ and \$               : literal '$' escape
// - Nested defaults supported up to depth 8 with zero heap allocation.

const MAX_ENV_VARS: usize = 64;
const MAX_EXPAND_DEPTH: u8 = 8;

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

fn isVarStart(c: u8) -> bool {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

fn isVarCont(c: u8) -> bool {
    return isVarStart(c) || (c >= '0' && c <= '9');
}

/// Stack-allocated key-value environment variable map (up to 64 entries, zero heap allocation).
pub struct EnvMap {
    pub keys: [MAX_ENV_VARS]string,
    pub vals: [MAX_ENV_VARS]string,
    pub len: usize,

    pub static fn init() -> EnvMap {
        let k: [MAX_ENV_VARS]string = undefined;
        let v: [MAX_ENV_VARS]string = undefined;
        return EnvMap{ keys: k, vals: v, len: 0 };
    }

    pub fn set(*_, key: string, val: string) {
        let i: usize = 0;
        while (i < _.len) {
            if (strEq(_.keys[i], key)) {
                _.vals[i] = val;
                return;
            }
            i += 1;
        }
        if (_.len < MAX_ENV_VARS) {
            _.keys[_.len] = key;
            _.vals[_.len] = val;
            _.len += 1;
        }
    }

    pub fn get(key: string) -> string? {
        let i: usize = 0;
        while (i < _.len) {
            if (strEq(_.keys[i], key)) {
                return _.vals[i];
            }
            i += 1;
        }
        return null;
    }

    pub fn has(key: string) -> bool {
        return _.get(key) != null;
    }
}

pub enum ExpandError: u8 {
    None,
    UnclosedBrace,
    MissingRequired,
    BufferOverflow,
}

pub struct ExpandResult {
    pub ok: bool,
    pub err: ExpandError,
    pub len: usize,
    pub missing_var: string,
}

fn expandImpl(tpl: string, env: EnvMap, out: []u8, start_out: usize, depth: u8) -> ExpandResult {
    let out_pos = start_out;
    let i: usize = 0;

    while (i < tpl.len) {
        // Backslash-escaped dollar: \$ -> $
        if (tpl[i] == '\\' && i + 1 < tpl.len && tpl[i + 1] == '$') {
            if (out_pos >= out.len) {
                return ExpandResult{ ok: false, err: ExpandError.BufferOverflow, len: out_pos, missing_var: "" };
            }
            out[out_pos] = '$';
            out_pos += 1;
            i += 2;
            continue;
        }

        if (tpl[i] == '$') {
            // Double-dollar escape: $$ -> $
            if (i + 1 < tpl.len && tpl[i + 1] == '$') {
                if (out_pos >= out.len) {
                    return ExpandResult{ ok: false, err: ExpandError.BufferOverflow, len: out_pos, missing_var: "" };
                }
                out[out_pos] = '$';
                out_pos += 1;
                i += 2;
                continue;
            }

            // Braced expansion: ${...}
            if (i + 1 < tpl.len && tpl[i + 1] == '{') {
                // Find matching closing '}' respecting nested braces
                let j = i + 2;
                let brace_depth: usize = 0;
                let found_close = false;
                while (j < tpl.len) {
                    if (tpl[j] == '\\' && j + 1 < tpl.len) {
                        j += 2;
                        continue;
                    }
                    if (tpl[j] == '{') {
                        brace_depth += 1;
                    } else if (tpl[j] == '}') {
                        if (brace_depth == 0) {
                            found_close = true;
                            break;
                        }
                        brace_depth -= 1;
                    }
                    j += 1;
                }

                if (!found_close) {
                    return ExpandResult{ ok: false, err: ExpandError.UnclosedBrace, len: out_pos, missing_var: "" };
                }

                const close_idx = j;
                const name_start = i + 2;
                let name_end = name_start;
                if (name_end < close_idx && isVarStart(tpl[name_end])) {
                    name_end += 1;
                    while (name_end < close_idx && isVarCont(tpl[name_end])) {
                        name_end += 1;
                    }
                }
                const var_name = tpl[name_start..name_end];
                const val_opt = env.get(var_name);

                // Simple ${VAR}
                if (name_end == close_idx) {
                    if (val_opt != null) {
                        const val = val_opt.?;
                        if (out_pos + val.len > out.len) {
                            return ExpandResult{ ok: false, err: ExpandError.BufferOverflow, len: out_pos, missing_var: "" };
                        }
                        let k: usize = 0;
                        while (k < val.len) {
                            out[out_pos + k] = val[k];
                            k += 1;
                        }
                        out_pos += val.len;
                    }
                    i = close_idx + 1;
                    continue;
                }

                // Operator parsing: :-, -, :+, +, :?, ?
                let colon = false;
                let op_idx = name_end;
                if (op_idx < close_idx && tpl[op_idx] == ':') {
                    colon = true;
                    op_idx += 1;
                }

                if (op_idx < close_idx) {
                    const op = tpl[op_idx];
                    const arg = tpl[(op_idx + 1)..close_idx];

                    if (op == '-') {
                        const use_default = if (colon) {
                            val_opt == null || val_opt.?.len == 0
                        } else {
                            val_opt == null
                        };
                        if (use_default) {
                            if (depth < MAX_EXPAND_DEPTH) {
                                const sub = expandImpl(arg, env, out, out_pos, depth + 1);
                                if (!sub.ok) {
                                    return sub;
                                }
                                out_pos = sub.len;
                            } else {
                                if (out_pos + arg.len > out.len) {
                                    return ExpandResult{ ok: false, err: ExpandError.BufferOverflow, len: out_pos, missing_var: "" };
                                }
                                let k: usize = 0;
                                while (k < arg.len) {
                                    out[out_pos + k] = arg[k];
                                    k += 1;
                                }
                                out_pos += arg.len;
                            }
                        } else {
                            const val = val_opt.?;
                            if (out_pos + val.len > out.len) {
                                return ExpandResult{ ok: false, err: ExpandError.BufferOverflow, len: out_pos, missing_var: "" };
                            }
                            let k: usize = 0;
                            while (k < val.len) {
                                out[out_pos + k] = val[k];
                                k += 1;
                            }
                            out_pos += val.len;
                        }
                        i = close_idx + 1;
                        continue;
                    }

                    if (op == '+') {
                        const use_alt = if (colon) {
                            val_opt != null && val_opt.?.len > 0
                        } else {
                            val_opt != null
                        };
                        if (use_alt) {
                            if (depth < MAX_EXPAND_DEPTH) {
                                const sub = expandImpl(arg, env, out, out_pos, depth + 1);
                                if (!sub.ok) {
                                    return sub;
                                }
                                out_pos = sub.len;
                            } else {
                                if (out_pos + arg.len > out.len) {
                                    return ExpandResult{ ok: false, err: ExpandError.BufferOverflow, len: out_pos, missing_var: "" };
                                }
                                let k: usize = 0;
                                while (k < arg.len) {
                                    out[out_pos + k] = arg[k];
                                    k += 1;
                                }
                                out_pos += arg.len;
                            }
                        }
                        i = close_idx + 1;
                        continue;
                    }

                    if (op == '?') {
                        const is_missing = if (colon) {
                            val_opt == null || val_opt.?.len == 0
                        } else {
                            val_opt == null
                        };
                        if (is_missing) {
                            return ExpandResult{
                                ok: false,
                                err: ExpandError.MissingRequired,
                                len: out_pos,
                                missing_var: var_name,
                            };
                        }
                        const val = val_opt.?;
                        if (out_pos + val.len > out.len) {
                            return ExpandResult{ ok: false, err: ExpandError.BufferOverflow, len: out_pos, missing_var: "" };
                        }
                        let k: usize = 0;
                        while (k < val.len) {
                            out[out_pos + k] = val[k];
                            k += 1;
                        }
                        out_pos += val.len;
                        i = close_idx + 1;
                        continue;
                    }
                }

                // Fallback: unrecognized brace syntax -> emit verbatim
                const raw_len = (close_idx + 1) - i;
                if (out_pos + raw_len > out.len) {
                    return ExpandResult{ ok: false, err: ExpandError.BufferOverflow, len: out_pos, missing_var: "" };
                }
                let k: usize = 0;
                while (k < raw_len) {
                    out[out_pos + k] = tpl[i + k];
                    k += 1;
                }
                out_pos += raw_len;
                i = close_idx + 1;
                continue;
            }

            // Bare variable: $VAR
            if (i + 1 < tpl.len && isVarStart(tpl[i + 1])) {
                let end = i + 2;
                while (end < tpl.len && isVarCont(tpl[end])) {
                    end += 1;
                }
                const var_name = tpl[(i + 1)..end];
                const val_opt = env.get(var_name);
                if (val_opt != null) {
                    const val = val_opt.?;
                    if (out_pos + val.len > out.len) {
                        return ExpandResult{ ok: false, err: ExpandError.BufferOverflow, len: out_pos, missing_var: "" };
                    }
                    let k: usize = 0;
                    while (k < val.len) {
                        out[out_pos + k] = val[k];
                        k += 1;
                    }
                    out_pos += val.len;
                }
                i = end;
                continue;
            }
        }

        // Literal character
        if (out_pos >= out.len) {
            return ExpandResult{ ok: false, err: ExpandError.BufferOverflow, len: out_pos, missing_var: "" };
        }
        out[out_pos] = tpl[i];
        out_pos += 1;
        i += 1;
    }

    return ExpandResult{ ok: true, err: ExpandError.None, len: out_pos, missing_var: "" };
}

/// Expands POSIX environment variable placeholders in `tpl` into caller-provided `out`.
pub fn expandEnv(tpl: string, env: EnvMap, out: []u8) -> ExpandResult {
    return expandImpl(tpl, env, out, 0, 0);
}

/// Convenience wrapper returning the expanded slice `out[0..res.len]`.
pub fn expandEnvStr(tpl: string, env: EnvMap, out: []u8) -> string {
    const res = expandEnv(tpl, env, out);
    return out[0..res.len];
}

/// Returns `true` if `tpl` contains at least one unescaped `$VAR` or `${...}` placeholder.
pub fn hasEnvVars(tpl: string) -> bool {
    let i: usize = 0;
    while (i < tpl.len) {
        if (tpl[i] == '\\' && i + 1 < tpl.len && tpl[i + 1] == '$') {
            i += 2;
            continue;
        }
        if (tpl[i] == '$') {
            if (i + 1 < tpl.len && tpl[i + 1] == '$') {
                i += 2;
                continue;
            }
            if (i + 1 < tpl.len && (tpl[i + 1] == '{' || isVarStart(tpl[i + 1]))) {
                return true;
            }
        }
        i += 1;
    }
    return false;
}
