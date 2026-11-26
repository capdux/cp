// @cap/debug — Zero-allocation namespace debug filter, deterministic color & diff formatter
// Clean-room MIT implementation inspired by npm `debug`.

const ANSI_COLORS: [6]u8 = [36, 32, 33, 34, 35, 31];

// ---------------------------------------------------------------------------
// Internal Glob & Formatting Helpers
// ---------------------------------------------------------------------------

fn isSep(c: u8) -> bool {
    return c == ',' || c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

/// Iterative zero-allocation glob matching supporting arbitrary `*` wildcards.
fn globMatch(pat: string, text: string) -> bool {
    let pi: usize = 0;
    let ti: usize = 0;
    let star_pi: usize = pat.len + 1;
    let star_ti: usize = 0;

    while (ti < text.len) {
        if (pi < pat.len && pat[pi] == '*') {
            star_pi = pi;
            star_ti = ti;
            pi += 1;
        } else if (pi < pat.len && pat[pi] == text[ti]) {
            pi += 1;
            ti += 1;
        } else if (star_pi <= pat.len) {
            pi = star_pi + 1;
            star_ti += 1;
            ti = star_ti;
        } else {
            return false;
        }
    }

    while (pi < pat.len && pat[pi] == '*') {
        pi += 1;
    }
    return pi == pat.len;
}

fn appendByte(out: []u8, pos: usize, c: u8) -> usize {
    if (pos < out.len) {
        out[pos] = c;
        return pos + 1;
    }
    return pos;
}

fn appendStr(out: []u8, pos: usize, s: string) -> usize {
    let p = pos;
    let i: usize = 0;
    while (i < s.len && p < out.len) {
        out[p] = s[i];
        p += 1;
        i += 1;
    }
    return p;
}

fn appendU64(out: []u8, pos: usize, val: u64) -> usize {
    if (val == 0) {
        return appendByte(out, pos, '0');
    }
    let digits: [24]u8 = undefined;
    let len: usize = 0;
    let v = val;
    while (v > 0) {
        digits[len] = ('0' as u64 + (v % 10)) as u8;
        v /= 10;
        len += 1;
    }
    let p = pos;
    let k = len;
    while (k > 0) {
        k -= 1;
        p = appendByte(out, p, digits[k]);
    }
    return p;
}

// ---------------------------------------------------------------------------
// Public Core API
// ---------------------------------------------------------------------------

/// Evaluates whether `namespace` is enabled by a `DEBUG`-style `pattern` string.
/// Supports comma/whitespace-separated rules, `*` wildcards, and `-` exclusion prefixes.
/// Exclusions always take precedence over inclusions.
pub fn isNamespaceEnabled(pattern: string, namespace: string) -> bool {
    if (pattern.len == 0 || namespace.len == 0) {
        return false;
    }

    let included = false;
    let i: usize = 0;
    while (i < pattern.len) {
        while (i < pattern.len && isSep(pattern[i])) {
            i += 1;
        }
        const start = i;
        while (i < pattern.len && !isSep(pattern[i])) {
            i += 1;
        }
        if (i <= start) {
            break;
        }

        const rule = pattern[start..i];
        if (rule[0] == '-') {
            const excl = rule[1..rule.len];
            if (excl.len > 0 && globMatch(excl, namespace)) {
                return false;
            }
        } else {
            if (!included && globMatch(rule, namespace)) {
                included = true;
            }
        }
    }

    return included;
}

/// Deterministically maps `namespace` to a standard ANSI SGR foreground color code (`31..36`).
pub fn selectColor(namespace: string) -> u8 {
    let hash: u64 = 0;
    let i: usize = 0;
    while (i < namespace.len) {
        hash = ((hash * 31) + (namespace[i] as u64)) % 2147483647;
        i += 1;
    }
    return ANSI_COLORS[(hash % 6) as usize];
}

/// Formats an elapsed millisecond delta into `out` (`+0ms`, `+42ms`, `+2s`, `+3m`, `+1h`).
pub fn formatDiffMs(diff_ms: i64, out: []u8) -> string {
    const ms: u64 = if (diff_ms < 0) 0 else (diff_ms as u64);
    let pos: usize = 0;
    pos = appendByte(out, pos, '+');

    if (ms < 1000) {
        pos = appendU64(out, pos, ms);
        pos = appendStr(out, pos, "ms");
    } else if (ms < 60000) {
        pos = appendU64(out, pos, ms / 1000);
        pos = appendByte(out, pos, 's');
    } else if (ms < 3600000) {
        pos = appendU64(out, pos, ms / 60000);
        pos = appendByte(out, pos, 'm');
    } else {
        pos = appendU64(out, pos, ms / 3600000);
        pos = appendByte(out, pos, 'h');
    }
    return out[0..pos];
}

/// Formats a debug log line (`ns msg +diff`) into `out`, with optional deterministic ANSI colors.
pub fn formatDebugLine(
    ns: string,
    msg: string,
    diff_ms: i64,
    use_color: bool,
    out: []u8,
) -> string {
    let diff_buf: [32]u8 = undefined;
    const diff_str = formatDiffMs(diff_ms, diff_buf[0..32]);

    let pos: usize = 0;
    if (!use_color) {
        pos = appendStr(out, pos, ns);
        pos = appendByte(out, pos, ' ');
        pos = appendStr(out, pos, msg);
        pos = appendByte(out, pos, ' ');
        pos = appendStr(out, pos, diff_str);
        return out[0..pos];
    }

    const color = selectColor(ns);
    // Bold colored namespace: \x1b[1;<color>m<ns>\x1b[0m
    pos = appendStr(out, pos, "\x1b[1;");
    pos = appendU64(out, pos, color as u64);
    pos = appendByte(out, pos, 'm');
    pos = appendStr(out, pos, ns);
    pos = appendStr(out, pos, "\x1b[0m ");

    // Message body
    pos = appendStr(out, pos, msg);
    pos = appendByte(out, pos, ' ');

    // Colored diff: \x1b[<color>m<diff>\x1b[0m
    pos = appendStr(out, pos, "\x1b[");
    pos = appendU64(out, pos, color as u64);
    pos = appendByte(out, pos, 'm');
    pos = appendStr(out, pos, diff_str);
    pos = appendStr(out, pos, "\x1b[0m");

    return out[0..pos];
}

// ---------------------------------------------------------------------------
// Public Structs: DebugFilter & DebugLogger
// ---------------------------------------------------------------------------

pub struct DebugFilter {
    pub raw_pattern: string,

    pub static fn from(pattern: string) -> DebugFilter {
        return DebugFilter{ raw_pattern: pattern };
    }

    pub fn isEnabled(namespace: string) -> bool {
        return isNamespaceEnabled(_.raw_pattern, namespace);
    }
}

pub struct DebugLogger {
    pub namespace: string,
    pub color_code: u8,
    pub last_ms: i64,
    pub enabled: bool,

    pub static fn create(pattern: string, namespace: string) -> DebugLogger {
        return DebugLogger{
            namespace: namespace,
            color_code: selectColor(namespace),
            last_ms: 0,
            enabled: isNamespaceEnabled(pattern, namespace),
        };
    }

    pub fn formatLine(msg: string, diff_ms: i64, use_color: bool, out: []u8) -> string {
        if (!_.enabled) {
            return out[0..0];
        }
        return formatDebugLine(_.namespace, msg, diff_ms, use_color, out);
    }
}
