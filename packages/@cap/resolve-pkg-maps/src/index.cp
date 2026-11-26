// @cap/resolve-pkg-maps — Zero-heap package.json exports subpath & condition resolver
// Clean-room CP implementation inspired by resolve-pkg-maps (MIT).

pub struct ExportRule {
    pattern: string,
    condition: string,
    target: string,
}

fn rpmEq(a: string, b: string): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn rpmFindStar(s: string): usize {
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        if (s[i] == '*') return i;
    }
    return s.len;
}

fn rpmHasParentTraversal(path: string): bool {
    let i: usize = 0;
    while (i < path.len) {
        const seg_start: usize = i;
        while (i < path.len && path[i] != '/' && path[i] != '\\') : (i += 1) {}
        const seg_len: usize = i - seg_start;
        if (seg_len == 2 && path[seg_start] == '.' && path[seg_start + 1] == '.') {
            return true;
        }
        if (i < path.len) {
            i += 1;
        }
    }
    return false;
}

fn rpmConditionRank(cond: string, conditions: []const string): usize {
    let i: usize = 0;
    while (i < conditions.len) : (i += 1) {
        if (rpmEq(cond, conditions[i])) return i;
    }
    if (cond.len == 0 || rpmEq(cond, "default")) {
        return conditions.len;
    }
    return conditions.len + 1000;
}

pub fn matchSubpathPattern(pattern: string, subpath: string, matched_star: []u8): i64 {
    const star_idx: usize = rpmFindStar(pattern);
    if (star_idx == pattern.len) {
        if (rpmEq(pattern, subpath)) return 0;
        return -1;
    }

    const prefix_len: usize = star_idx;
    const suffix_start: usize = star_idx + 1;
    const suffix_len: usize = pattern.len - suffix_start;

    if (subpath.len < prefix_len + suffix_len) return -1;

    let i: usize = 0;
    while (i < prefix_len) : (i += 1) {
        if (subpath[i] != pattern[i]) return -1;
    }

    const sub_suffix_start: usize = subpath.len - suffix_len;
    i = 0;
    while (i < suffix_len) : (i += 1) {
        if (subpath[sub_suffix_start + i] != pattern[suffix_start + i]) return -1;
    }

    const star_len: usize = subpath.len - prefix_len - suffix_len;
    const copy_len: usize = if (star_len < matched_star.len) star_len else matched_star.len;
    i = 0;
    while (i < copy_len) : (i += 1) {
        matched_star[i] = subpath[prefix_len + i];
    }
    return i64(star_len);
}

pub fn resolveExportTarget(
    rules: []const ExportRule,
    subpath: string,
    conditions: []const string,
    out: []u8,
): usize {
    if (rpmHasParentTraversal(subpath)) return 0;

    const no_match_rank: usize = conditions.len + 1000;
    let best_idx: usize = rules.len;
    let best_is_exact: bool = false;
    let best_prefix_len: usize = 0;
    let best_pattern_len: usize = 0;
    let best_cond_rank: usize = no_match_rank;
    let best_star_len: usize = 0;
    let best_star_buf: [256]u8 = [0 as u8; 256];
    let tmp_star_buf: [256]u8 = [0 as u8; 256];

    let i: usize = 0;
    while (i < rules.len) : (i += 1) {
        const rule = rules[i];
        const cond_rank: usize = rpmConditionRank(rule.condition, conditions);
        if (cond_rank >= no_match_rank) continue;

        const m_len: i64 = matchSubpathPattern(rule.pattern, subpath, tmp_star_buf[0..256]);
        if (m_len < 0) continue;

        const star_idx: usize = rpmFindStar(rule.pattern);
        const is_exact: bool = star_idx == rule.pattern.len;
        const prefix_len: usize = star_idx;
        const pat_len: usize = rule.pattern.len;

        let better: bool = false;
        if (best_idx == rules.len) {
            better = true;
        } else if (is_exact && !best_is_exact) {
            better = true;
        } else if (is_exact == best_is_exact) {
            if (prefix_len > best_prefix_len) {
                better = true;
            } else if (prefix_len == best_prefix_len) {
                if (pat_len > best_pattern_len) {
                    better = true;
                } else if (pat_len == best_pattern_len && cond_rank < best_cond_rank) {
                    better = true;
                }
            }
        }

        if (better) {
            best_idx = i;
            best_is_exact = is_exact;
            best_prefix_len = prefix_len;
            best_pattern_len = pat_len;
            best_cond_rank = cond_rank;
            const slen: usize = usize(m_len);
            best_star_len = if (slen < 256) slen else 256;
            let k: usize = 0;
            while (k < best_star_len) : (k += 1) {
                best_star_buf[k] = tmp_star_buf[k];
            }
        }
    }

    if (best_idx == rules.len) return 0;

    const target: string = rules[best_idx].target;
    if (rpmHasParentTraversal(target)) return 0;

    let pos: usize = 0;
    let ti: usize = 0;
    while (ti < target.len) : (ti += 1) {
        if (target[ti] == '*') {
            let si: usize = 0;
            while (si < best_star_len && pos < out.len) : (si += 1) {
                out[pos] = best_star_buf[si];
                pos += 1;
            }
        } else {
            if (pos < out.len) {
                out[pos] = target[ti];
                pos += 1;
            }
        }
    }

    if (rpmHasParentTraversal(out[0..pos] as string)) return 0;
    return pos;
}
