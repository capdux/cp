// @cap/tsconfig-paths — Zero-allocation tsconfig.json baseUrl + paths resolver for CP
// Clean-room MIT implementation inspired by npm `tsconfig-paths`.

pub const MAX_TARGETS: usize = 4;
pub const MAX_RULES: usize = 32;

pub struct PathRule {
    pub pattern: string,
    pub targets: [4]string,
    pub target_count: usize,

    pub static fn empty() -> PathRule {
        return PathRule{
            pattern: "",
            targets: ["", "", "", ""],
            target_count: 0,
        };
    }
}

pub struct PatternMatch {
    pub matched: bool,
    pub is_exact: bool,
    pub prefix_len: usize,
    pub captured: string,

    pub static fn none() -> PatternMatch {
        return PatternMatch{
            matched: false,
            is_exact: false,
            prefix_len: 0,
            captured: "",
        };
    }
}

pub struct CandidateList {
    pub items: [4]string,
    pub count: usize,

    pub static fn empty() -> CandidateList {
        return CandidateList{
            items: ["", "", "", ""],
            count: 0,
        };
    }

    pub fn itemAt(i: usize) -> string? {
        if (i < _.count) {
            return _.items[i];
        }
        return null;
    }
}

fn eqStr(a: string, b: string) -> bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

pub struct PathsMatcher {
    pub base_url: string,
    pub rules: [32]PathRule,
    pub rule_count: usize,

    pub static fn init(base_url: string) -> PathsMatcher {
        let rules: [32]PathRule = undefined;
        let i: usize = 0;
        while (i < MAX_RULES) {
            rules[i] = PathRule.empty();
            i += 1;
        }
        return PathsMatcher{
            base_url: base_url,
            rules: rules,
            rule_count: 0,
        };
    }

    pub fn addRule(*_, pattern: string, target: string) {
        if (pattern.len == 0 || target.len == 0) {
            return;
        }
        // Append to existing rule if pattern already registered
        let i: usize = 0;
        while (i < _.rule_count) {
            if (eqStr(_.rules[i].pattern, pattern)) {
                if (_.rules[i].target_count < MAX_TARGETS) {
                    _.rules[i].targets[_.rules[i].target_count] = target;
                    _.rules[i].target_count += 1;
                }
                return;
            }
            i += 1;
        }
        if (_.rule_count < MAX_RULES) {
            let rule = PathRule.empty();
            rule.pattern = pattern;
            rule.targets[0] = target;
            rule.target_count = 1;
            _.rules[_.rule_count] = rule;
            _.rule_count += 1;
        }
    }

    pub fn addRuleMulti(*_, pattern: string, t1: string, t2: string) {
        _.addRule(pattern, t1);
        _.addRule(pattern, t2);
    }
}

// ---------------------------------------------------------------------------
// Pattern Matching (TypeScript `paths` Specification)
// ---------------------------------------------------------------------------

fn startsWith(s: string, prefix: string) -> bool {
    if (s.len < prefix.len) return false;
    let i: usize = 0;
    while (i < prefix.len) {
        if (s[i] != prefix[i]) return false;
        i += 1;
    }
    return true;
}

fn endsWith(s: string, suffix: string) -> bool {
    if (s.len < suffix.len) return false;
    const offset = s.len - suffix.len;
    let i: usize = 0;
    while (i < suffix.len) {
        if (s[offset + i] != suffix[i]) return false;
        i += 1;
    }
    return true;
}

fn findStar(s: string) -> usize? {
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] == '*') {
            return i;
        }
        i += 1;
    }
    return null;
}

/// Matches a module `specifier` against a tsconfig `paths` `pattern` (exact or single `*` wildcard).
pub fn matchesPattern(pattern: string, specifier: string) -> PatternMatch {
    if (pattern.len == 0 || specifier.len == 0) {
        return PatternMatch.none();
    }
    const star_opt = findStar(pattern);
    if (star_opt == null) {
        if (eqStr(pattern, specifier)) {
            return PatternMatch{
                matched: true,
                is_exact: true,
                prefix_len: pattern.len,
                captured: "",
            };
        }
        return PatternMatch.none();
    }

    const star = star_opt.?;
    const prefix = pattern[0..star];
    const suffix = pattern[(star + 1)..pattern.len];
    if (specifier.len < prefix.len + suffix.len) {
        return PatternMatch.none();
    }
    if (!startsWith(specifier, prefix) || !endsWith(specifier, suffix)) {
        return PatternMatch.none();
    }
    const cap_end = specifier.len - suffix.len;
    return PatternMatch{
        matched: true,
        is_exact: false,
        prefix_len: prefix.len,
        captured: specifier[prefix.len..cap_end],
    };
}

// ---------------------------------------------------------------------------
// Path Expansion & Normalization (Zero Heap Allocation)
// ---------------------------------------------------------------------------

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

struct SegSpan {
    start: usize,
    end: usize,
}

/// Normalizes `raw` (collapsing `//`, stripping redundant `.` segments, resolving `..`)
/// and writes the result into `out[start_pos..]`, returning the new write cursor.
fn writeNormalizedPath(raw: string, out: []u8, start_pos: usize) -> usize {
    if (raw.len == 0) {
        return start_pos;
    }
    const is_abs = (raw[0] == '/');

    let segs: [32]SegSpan = undefined;
    let seg_count: usize = 0;

    let i: usize = 0;
    while (i < raw.len) {
        while (i < raw.len && raw[i] == '/') {
            i += 1;
        }
        const s_start = i;
        while (i < raw.len && raw[i] != '/') {
            i += 1;
        }
        if (i <= s_start) {
            break;
        }
        const seg = raw[s_start..i];
        if (eqStr(seg, ".")) {
            continue;
        }
        if (eqStr(seg, "..")) {
            if (seg_count > 0) {
                const prev = raw[segs[seg_count - 1].start..segs[seg_count - 1].end];
                if (!eqStr(prev, "..")) {
                    seg_count -= 1;
                    continue;
                }
            }
            if (!is_abs && seg_count < 32) {
                segs[seg_count] = SegSpan{ start: s_start, end: i };
                seg_count += 1;
            }
            continue;
        }
        if (seg_count < 32) {
            segs[seg_count] = SegSpan{ start: s_start, end: i };
            seg_count += 1;
        }
    }

    let pos = start_pos;
    if (is_abs) {
        pos = appendByte(out, pos, '/');
    }
    let k: usize = 0;
    while (k < seg_count) {
        if (k > 0) {
            pos = appendByte(out, pos, '/');
        }
        pos = appendStr(out, pos, raw[segs[k].start..segs[k].end]);
        k += 1;
    }
    if (pos == start_pos) {
        pos = appendByte(out, pos, '.');
    }
    return pos;
}

fn expandTargetInto(
    base_url: string,
    target: string,
    captured: string,
    out: []u8,
    start_pos: usize,
) -> usize {
    // Stage 1: assemble raw joined path in a stack scratch buffer
    let scratch: [512]u8 = undefined;
    let sp: usize = 0;

    const target_is_abs = (target.len > 0 && target[0] == '/');
    if (!target_is_abs && base_url.len > 0 && !eqStr(base_url, ".") && !eqStr(base_url, "./")) {
        sp = appendStr(scratch[0..512], sp, base_url);
        sp = appendByte(scratch[0..512], sp, '/');
    }

    const star_opt = findStar(target);
    if (star_opt != null) {
        const star = star_opt.?;
        sp = appendStr(scratch[0..512], sp, target[0..star]);
        sp = appendStr(scratch[0..512], sp, captured);
        sp = appendStr(scratch[0..512], sp, target[(star + 1)..target.len]);
    } else {
        sp = appendStr(scratch[0..512], sp, target);
    }

    // Stage 2: normalize into caller's `out[start_pos..]`
    return writeNormalizedPath(scratch[0..sp], out, start_pos);
}

struct BestRuleHit {
    found: bool,
    rule_idx: usize,
    match_info: PatternMatch,
}

fn findBestRule(matcher: PathsMatcher, specifier: string) -> BestRuleHit {
    let best = BestRuleHit{
        found: false,
        rule_idx: 0,
        match_info: PatternMatch.none(),
    };

    let i: usize = 0;
    while (i < matcher.rule_count) {
        const m = matchesPattern(matcher.rules[i].pattern, specifier);
        if (m.matched) {
            let take = false;
            if (!best.found) {
                take = true;
            } else if (m.is_exact && !best.match_info.is_exact) {
                take = true;
            } else if (!m.is_exact && !best.match_info.is_exact && m.prefix_len > best.match_info.prefix_len) {
                take = true;
            }
            if (take) {
                best = BestRuleHit{
                    found: true,
                    rule_idx: i,
                    match_info: m,
                };
            }
        }
        i += 1;
    }

    return best;
}

// ---------------------------------------------------------------------------
// Public Resolution API
// ---------------------------------------------------------------------------

/// Resolves `specifier` using the highest-priority matching rule in `matcher`,
/// returning the primary normalized path written into `out`, or `null` if unmatched.
pub fn resolvePath(matcher: PathsMatcher, specifier: string, out: []u8) -> string? {
    const hit = findBestRule(matcher, specifier);
    if (!hit.found) {
        return null;
    }
    const rule = matcher.rules[hit.rule_idx];
    if (rule.target_count == 0) {
        return null;
    }
    const end_pos = expandTargetInto(
        matcher.base_url,
        rule.targets[0],
        hit.match_info.captured,
        out,
        0 as usize,
    );
    return out[0..end_pos];
}

/// Resolves `specifier` into all fallback target candidates of the winning rule,
/// storing each candidate consecutively in `out_buf` with zero heap allocation.
pub fn resolveAllCandidates(matcher: PathsMatcher, specifier: string, out_buf: []u8) -> CandidateList {
    let list = CandidateList.empty();
    const hit = findBestRule(matcher, specifier);
    if (!hit.found) {
        return list;
    }
    const rule = matcher.rules[hit.rule_idx];
    let pos: usize = 0;
    let t: usize = 0;
    while (t < rule.target_count && list.count < MAX_TARGETS) {
        const start = pos;
        const end = expandTargetInto(
            matcher.base_url,
            rule.targets[t],
            hit.match_info.captured,
            out_buf,
            start,
        );
        list.items[list.count] = out_buf[start..end];
        list.count += 1;
        pos = end;
        t += 1;
    }
    return list;
}
