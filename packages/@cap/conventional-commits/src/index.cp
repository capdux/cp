// @cap/conventional-commits — Zero-allocation Conventional Commits 1.0.0 parser & Semver engine
// Clean-room MIT implementation inspired by npm `conventional-commits-parser`
// and Rust `git-conventional`.

pub const MAX_FOOTERS: usize = 16;

pub enum SemverBump: u8 {
    None = 0,
    Patch = 1,
    Minor = 2,
    Major = 3,
}

pub struct CommitFooter {
    pub token: string,
    pub value: string,
    pub is_breaking: bool,

    pub static fn empty() -> CommitFooter {
        return CommitFooter{
            token: "",
            value: "",
            is_breaking: false,
        };
    }
}

fn toLowerAscii(c: u8) -> u8 {
    if (c >= 'A' && c <= 'Z') {
        return c + 32;
    }
    return c;
}

fn eqIgnoreCase(a: string, b: string) -> bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (toLowerAscii(a[i]) != toLowerAscii(b[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

pub struct ParsedCommit {
    pub valid: bool,
    pub commit_type: string,
    pub scope: string,
    pub description: string,
    pub body: string,
    pub is_breaking: bool,
    pub breaking_note: string,
    pub footers: [16]CommitFooter,
    pub footer_count: usize,

    pub static fn invalid() -> ParsedCommit {
        let footers: [16]CommitFooter = undefined;
        let i: usize = 0;
        while (i < MAX_FOOTERS) {
            footers[i] = CommitFooter.empty();
            i += 1;
        }
        return ParsedCommit{
            valid: false,
            commit_type: "",
            scope: "",
            description: "",
            body: "",
            is_breaking: false,
            breaking_note: "",
            footers: footers,
            footer_count: 0,
        };
    }

    pub fn footerAt(i: usize) -> CommitFooter? {
        if (i < _.footer_count) {
            return _.footers[i];
        }
        return null;
    }

    pub fn getFooter(token: string) -> string? {
        let i: usize = 0;
        while (i < _.footer_count) {
            if (eqIgnoreCase(_.footers[i].token, token)) {
                return _.footers[i].value;
            }
            i += 1;
        }
        return null;
    }
}

// ---------------------------------------------------------------------------
// Internal Parsing Helpers
// ---------------------------------------------------------------------------

fn isAlpha(c: u8) -> bool {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

fn isAlphaNum(c: u8) -> bool {
    return isAlpha(c) || (c >= '0' && c <= '9');
}

fn isTypeChar(c: u8) -> bool {
    return isAlphaNum(c) || c == '-' || c == '_';
}

fn isFooterTokenChar(c: u8) -> bool {
    return isAlphaNum(c) || c == '-';
}

fn isSpace(c: u8) -> bool {
    return c == ' ' || c == '\t';
}

fn isLineBlank(s: string, start: usize, end: usize) -> bool {
    let i = start;
    while (i < end) {
        const c = s[i];
        if (c != ' ' && c != '\t' && c != '\r') {
            return false;
        }
        i += 1;
    }
    return true;
}

fn trimLineEnd(s: string, start: usize, end: usize) -> usize {
    let e = end;
    while (e > start && (s[e - 1] == ' ' || s[e - 1] == '\t' || s[e - 1] == '\r')) {
        e -= 1;
    }
    return e;
}

fn trimBlockEdges(s: string, start: usize, end: usize) -> string {
    let a = start;
    let b = end;
    while (a < b && (s[a] == '\n' || s[a] == '\r')) {
        a += 1;
    }
    while (b > a && (s[b - 1] == '\n' || s[b - 1] == '\r' || s[b - 1] == ' ' || s[b - 1] == '\t')) {
        b -= 1;
    }
    return s[a..b];
}

struct FooterLineMatch {
    matched: bool,
    token: string,
    val_start: usize,
    is_breaking: bool,
}

fn matchFooterLine(msg: string, line_start: usize, line_end: usize) -> FooterLineMatch {
    const empty_match = FooterLineMatch{
        matched: false,
        token: "",
        val_start: line_start,
        is_breaking: false,
    };
    const e = trimLineEnd(msg, line_start, line_end);
    if (e <= line_start) {
        return empty_match;
    }

    // Special token: "BREAKING CHANGE: <value>" or "BREAKING CHANGE #<value>"
    const bc = "BREAKING CHANGE";
    if (e - line_start > bc.len && eqIgnoreCase(msg[line_start..(line_start + bc.len)], bc)) {
        const sep_pos = line_start + bc.len;
        if (sep_pos + 1 < e && msg[sep_pos] == ':' && isSpace(msg[sep_pos + 1])) {
            let v = sep_pos + 2;
            while (v < e && isSpace(msg[v])) {
                v += 1;
            }
            return FooterLineMatch{
                matched: true,
                token: msg[line_start..sep_pos],
                val_start: v,
                is_breaking: true,
            };
        }
        if (sep_pos + 2 <= e && msg[sep_pos] == ' ' && msg[sep_pos + 1] == '#') {
            return FooterLineMatch{
                matched: true,
                token: msg[line_start..sep_pos],
                val_start: sep_pos + 2,
                is_breaking: true,
            };
        }
    }

    // Standard token: <word-with-hyphens>: <value>  OR  <word-with-hyphens> #<value>
    let i = line_start;
    if (!isAlpha(msg[i])) {
        return empty_match;
    }
    while (i < e && isFooterTokenChar(msg[i])) {
        i += 1;
    }
    const token = msg[line_start..i];
    if (token.len == 0 || i >= e) {
        return empty_match;
    }

    const breaking = eqIgnoreCase(token, "BREAKING-CHANGE");

    if (msg[i] == ':' && i + 1 < e && isSpace(msg[i + 1])) {
        let v = i + 2;
        while (v < e && isSpace(msg[v])) {
            v += 1;
        }
        return FooterLineMatch{
            matched: true,
            token: token,
            val_start: v,
            is_breaking: breaking,
        };
    }

    if (msg[i] == ' ' && i + 1 < e && msg[i + 1] == '#') {
        return FooterLineMatch{
            matched: true,
            token: token,
            val_start: i + 2,
            is_breaking: breaking,
        };
    }

    return empty_match;
}

// ---------------------------------------------------------------------------
// Public API: Parser & Validator
// ---------------------------------------------------------------------------

/// Parses a Conventional Commits 1.0.0 commit message with zero heap allocation.
pub fn parseCommit(msg: string) -> ParsedCommit {
    let res = ParsedCommit.invalid();
    if (msg.len == 0) {
        return res;
    }

    // 1. Extract first line (header)
    let nl: usize = 0;
    while (nl < msg.len && msg[nl] != '\n') {
        nl += 1;
    }
    const hdr_end = trimLineEnd(msg, 0 as usize, nl);
    if (hdr_end == 0) {
        return res;
    }

    // 2. Parse <type>[(<scope>)][!]: <description>
    let p: usize = 0;
    if (!isAlpha(msg[p])) {
        return res;
    }
    while (p < hdr_end && isTypeChar(msg[p])) {
        p += 1;
    }
    const commit_type = msg[0..p];
    if (commit_type.len == 0 || p >= hdr_end) {
        return res;
    }

    let scope: string = "";
    if (msg[p] == '(') {
        p += 1;
        const sc_start = p;
        while (p < hdr_end && msg[p] != ')') {
            p += 1;
        }
        if (p >= hdr_end || p == sc_start) {
            return res;
        }
        scope = msg[sc_start..p];
        p += 1; // consume ')'
    }

    let is_breaking = false;
    if (p < hdr_end && msg[p] == '!') {
        is_breaking = true;
        p += 1;
    }

    // Must be followed by ": <description>"
    if (p + 1 >= hdr_end || msg[p] != ':' || !isSpace(msg[p + 1])) {
        return res;
    }
    p += 2;
    while (p < hdr_end && isSpace(msg[p])) {
        p += 1;
    }
    if (p >= hdr_end) {
        return res;
    }
    const description = msg[p..hdr_end];

    res.valid = true;
    res.commit_type = commit_type;
    res.scope = scope;
    res.description = description;
    res.is_breaking = is_breaking;

    // 3. Parse optional body & footers after header
    if (nl >= msg.len) {
        return res;
    }

    // Advance past header '\n' and any blank separator lines
    let rest_start = nl + 1;
    while (rest_start < msg.len) {
        let le = rest_start;
        while (le < msg.len && msg[le] != '\n') {
            le += 1;
        }
        if (isLineBlank(msg, rest_start, le)) {
            rest_start = if (le < msg.len) le + 1 else msg.len;
        } else {
            break;
        }
    }
    if (rest_start >= msg.len) {
        return res;
    }

    // Trim trailing blank lines from msg
    let rest_end = msg.len;
    while (rest_end > rest_start && (msg[rest_end - 1] == '\n' || msg[rest_end - 1] == '\r' || isSpace(msg[rest_end - 1]))) {
        rest_end -= 1;
    }

    // Locate the start of the last paragraph (after the last blank line in [rest_start, rest_end))
    let last_para_start = rest_start;
    let cur = rest_start;
    while (cur < rest_end) {
        let le = cur;
        while (le < rest_end && msg[le] != '\n') {
            le += 1;
        }
        if (isLineBlank(msg, cur, le)) {
            let next_line = if (le < rest_end) le + 1 else rest_end;
            while (next_line < rest_end) {
                let nle = next_line;
                while (nle < rest_end && msg[nle] != '\n') {
                    nle += 1;
                }
                if (isLineBlank(msg, next_line, nle)) {
                    next_line = if (nle < rest_end) nle + 1 else rest_end;
                } else {
                    break;
                }
            }
            if (next_line < rest_end) {
                last_para_start = next_line;
            }
            cur = next_line;
        } else {
            cur = if (le < rest_end) le + 1 else rest_end;
        }
    }

    // Check if the last paragraph starts with a valid Conventional Commits footer
    let first_line_end = last_para_start;
    while (first_line_end < rest_end && msg[first_line_end] != '\n') {
        first_line_end += 1;
    }
    const first_match = matchFooterLine(msg, last_para_start, first_line_end);

    if (!first_match.matched) {
        res.body = trimBlockEdges(msg, rest_start, rest_end);
        return res;
    }

    // Anything before `last_para_start` is the body
    if (last_para_start > rest_start) {
        res.body = trimBlockEdges(msg, rest_start, last_para_start);
    }

    // Parse all footers in [last_para_start, rest_end), supporting multiline values
    let f_cur = last_para_start;
    let active = false;
    let active_token: string = "";
    let active_val_start: usize = 0;
    let active_val_end: usize = 0;
    let active_breaking = false;

    while (f_cur < rest_end) {
        let le = f_cur;
        while (le < rest_end && msg[le] != '\n') {
            le += 1;
        }
        const m = matchFooterLine(msg, f_cur, le);
        if (m.matched) {
            if (active && res.footer_count < MAX_FOOTERS) {
                const val = msg[active_val_start..trimLineEnd(msg, active_val_start, active_val_end)];
                res.footers[res.footer_count] = CommitFooter{
                    token: active_token,
                    value: val,
                    is_breaking: active_breaking,
                };
                res.footer_count += 1;
                if (active_breaking) {
                    res.is_breaking = true;
                    if (res.breaking_note.len == 0) {
                        res.breaking_note = val;
                    }
                }
            }
            active = true;
            active_token = m.token;
            active_val_start = m.val_start;
            active_val_end = trimLineEnd(msg, m.val_start, le);
            active_breaking = m.is_breaking;
        } else if (active) {
            // Continuation line for the current footer value
            active_val_end = trimLineEnd(msg, active_val_start, le);
        }
        f_cur = if (le < rest_end) le + 1 else rest_end;
    }

    if (active && res.footer_count < MAX_FOOTERS) {
        const val = msg[active_val_start..trimLineEnd(msg, active_val_start, active_val_end)];
        res.footers[res.footer_count] = CommitFooter{
            token: active_token,
            value: val,
            is_breaking: active_breaking,
        };
        res.footer_count += 1;
        if (active_breaking) {
            res.is_breaking = true;
            if (res.breaking_note.len == 0) {
                res.breaking_note = val;
            }
        }
    }

    return res;
}

/// Returns `true` if `header` is a valid single-line Conventional Commits 1.0.0 header.
pub fn isValidHeader(header: string) -> bool {
    let i: usize = 0;
    while (i < header.len) {
        if (header[i] == '\n' || header[i] == '\r') {
            return false;
        }
        i += 1;
    }
    return parseCommit(header).valid;
}

// ---------------------------------------------------------------------------
// Public API: Semver Bump Recommendation & Formatting
// ---------------------------------------------------------------------------

/// Recommends the Semver bump level for a parsed commit per Conventional Commits 1.0.0:
/// - Breaking change -> `Major`
/// - `feat` -> `Minor`
/// - `fix` / `perf` / `revert` -> `Patch`
/// - Everything else / invalid -> `None`
pub fn recommendedBump(commit: ParsedCommit) -> SemverBump {
    if (!commit.valid) {
        return SemverBump.None;
    }
    if (commit.is_breaking) {
        return SemverBump.Major;
    }
    if (eqIgnoreCase(commit.commit_type, "feat")) {
        return SemverBump.Minor;
    }
    if (eqIgnoreCase(commit.commit_type, "fix")
        || eqIgnoreCase(commit.commit_type, "perf")
        || eqIgnoreCase(commit.commit_type, "revert"))
    {
        return SemverBump.Patch;
    }
    return SemverBump.None;
}

fn bumpRank(b: SemverBump) -> u8 {
    switch (b) {
        .None => return 0,
        .Patch => return 1,
        .Minor => return 2,
        .Major => return 3,
    }
}

/// Returns the higher Semver bump level between `a` and `b`.
pub fn highestBump(a: SemverBump, b: SemverBump) -> SemverBump {
    if (bumpRank(a) >= bumpRank(b)) {
        return a;
    }
    return b;
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

/// Formats a `ParsedCommit` back into a canonical `<type>[(<scope>)][!]: <description>` header in `out`.
pub fn formatCommitHeader(commit: ParsedCommit, out: []u8) -> string {
    if (!commit.valid) {
        return out[0..0];
    }
    let pos: usize = 0;
    pos = appendStr(out, pos, commit.commit_type);
    if (commit.scope.len > 0) {
        pos = appendByte(out, pos, '(');
        pos = appendStr(out, pos, commit.scope);
        pos = appendByte(out, pos, ')');
    }
    if (commit.is_breaking) {
        pos = appendByte(out, pos, '!');
    }
    pos = appendStr(out, pos, ": ");
    pos = appendStr(out, pos, commit.description);
    return out[0..pos];
}
