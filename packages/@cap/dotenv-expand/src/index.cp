// @cap/dotenv-expand — Zero-allocation .env Variable Interpolation Engine
// Clean-room MIT implementation in pure CP.
// Supports $VAR, ${VAR}, ${VAR:-default}, ${VAR-default}, ${VAR:+alt}, nested defaults,
// escaped \$, and cycle-safe bounded recursive expansion (MAX_DEPTH = 8).

pub const MAX_DEPTH: usize = 8;

pub struct EnvVar {
    pub key: []const u8,
    pub val: []const u8,
}

fn sliceEq(a: []const u8, b: []const u8) -> bool {
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

fn isIdentStart(b: u8) -> bool {
    if (b >= 65 && b <= 90) { return true; }  // A-Z
    if (b >= 97 && b <= 122) { return true; } // a-z
    return b == 95;                           // '_'
}

fn isIdentChar(b: u8) -> bool {
    if (isIdentStart(b)) { return true; }
    return b >= 48 && b <= 57;                // 0-9
}

/// Returns true if `key` is defined in `vars`.
pub fn hasVar(vars: []const EnvVar, key: []const u8) -> bool {
    let i: usize = 0;
    while (i < vars.len) {
        if (sliceEq(vars[i].key, key)) {
            return true;
        }
        i += 1;
    }
    return false;
}

/// Returns the value associated with `key` in `vars`, or `""` if not found.
pub fn lookupVar(vars: []const EnvVar, key: []const u8) -> []const u8 {
    let i: usize = 0;
    while (i < vars.len) {
        if (sliceEq(vars[i].key, key)) {
            return vars[i].val;
        }
        i += 1;
    }
    return "";
}

fn expandRec(tmpl: []const u8, vars: []const EnvVar, dst: []u8, start_out: usize, depth: usize) -> i64 {
    if (depth > MAX_DEPTH) {
        return start_out as i64;
    }

    let i: usize = 0;
    let out = start_out;
    const n = tmpl.len;

    while (i < n) {
        const ch = tmpl[i];

        // Escaped dollar: \$ -> literal $
        if (ch == 92 && i + 1 < n && tmpl[i + 1] == 36) {
            if (out >= dst.len) {
                return -1;
            }
            dst[out] = 36;
            out += 1;
            i += 2;
            continue;
        }

        if (ch == 36) {
            // '$'
            if (i + 1 < n && tmpl[i + 1] == 123) {
                // "${...}"
                const body_start = i + 2;
                let scan = body_start;
                let brace_depth: usize = 1;
                while (scan < n && brace_depth > 0) {
                    if (tmpl[scan] == 92 && scan + 1 < n) {
                        scan += 2;
                        continue;
                    }
                    if (tmpl[scan] == 36 && scan + 1 < n && tmpl[scan + 1] == 123) {
                        brace_depth += 1;
                        scan += 2;
                        continue;
                    }
                    if (tmpl[scan] == 125) {
                        brace_depth -= 1;
                        if (brace_depth == 0) {
                            break;
                        }
                    }
                    scan += 1;
                }
                if (brace_depth != 0) {
                    // Unclosed "${"
                    return -1;
                }
                const body_end = scan;

                // Parse variable name inside ${...}
                let name_end = body_start;
                if (name_end < body_end && isIdentStart(tmpl[name_end])) {
                    name_end += 1;
                    while (name_end < body_end && isIdentChar(tmpl[name_end])) {
                        name_end += 1;
                    }
                }
                if (name_end == body_start) {
                    return -1;
                }

                const name = tmpl[body_start..name_end];
                const defined = hasVar(vars, name);
                const val = lookupVar(vars, name);

                if (name_end == body_end) {
                    // ${VAR}
                    if (defined && val.len > 0) {
                        const next_out = expandRec(val, vars, dst, out, depth + 1);
                        if (next_out < 0) {
                            return -1;
                        }
                        out = next_out as usize;
                    }
                } else if (name_end + 1 < body_end && tmpl[name_end] == 58 && tmpl[name_end + 1] == 45) {
                    // ${VAR:-default}: use default if unset OR empty
                    if (!defined || val.len == 0) {
                        const fallback = tmpl[name_end + 2..body_end];
                        const next_out = expandRec(fallback, vars, dst, out, depth + 1);
                        if (next_out < 0) {
                            return -1;
                        }
                        out = next_out as usize;
                    } else {
                        const next_out = expandRec(val, vars, dst, out, depth + 1);
                        if (next_out < 0) {
                            return -1;
                        }
                        out = next_out as usize;
                    }
                } else if (tmpl[name_end] == 45) {
                    // ${VAR-default}: use default ONLY if unset
                    if (!defined) {
                        const fallback = tmpl[name_end + 1..body_end];
                        const next_out = expandRec(fallback, vars, dst, out, depth + 1);
                        if (next_out < 0) {
                            return -1;
                        }
                        out = next_out as usize;
                    } else if (val.len > 0) {
                        const next_out = expandRec(val, vars, dst, out, depth + 1);
                        if (next_out < 0) {
                            return -1;
                        }
                        out = next_out as usize;
                    }
                } else if (name_end + 1 < body_end && tmpl[name_end] == 58 && tmpl[name_end + 1] == 43) {
                    // ${VAR:+alt}: use alt if set AND non-empty
                    if (defined && val.len > 0) {
                        const alt = tmpl[name_end + 2..body_end];
                        const next_out = expandRec(alt, vars, dst, out, depth + 1);
                        if (next_out < 0) {
                            return -1;
                        }
                        out = next_out as usize;
                    }
                } else if (tmpl[name_end] == 43) {
                    // ${VAR+alt}: use alt if set (even if empty)
                    if (defined) {
                        const alt = tmpl[name_end + 1..body_end];
                        const next_out = expandRec(alt, vars, dst, out, depth + 1);
                        if (next_out < 0) {
                            return -1;
                        }
                        out = next_out as usize;
                    }
                } else {
                    return -1;
                }

                i = body_end + 1;
                continue;
            }

            if (i + 1 < n && isIdentStart(tmpl[i + 1])) {
                // "$VAR"
                const name_start = i + 1;
                let name_end = name_start + 1;
                while (name_end < n && isIdentChar(tmpl[name_end])) {
                    name_end += 1;
                }
                const name = tmpl[name_start..name_end];
                if (hasVar(vars, name)) {
                    const val = lookupVar(vars, name);
                    if (val.len > 0) {
                        const next_out = expandRec(val, vars, dst, out, depth + 1);
                        if (next_out < 0) {
                            return -1;
                        }
                        out = next_out as usize;
                    }
                }
                i = name_end;
                continue;
            }

            // Standalone '$'
            if (out >= dst.len) {
                return -1;
            }
            dst[out] = 36;
            out += 1;
            i += 1;
            continue;
        }

        if (out >= dst.len) {
            return -1;
        }
        dst[out] = ch;
        out += 1;
        i += 1;
    }

    return out as i64;
}

/// Expands environment variable references in `template` using `vars` into `dst`.
/// Returns the number of bytes written, or `-1` on syntax error (unclosed `${`) or insufficient buffer.
pub fn expandValue(template: []const u8, vars: []const EnvVar, dst: []u8) -> i64 {
    return expandRec(template, vars, dst, 0, 0);
}
