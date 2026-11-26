// @cap/test-exclude — Zero-heap test and coverage file path glob filter engine.

pub struct ExcludeConfig {
    includes: []const string,
    excludes: []const string,
    extensions: []const string,
}

fn texcHasPrefix(s: string, prefix: string): bool {
    if (s.len < prefix.len) return false;
    let i: usize = 0;
    while (i < prefix.len) {
        if (s[i] != prefix[i]) return false;
        i += 1;
    }
    return true;
}

fn texcHasSuffix(s: string, suffix: string): bool {
    if (s.len < suffix.len) return false;
    const offset = s.len - suffix.len;
    let i: usize = 0;
    while (i < suffix.len) {
        if (s[offset + i] != suffix[i]) return false;
        i += 1;
    }
    return true;
}

fn texcContainsSegment(path: string, seg: string): bool {
    if (path.len < seg.len) return false;
    let start: usize = 0;
    while (start < path.len) {
        let end = start;
        while (end < path.len && path[end] != '/') {
            end += 1;
        }
        const cur = path[start..end];
        if (cur.len == seg.len) {
            let match_ok: bool = true;
            let k: usize = 0;
            while (k < seg.len) {
                if (cur[k] != seg[k]) {
                    match_ok = false;
                    break;
                }
                k += 1;
            }
            if (match_ok) return true;
        }
        if (end >= path.len) break;
        start = end + 1;
    }
    return false;
}

fn texcMatchAt(pat: string, pi: usize, text: string, ti: usize): bool {
    if (pi == pat.len) {
        return ti == text.len;
    }

    if (pi + 1 < pat.len && pat[pi] == '*' && pat[pi + 1] == '*') {
        let next_pi = pi + 2;
        while (next_pi < pat.len && pat[next_pi] == '*') {
            next_pi += 1;
        }
        if (next_pi < pat.len && pat[next_pi] == '/') {
            if (texcMatchAt(pat, next_pi + 1, text, ti)) {
                return true;
            }
            if (ti < text.len && texcMatchAt(pat, pi, text, ti + 1)) {
                return true;
            }
            return false;
        }
        if (texcMatchAt(pat, next_pi, text, ti)) {
            return true;
        }
        if (ti < text.len && texcMatchAt(pat, pi, text, ti + 1)) {
            return true;
        }
        return false;
    }

    if (pat[pi] == '*') {
        if (texcMatchAt(pat, pi + 1, text, ti)) {
            return true;
        }
        if (ti < text.len && text[ti] != '/' && texcMatchAt(pat, pi, text, ti + 1)) {
            return true;
        }
        return false;
    }

    if (pat[pi] == '?') {
        if (ti < text.len && text[ti] != '/') {
            return texcMatchAt(pat, pi + 1, text, ti + 1);
        }
        return false;
    }

    if (ti < text.len && pat[pi] == text[ti]) {
        return texcMatchAt(pat, pi + 1, text, ti + 1);
    }
    return false;
}

fn texcMatchRaw(pat: string, path: string): bool {
    return texcMatchAt(pat, 0, path, 0);
}

pub fn matchSimpleGlob(pattern: string, path: string): bool {
    if (pattern.len > 0 && pattern[0] == '!') {
        return !texcMatchRaw(pattern[1..pattern.len], path);
    }
    return texcMatchRaw(pattern, path);
}

fn texcMatchesExt(path: string, ext: string): bool {
    if (ext.len == 0) return false;
    if (ext[0] == '.') {
        return texcHasSuffix(path, ext);
    }
    if (path.len <= ext.len) return false;
    if (path[path.len - ext.len - 1] != '.') return false;
    return texcHasSuffix(path, ext);
}

pub fn shouldInstrument(rel_path: string, cfg: ExcludeConfig): bool {
    let p = rel_path;
    while (texcHasPrefix(p, "./")) {
        p = p[2..p.len];
    }
    if (p.len == 0) return false;

    // Always exclude node_modules and .git directories
    if (texcContainsSegment(p, "node_modules") || texcContainsSegment(p, ".git")) {
        return false;
    }

    // Extension whitelist
    if (cfg.extensions.len > 0) {
        let ext_ok: bool = false;
        let i: usize = 0;
        while (i < cfg.extensions.len) {
            if (texcMatchesExt(p, cfg.extensions[i])) {
                ext_ok = true;
                break;
            }
            i += 1;
        }
        if (!ext_ok) return false;
    }

    // Includes rules
    if (cfg.includes.len > 0) {
        let has_pos: bool = false;
        let matched_pos: bool = false;
        let i: usize = 0;
        while (i < cfg.includes.len) {
            const pat = cfg.includes[i];
            if (pat.len > 0 && pat[0] == '!') {
                if (texcMatchRaw(pat[1..pat.len], p)) {
                    return false;
                }
            } else {
                has_pos = true;
                if (texcMatchRaw(pat, p)) {
                    matched_pos = true;
                }
            }
            i += 1;
        }
        if (has_pos && !matched_pos) return false;
    }

    // Excludes rules (supporting !re-include)
    if (cfg.excludes.len > 0) {
        let excluded: bool = false;
        let i: usize = 0;
        while (i < cfg.excludes.len) {
            const pat = cfg.excludes[i];
            if (pat.len > 0 && pat[0] == '!') {
                if (texcMatchRaw(pat[1..pat.len], p)) {
                    excluded = false;
                }
            } else {
                if (texcMatchRaw(pat, p)) {
                    excluded = true;
                }
            }
            i += 1;
        }
        if (excluded) return false;
    }

    return true;
}
