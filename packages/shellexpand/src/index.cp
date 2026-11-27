// @cap/shellexpand — Zero-allocation shell-like tilde & env variable expansion in pure CP
// Clean-room MIT implementation of crates.io `shellexpand` v3.

pub struct EnvEntry {
    pub key: string,
    pub value: string,
}

fn sexpIsVarStart(b: u8): bool {
    return (b >= 65 && b <= 90) || (b >= 97 && b <= 122) || b == 95;
}

fn sexpIsVarChar(b: u8): bool {
    return sexpIsVarStart(b) || (b >= 48 && b <= 57);
}

fn sexpStrEq(a: string, b: string): bool {
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

fn sexpLookup(vars: []const EnvEntry, key: string): string? {
    let i: usize = 0;
    while (i < vars.len) {
        if (sexpStrEq(vars[i].key, key)) {
            return vars[i].value;
        }
        i += 1;
    }
    return null;
}

fn sexpAppend(out: []u8, pos: usize, s: string): usize {
    let p = pos;
    let i: usize = 0;
    while (i < s.len && p < out.len) {
        out[p] = s[i];
        p += 1;
        i += 1;
    }
    return p;
}

/// Expands leading `~` or `~/...` (and `~\...`) using `home_dir`.
/// Leaves `~user/...` untouched (matching `shellexpand::tilde`).
pub fn tildeWithHome(input: string, home_dir: string, out: []u8): usize {
    if (input.len == 0) {
        return 0;
    }
    if (input[0] == 126) { // '~'
        if (input.len == 1) {
            return sexpAppend(out, 0, home_dir);
        }
        let next = input[1];
        if (next == 47 || next == 92) { // '/' or '\'
            let p = sexpAppend(out, 0, home_dir);
            // Avoid double slash if home_dir already ends with '/'
            let rest_start: usize = 1;
            if (home_dir.len > 0 && (home_dir[home_dir.len - 1] == 47 || home_dir[home_dir.len - 1] == 92)) {
                rest_start = 2;
            }
            return sexpAppend(out, p, input[rest_start..input.len]);
        }
    }
    return sexpAppend(out, 0, input);
}

/// Expands `$VAR`, `${VAR}`, `${VAR:-default}`, and `$$` (literal `$`) using `vars`.
/// Unknown variables without a default expand to empty string (or remain literal if `keep_unknown` is true).
pub fn envExpand(input: string, vars: []const EnvEntry, keep_unknown: bool, out: []u8): usize {
    let i: usize = 0;
    let p: usize = 0;
    while (i < input.len && p < out.len) {
        if (input[i] != 36) { // '$'
            out[p] = input[i];
            p += 1;
            i += 1;
            continue;
        }
        // Escape `$$` -> `$`
        if (i + 1 < input.len && input[i + 1] == 36) {
            out[p] = 36;
            p += 1;
            i += 2;
            continue;
        }
        // Braced form: `${VAR}` or `${VAR:-default}`
        if (i + 1 < input.len && input[i + 1] == 123) { // '{'
            let close = i + 2;
            while (close < input.len && input[close] != 125) { // '}'
                close += 1;
            }
            if (close < input.len && input[close] == 125) {
                const body = input[(i + 2)..close];
                // Check for `:-` default separator
                let sep_idx: usize = body.len;
                let k: usize = 0;
                while (k + 1 < body.len) {
                    if (body[k] == 58 && body[k + 1] == 45) { // ':-'
                        sep_idx = k;
                        break;
                    }
                    k += 1;
                }
                const var_name = body[0..sep_idx];
                const has_default = sep_idx < body.len;
                if (val := sexpLookup(vars, var_name)) {
                    if (has_default && val.len == 0) {
                        p = sexpAppend(out, p, body[(sep_idx + 2)..body.len]);
                    } else {
                        p = sexpAppend(out, p, val);
                    }
                } else if (has_default) {
                    p = sexpAppend(out, p, body[(sep_idx + 2)..body.len]);
                } else if (keep_unknown) {
                    p = sexpAppend(out, p, input[i..(close + 1)]);
                }
                i = close + 1;
                continue;
            }
        }
        // Unbraced form: `$VAR`
        if (i + 1 < input.len && sexpIsVarStart(input[i + 1])) {
            let end = i + 2;
            while (end < input.len && sexpIsVarChar(input[end])) {
                end += 1;
            }
            const var_name = input[(i + 1)..end];
            if (val := sexpLookup(vars, var_name)) {
                p = sexpAppend(out, p, val);
            } else if (keep_unknown) {
                p = sexpAppend(out, p, input[i..end]);
            }
            i = end;
            continue;
        }
        // Standalone `$`
        out[p] = 36;
        p += 1;
        i += 1;
    }
    return p;
}

/// Full shell expansion: first expands leading `~` with `home_dir`, then expands `$VAR` / `${VAR}`.
pub fn fullExpand(input: string, home_dir: string, vars: []const EnvEntry, out: []u8): usize {
    let tmp: [4096]u8 = undefined;
    const tlen = tildeWithHome(input, home_dir, tmp[0..4096]);
    return envExpand(tmp[0..tlen], vars, false, out);
}

export {
    EnvEntry,
    tildeWithHome,
    envExpand,
    fullExpand,
};
