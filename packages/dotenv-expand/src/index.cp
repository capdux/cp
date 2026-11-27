// @cap/dotenv-expand — Zero-heap `.env` file parser & POSIX variable expansion engine.
// Clean-room MIT port of npm `dotenv` + `dotenv-expand` (100M+/week).

import { isAlnum, strEq, trim, startsWith, findByte } from "cap:ascii";
import { writeStr } from "cap:fmt";

pub const DE_MAX_VARS: usize = 32;

pub struct EnvEntry {
    pub key: string,
    pub value: string,
}

pub struct EnvMap {
    pub entries: [32]EnvEntry,
    pub count: usize,

    pub static fn new(): EnvMap {
        return EnvMap { entries: undefined, count: 0 };
    }

    pub fn len(): usize { return _.count; }

    pub fn set(*_, key: string, val: string): bool {
        let i: usize = 0;
        while (i < _.count) {
            if (strEq(_.entries[i].key, key)) {
                _.entries[i].value = val;
                return true;
            }
            i = i + 1;
        }
        if (_.count >= DE_MAX_VARS) { return false; }
        _.entries[_.count] = EnvEntry { key: key, value: val };
        _.count = _.count + 1;
        return true;
    }

    pub fn get(key: string): string? {
        let i: usize = 0;
        while (i < _.count) {
            if (strEq(_.entries[i].key, key)) {
                return _.entries[i].value;
            }
            i = i + 1;
        }
        return null;
    }

    pub fn has(key: string): bool {
        return _.get(key) != null;
    }
}

fn deIsVarChar(b: u8): bool {
    return isAlnum(b) || b == 95;
}

/// Parses a `.env` document (supporting `export KEY=VAL`, `#` comments, single/double quotes,
/// inline `#` comments on unquoted values) into an `EnvMap`.
pub fn parseDotenv(src: string): EnvMap {
    let env = EnvMap.new();
    let pos: usize = 0;
    while (pos < src.len) {
        let line_end = findByte(src, 10, pos) ?? src.len;
        let line = trim(src[pos..line_end]);
        pos = if (line_end < src.len) { line_end + 1 } else { src.len };

        if (line.len == 0 || line[0] == 35) { continue; } // empty or '#' comment
        let work = line;
        if (startsWith(work, "export ")) {
            work = trim(work[7..work.len]);
        }
        let eq_opt = findByte(work, 61, 0);
        if (eq_opt == null) { continue; }
        let eq = eq_opt.?;
        if (eq == 0) { continue; }

        let key = trim(work[0..eq]);
        let raw_val = trim(work[eq + 1..work.len]);
        if (key.len == 0) { continue; }

        let val = raw_val;
        if (raw_val.len >= 2 && (raw_val[0] == 34 || raw_val[0] == 39) && raw_val[raw_val.len - 1] == raw_val[0]) {
            val = raw_val[1..raw_val.len - 1];
        } else {
            // Strip inline `#` comment for unquoted values
            let hash = findByte(raw_val, 35, 0) ?? raw_val.len;
            val = trim(raw_val[0..hash]);
        }
        env.set(key, val);
    }
    return env;
}

/// Expands `$VAR`, `${VAR}`, `${VAR:-default}`, `${VAR-default}`, and escaped `\$` in `template`
/// using `env`, writing the expanded string into `out` and returning bytes written.
pub fn expandEnvValue(template: string, env: EnvMap, out: []u8): usize {
    let p: usize = 0;
    let i: usize = 0;
    while (i < template.len) {
        if (template[i] == 92 && i + 1 < template.len && template[i + 1] == 36) {
            // Escaped "\$" -> literal "$"
            if (p < out.len) { out[p] = 36; p = p + 1; }
            i = i + 2;
            continue;
        }
        if (template[i] == 36 && i + 1 < template.len) {
            if (template[i + 1] == 123) {
                // Braced: ${VAR}, ${VAR:-default}, ${VAR-default}
                let close = findByte(template, 125, i + 2) ?? template.len;
                if (close < template.len) {
                    let inner = template[i + 2..close];
                    // Check for ":-" or "-"
                    let sep: usize = inner.len;
                    let colon_dash = false;
                    let k: usize = 0;
                    while (k < inner.len) {
                        if (inner[k] == 58 && k + 1 < inner.len && inner[k + 1] == 45) {
                            sep = k;
                            colon_dash = true;
                            break;
                        }
                        if (inner[k] == 45) {
                            sep = k;
                            break;
                        }
                        k = k + 1;
                    }
                    let var_name = inner[0..sep];
                    let has_default = sep < inner.len;
                    let def_val = if (colon_dash) { inner[sep + 2..inner.len] } else if (has_default) { inner[sep + 1..inner.len] } else { "" };
                    let found = env.get(var_name);
                    if (found != null) {
                        let fv = found ?? "";
                        if (colon_dash && fv.len == 0) {
                            p = writeStr(def_val, out, p);
                        } else {
                            p = writeStr(fv, out, p);
                        }
                    } else if (has_default) {
                        p = writeStr(def_val, out, p);
                    }
                    i = close + 1;
                    continue;
                }
            } else if (deIsVarChar(template[i + 1])) {
                // Bare $VAR
                let end = i + 1;
                while (end < template.len && deIsVarChar(template[end])) { end = end + 1; }
                let var_name = template[i + 1..end];
                let found = env.get(var_name);
                if (found != null) {
                    p = writeStr(found ?? "", out, p);
                }
                i = end;
                continue;
            }
        }
        if (p < out.len) { out[p] = template[i]; p = p + 1; }
        i = i + 1;
    }
    return p;
}
