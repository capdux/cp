// Source: Absorbed from /data/dev/bun/src/resolver/tsconfig_json.cp | Synced: 2026-11-21T13:55:16Z
// Pure-CP zero-allocation TSConfig `paths` wildcard resolver & `${configDir}`
// template expander (longest-prefix match + target substitution).

pub struct PathMapping {
    /// Pattern key, e.g. `@/*` or `exact-alias` (at most one `*`).
    pattern: []const u8,
    /// Up to 4 fallback target templates, e.g. `["src/*", "generated/*"]`.
    targets: [4][]const u8,
    target_len: usize,
}

pub struct ResolvedCandidate {
    /// Byte length written into caller buffer (or path slice).
    path: []const u8,
}

/// Expand all `${configDir}` placeholders in `input` with `config_dir`, writing into `out`.
/// Returns the expanded slice, or null if `out` is too small.
pub fn expandConfigDir(input: []const u8, config_dir: []const u8, out: []u8): ([]const u8)? {
    const needle = "${configDir}";
    let pos: usize = 0;
    let out_pos: usize = 0;
    while (pos < input.len) {
        if (pos + needle.len <= input.len && strEq(input[pos .. pos + needle.len], needle)) {
            if (out_pos + config_dir.len > out.len) return null;
            let k: usize = 0;
            while (k < config_dir.len) : (k += 1) {
                out[out_pos] = config_dir[k];
                out_pos += 1;
            }
            pos += needle.len;
        } else {
            if (out_pos >= out.len) return null;
            out[out_pos] = input[pos];
            out_pos += 1;
            pos += 1;
        }
    }
    return out[0..out_pos];
}

/// Zero-allocation TSConfig `compilerOptions.paths` table (up to 32 rules).
pub struct TsConfigPaths {
    base_url: []const u8,
    rules: [32]PathMapping,
    len: usize,

    pub static fn init(base_url: []const u8): TsConfigPaths {
        return TsConfigPaths{
            base_url: base_url,
            rules: [PathMapping{ pattern: "", targets: [""; 4], target_len: 0 }; 32],
            len: 0,
        };
    }

    /// Add a path mapping rule (validates at most one `*` in `pattern`).
    pub fn addRule(*_, pattern: []const u8, targets: []const []const u8): bool {
        if (_.len >= 32 || pattern.len == 0 || targets.len == 0) return false;
        // Count '*' in pattern (TSConfig spec allows at most one '*')
        let stars: usize = 0;
        let i: usize = 0;
        while (i < pattern.len) : (i += 1) {
            if (pattern[i] == '*') stars += 1;
        }
        if (stars > 1) return false;

        let tbuf: [4][]const u8 = [""; 4];
        const count = @min(targets.len, 4);
        let k: usize = 0;
        while (k < count) : (k += 1) {
            tbuf[k] = targets[k];
        }
        _.rules[_.len] = PathMapping{
            pattern: pattern,
            targets: tbuf,
            target_len: count,
        };
        _.len += 1;
        return true;
    }

    /// Resolve `specifier` against registered `paths` rules following TypeScript's
    /// exact-match-first, then longest-wildcard-prefix rule.
    /// Writes the `target_idx`-th candidate (0-based) into `out` (prefixed with `base_url` if set).
    pub fn resolveNth(specifier: []const u8, target_idx: usize, out: []u8): ([]const u8)? {
        // 1. Exact match first (no '*')
        let i: usize = 0;
        while (i < _.len) : (i += 1) {
            const r = _.rules[i];
            if (findStar(r.pattern) == null && strEq(r.pattern, specifier)) {
                if (target_idx >= r.target_len) return null;
                return joinAndSubstitute(_.base_url, r.targets[target_idx], "", out);
            }
        }

        // 2. Wildcard match with longest prefix (per TypeScript module resolution spec)
        let best_idx: usize? = null;
        let best_prefix_len: usize = 0;
        let best_matched_mid: []const u8 = "";

        let j: usize = 0;
        while (j < _.len) : (j += 1) {
            const r = _.rules[j];
            if (star := findStar(r.pattern)) {
                const prefix = r.pattern[0..star];
                const suffix = r.pattern[star + 1 ..];
                if (specifier.len >= prefix.len + suffix.len &&
                    strEq(specifier[0..prefix.len], prefix) &&
                    strEq(specifier[specifier.len - suffix.len ..], suffix))
                {
                    if (best_idx == null || prefix.len > best_prefix_len) {
                        best_idx = j;
                        best_prefix_len = prefix.len;
                        best_matched_mid = specifier[prefix.len .. specifier.len - suffix.len];
                    }
                }
            }
        }

        if (b := best_idx) {
            const r = _.rules[b];
            if (target_idx >= r.target_len) return null;
            return joinAndSubstitute(_.base_url, r.targets[target_idx], best_matched_mid, out);
        }

        return null;
    }

    /// Convenience helper resolving the first (primary) candidate for `specifier`.
    pub fn resolve(specifier: []const u8, out: []u8): ([]const u8)? {
        return _.resolveNth(specifier, 0, out);
    }
}

fn joinAndSubstitute(base_url: []const u8, target: []const u8, matched_star: []const u8, out: []u8): ([]const u8)? {
    let p: usize = 0;
    let clean_target = target;
    if (clean_target.len >= 2 && clean_target[0] == '.' && clean_target[1] == '/') {
        clean_target = clean_target[2..];
    }

    if (base_url.len > 0 && !(clean_target.len > 0 && clean_target[0] == '/')) {
        let base = base_url;
        while (base.len > 1 && base[base.len - 1] == '/') : (base = base[0 .. base.len - 1]) {}
        if (!strEq(base, ".")) {
            p = appendChecked(out, p, base) ?? return null;
            if (base[base.len - 1] != '/') {
                p = appendChecked(out, p, "/") ?? return null;
            }
        }
    }

    if (star := findStar(clean_target)) {
        const p1 = appendChecked(out, p, clean_target[0..star]) ?? return null;
        const p2 = appendChecked(out, p1, matched_star) ?? return null;
        const p3 = appendChecked(out, p2, clean_target[star + 1 ..]) ?? return null;
        p = p3;
    } else {
        const p1 = appendChecked(out, p, clean_target) ?? return null;
        p = p1;
    }
    return out[0..p];
}

fn findStar(s: []const u8): usize? {
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        if (s[i] == '*') return i;
    }
    return null;
}

fn appendChecked(out: []u8, pos: usize, s: []const u8): usize? {
    if (pos + s.len > out.len) return null;
    let p = pos;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        out[p] = s[i];
        p += 1;
    }
    return p;
}

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}
