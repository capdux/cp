// Source: Absorbed from /data/dev/bun/src/install/PnpmMatcher.cp | Synced: 2026-11-22T02:32:12Z
// Pure-CP zero-allocation PNPM-compatible package name matcher with include/exclude
// 3-state semantics and linear-time wildcard (`*`, `?`) matching.

pub enum MatcherBehavior: u8 {
    all_include = 0,
    all_exclude = 1,
    mixed = 2,
}

pub struct PackagePattern {
    pattern: []const u8,
    is_exclude: bool,
    match_all: bool,
}

/// Zero-allocation package name matcher (up to 32 rules) following pnpm `@pnpm/matcher` rules:
/// - All include rules: matches iff any include rule matches.
/// - All exclude rules: matches iff NO exclude rule matches.
/// - Mixed include & exclude rules: matches iff at least one include matches AND no exclude matches.
pub struct PackageMatcher {
    rules: [32]PackagePattern,
    len: usize,
    behavior: MatcherBehavior,

    pub static fn init(): PackageMatcher {
        return PackageMatcher{
            rules: [PackagePattern{ pattern: "", is_exclude: false, match_all: false }; 32],
            len: 0,
            behavior: .all_include,
        };
    }

    /// Build a `PackageMatcher` from a slice of pattern strings (e.g. `["@babel/*", "!@babel/core"]`).
    pub static fn fromPatterns(patterns: []const []const u8): PackageMatcher {
        let m = PackageMatcher.init();
        let i: usize = 0;
        while (i < patterns.len) : (i += 1) {
            if (!m.addPattern(patterns[i])) break;
        }
        return m;
    }

    /// Add a single pattern string (leading `!` marks exclude; `*` matches everything).
    pub fn addPattern(*_, raw: []const u8): bool {
        if (_.len >= 32) return false;
        let s = trimAscii(raw);
        if (s.len == 0) return false;

        let is_exclude = false;
        if (s[0] == '!') {
            is_exclude = true;
            s = trimAscii(s[1..]);
            if (s.len == 0) return false;
        }

        const is_all = s.len == 1 && s[0] == '*';
        _.rules[_.len] = PackagePattern{
            pattern: s,
            is_exclude: is_exclude,
            match_all: is_all,
        };
        _.len += 1;
        _.recomputeBehavior();
        return true;
    }

    fn recomputeBehavior(*_) {
        let has_inc = false;
        let has_exc = false;
        let i: usize = 0;
        while (i < _.len) : (i += 1) {
            if (_.rules[i].is_exclude) {
                has_exc = true;
            } else {
                has_inc = true;
            }
        }
        if (!has_inc) {
            _.behavior = .all_exclude;
        } else if (!has_exc) {
            _.behavior = .all_include;
        } else {
            _.behavior = .mixed;
        }
    }

    /// Evaluate whether `name` matches this matcher.
    pub fn isMatch(name: []const u8): bool {
        if (_.len == 0) return false;
        switch (_.behavior) {
            .all_include => {
                let i: usize = 0;
                while (i < _.len) : (i += 1) {
                    if (ruleMatches(_.rules[i], name)) return true;
                }
                return false;
            },
            .all_exclude => {
                let i: usize = 0;
                while (i < _.len) : (i += 1) {
                    if (ruleMatches(_.rules[i], name)) return false;
                }
                return true;
            },
            .mixed => {
                let matched_inc = false;
                let i: usize = 0;
                while (i < _.len) : (i += 1) {
                    const r = _.rules[i];
                    if (r.is_exclude) {
                        if (ruleMatches(r, name)) return false;
                    } else if (!matched_inc) {
                        if (ruleMatches(r, name)) matched_inc = true;
                    }
                }
                return matched_inc;
            },
        }
    }
}

fn ruleMatches(r: PackagePattern, name: []const u8): bool {
    if (r.match_all) return true;
    return wildcardMatch(r.pattern, name);
}

/// Linear-time wildcard matcher supporting `*` (0+ chars) and `?` (1 char).
pub fn wildcardMatch(pattern: []const u8, text: []const u8): bool {
    let pi: usize = 0;
    let ti: usize = 0;
    let star_pi: usize? = null;
    let star_ti: usize = 0;

    while (ti < text.len) {
        if (pi < pattern.len && (pattern[pi] == '?' || pattern[pi] == text[ti])) {
            pi += 1;
            ti += 1;
        } else if (pi < pattern.len && pattern[pi] == '*') {
            star_pi = pi;
            star_ti = ti;
            pi += 1;
        } else if (sp := star_pi) {
            pi = sp + 1;
            star_ti += 1;
            ti = star_ti;
        } else {
            return false;
        }
    }

    while (pi < pattern.len && pattern[pi] == '*') : (pi += 1) {}
    return pi == pattern.len;
}

fn trimAscii(s: []const u8): []const u8 {
    let start: usize = 0;
    while (start < s.len && (s[start] == ' ' || s[start] == '\t' || s[start] == '\n' || s[start] == '\r')) : (start += 1) {}
    let end = s.len;
    while (end > start && (s[end - 1] == ' ' || s[end - 1] == '\t' || s[end - 1] == '\n' || s[end - 1] == '\r')) : (end -= 1) {}
    return s[start..end];
}
