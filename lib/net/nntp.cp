// Origin: Pure-CP Stdlib (Zero-Alloc RFC 3977 NNTP Wire Protocol Codec) | Created: 2026-11-22

// Standard NNTP TCP Ports & Protocol Limits (RFC 3977 / RFC 8143)
pub const NNTP_TCP_PORT: u16 = 119;
pub const NNTPS_TCP_PORT: u16 = 563;
pub const NNTP_MAX_CMD_LEN: usize = 512;

// Standard NNTP Status Codes (RFC 3977)
pub const CODE_READY_POST: u16 = 200;
pub const CODE_READY_NOPOST: u16 = 201;
pub const CODE_CLOSING: u16 = 205;
pub const CODE_GROUP_OK: u16 = 211;
pub const CODE_LIST_FOLLOWS: u16 = 215;
pub const CODE_ARTICLE_FOLLOWS: u16 = 220;
pub const CODE_HEAD_FOLLOWS: u16 = 221;
pub const CODE_BODY_FOLLOWS: u16 = 222;
pub const CODE_STAT_OK: u16 = 223;
pub const CODE_OVER_FOLLOWS: u16 = 224;
pub const CODE_SEND_ARTICLE: u16 = 340;
pub const CODE_NO_SUCH_GROUP: u16 = 411;
pub const CODE_NO_SUCH_ARTICLE: u16 = 430;
pub const CODE_SYNTAX_ERROR: u16 = 500;

fn stripCrlf(line: []const u8): []const u8 {
    let end = line.len;
    while (end > 0 && (line[end - 1] == '\r' || line[end - 1] == '\n')) {
        end -= 1;
    }
    return line[0..end];
}

fn isSpaceOrTab(c: u8): bool {
    return c == ' ' || c == '\t';
}

fn containsCrlfOrNul(s: []const u8): bool {
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] == '\r' || s[i] == '\n' || s[i] == 0) {
            return true;
        }
        i += 1;
    }
    return false;
}

fn isValidVerb(verb: []const u8): bool {
    if (verb.len == 0) {
        return false;
    }
    let i: usize = 0;
    while (i < verb.len) {
        const c = verb[i];
        const is_alpha = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
        const is_digit = c >= '0' && c <= '9';
        if (!is_alpha && !is_digit && c != '-') {
            return false;
        }
        i += 1;
    }
    return true;
}

fn parseU64Dec(s: []const u8): u64? {
    if (s.len == 0) {
        return null;
    }
    let acc: u64 = 0;
    let i: usize = 0;
    while (i < s.len) {
        const c = s[i];
        if (c < '0' || c > '9') {
            return null;
        }
        const digit = (c - '0') as u64;
        if (acc > (18446744073709551615 - digit) / 10) {
            return null;
        }
        acc = acc * 10 + digit;
        i += 1;
    }
    return acc;
}

fn isValidMessageId(s: []const u8): bool {
    if (s.len < 3) {
        return false;
    }
    if (s[0] != '<' || s[s.len - 1] != '>') {
        return false;
    }
    let i: usize = 1;
    while (i + 1 < s.len) {
        const c = s[i];
        if (c <= 0x20 || c >= 0x7F || c == '<' || c == '>') {
            return false;
        }
        i += 1;
    }
    return true;
}

fn isValidGroupName(s: []const u8): bool {
    if (s.len == 0) {
        return false;
    }
    let i: usize = 0;
    while (i < s.len) {
        const c = s[i];
        if (c <= 0x20 || c >= 0x7F) {
            return false;
        }
        i += 1;
    }
    return true;
}

struct TokenSlice {
    token: []const u8,
    rest: []const u8,
}

fn nextToken(input: []const u8): TokenSlice? {
    let i: usize = 0;
    while (i < input.len && isSpaceOrTab(input[i])) {
        i += 1;
    }
    if (i >= input.len) {
        return null;
    }
    const start = i;
    while (i < input.len && !isSpaceOrTab(input[i])) {
        i += 1;
    }
    return TokenSlice{
        .token = input[start..i],
        .rest = input[i..],
    };
}

struct TabField {
    field: []const u8,
    rest: []const u8,
    has_more: bool,
}

fn nextTabField(input: []const u8): TabField {
    let i: usize = 0;
    while (i < input.len && input[i] != '\t') {
        i += 1;
    }
    if (i < input.len) {
        return TabField{
            .field = input[0..i],
            .rest = input[(i + 1)..],
            .has_more = true,
        };
    }
    return TabField{
        .field = input,
        .rest = input[input.len..],
        .has_more = false,
    };
}

fn writeBytes(out: []u8, pos: usize, src: []const u8): usize? {
    if (pos + src.len > out.len) {
        return null;
    }
    let i: usize = 0;
    while (i < src.len) {
        out[pos + i] = src[i];
        i += 1;
    }
    return pos + src.len;
}

/// RFC 3977 §3.2 single status response line (`DDD [text]`).
pub struct NntpResponse {
    code: u16,
    text: []const u8,

    fn isInfo(): bool {
        return _.code >= 100 && _.code <= 199;
    }

    fn isOk(): bool {
        return _.code >= 200 && _.code <= 299;
    }

    fn isContinue(): bool {
        return _.code >= 300 && _.code <= 399;
    }

    fn isTempError(): bool {
        return _.code >= 400 && _.code <= 499;
    }

    fn isPermError(): bool {
        return _.code >= 500 && _.code <= 599;
    }
}

/// Parses an RFC 3977 NNTP status response line (`DDD [text]\r\n`).
/// Rejects malformed lines, non-digit codes, out-of-range codes (< 100 or > 599),
/// or missing whitespace separator between the 3-digit code and trailing text.
pub fn parseNntpResponse(line: []const u8): NntpResponse? {
    const s = stripCrlf(line);
    if (s.len < 3) {
        return null;
    }
    const c0 = s[0];
    const c1 = s[1];
    const c2 = s[2];
    if (c0 < '1' || c0 > '5' || c1 < '0' || c1 > '9' || c2 < '0' || c2 > '9') {
        return null;
    }
    const code: u16 = ((c0 - '0') as u16) * 100 + ((c1 - '0') as u16) * 10 + ((c2 - '0') as u16);
    if (s.len == 3) {
        return NntpResponse{
            .code = code,
            .text = s[3..3],
        };
    }
    if (!isSpaceOrTab(s[3])) {
        return null;
    }
    return NntpResponse{
        .code = code,
        .text = s[4..],
    };
}

/// RFC 3977 §6.1.1 `GROUP` 211 response payload (`number low high group`).
pub struct NntpGroupInfo {
    count: u64,
    low: u64,
    high: u64,
    name: []const u8,
}

/// Parses the response text of an NNTP `211` `GROUP` reply (`count low high name`).
pub fn parseNntpGroup(resp_text: []const u8): NntpGroupInfo? {
    const s = stripCrlf(resp_text);
    const t_count = nextToken(s) ?? return null;
    const t_low = nextToken(t_count.rest) ?? return null;
    const t_high = nextToken(t_low.rest) ?? return null;
    const t_name = nextToken(t_high.rest) ?? return null;

    const count = parseU64Dec(t_count.token) ?? return null;
    const low = parseU64Dec(t_low.token) ?? return null;
    const high = parseU64Dec(t_high.token) ?? return null;
    if (!isValidGroupName(t_name.token)) {
        return null;
    }

    return NntpGroupInfo{
        .count = count,
        .low = low,
        .high = high,
        .name = t_name.token,
    };
}

/// RFC 3977 §6.2 `ARTICLE` / `HEAD` / `BODY` / `STAT` response payload (`n <message-id>`).
pub struct NntpArticleStat {
    number: u64,
    message_id: []const u8,
}

/// Parses the response text of `220/221/222/223` article replies (`number <message-id>`).
pub fn parseNntpArticleStat(resp_text: []const u8): NntpArticleStat? {
    const s = stripCrlf(resp_text);
    const t_num = nextToken(s) ?? return null;
    const t_id = nextToken(t_num.rest) ?? return null;

    const number = parseU64Dec(t_num.token) ?? return null;
    if (!isValidMessageId(t_id.token)) {
        return null;
    }

    return NntpArticleStat{
        .number = number,
        .message_id = t_id.token,
    };
}

/// RFC 3977 §8.3.2 TAB-delimited 8-field `OVER` / `XOVER` overview record.
pub struct NntpOverview {
    article_num: u64,
    subject: []const u8,
    from: []const u8,
    date: []const u8,
    message_id: []const u8,
    references: []const u8,
    bytes: u64,
    lines: u64,
}

/// Parses an RFC 3977 §8.3.2 TAB-delimited `OVER`/`XOVER` overview line.
/// Requires at least 8 TAB-separated fields; optional trailing extension fields are ignored.
pub fn parseNntpOverview(line: []const u8): NntpOverview? {
    const s = stripCrlf(line);

    const f0 = nextTabField(s);
    if (!f0.has_more) {
        return null;
    }
    const f1 = nextTabField(f0.rest);
    if (!f1.has_more) {
        return null;
    }
    const f2 = nextTabField(f1.rest);
    if (!f2.has_more) {
        return null;
    }
    const f3 = nextTabField(f2.rest);
    if (!f3.has_more) {
        return null;
    }
    const f4 = nextTabField(f3.rest);
    if (!f4.has_more) {
        return null;
    }
    const f5 = nextTabField(f4.rest);
    if (!f5.has_more) {
        return null;
    }
    const f6 = nextTabField(f5.rest);
    if (!f6.has_more) {
        return null;
    }
    const f7 = nextTabField(f6.rest);

    const article_num = parseU64Dec(f0.field) ?? return null;
    if (!isValidMessageId(f4.field)) {
        return null;
    }
    const bytes = parseU64Dec(f6.field) ?? return null;
    const lines = parseU64Dec(f7.field) ?? return null;

    return NntpOverview{
        .article_num = article_num,
        .subject = f1.field,
        .from = f2.field,
        .date = f3.field,
        .message_id = f4.field,
        .references = f5.field,
        .bytes = bytes,
        .lines = lines,
    };
}

/// Zero-copy iterator over RFC 3977 §3.1.1 CRLF-delimited multi-line data blocks.
/// Stops at the terminating `.\r\n` line and performs zero-copy dot-unstuffing
/// on lines beginning with `..`.
pub struct NntpMultiLineIter {
    data: []const u8,
    pos: usize,
    terminated: bool,

    pub static fn init(data: []const u8): NntpMultiLineIter {
        return NntpMultiLineIter{
            .data = data,
            .pos = 0,
            .terminated = false,
        };
    }

    fn next(*_): ([]const u8)? {
        if (_.terminated || _.pos >= _.data.len) {
            return null;
        }
        // Scan for RFC 3977 CRLF line terminator
        let i = _.pos;
        while (i + 1 < _.data.len) {
            if (_.data[i] == '\r' && _.data[i + 1] == '\n') {
                const raw_line = _.data[_.pos..i];
                _.pos = i + 2;

                // Standalone "." terminates the multi-line block
                if (raw_line.len == 1 && raw_line[0] == '.') {
                    _.terminated = true;
                    return null;
                }

                // RFC 3977 §3.1.1 dot-unstuffing: strip leading '.' from ".." lines
                if (raw_line.len >= 2 && raw_line[0] == '.' && raw_line[1] == '.') {
                    return raw_line[1..];
                }

                return raw_line;
            }
            i += 1;
        }
        return null;
    }

    fn isTerminated(): bool {
        return _.terminated;
    }

    fn bytesConsumed(): usize {
        return _.pos;
    }
}

/// Zero-alloc encoder for an RFC 3977 NNTP command line (`VERB\r\n` or `VERB ARG\r\n`).
/// Enforces `NNTP_MAX_CMD_LEN` (512 bytes) and rejects CR/LF/NUL injection or invalid verbs.
pub fn encodeNntpCommand(verb: []const u8, arg: []const u8, out: []u8): usize? {
    if (!isValidVerb(verb) || containsCrlfOrNul(arg)) {
        return null;
    }
    let extra: usize = 2;
    if (arg.len > 0) {
        extra = arg.len + 3;
    }
    if (verb.len > NNTP_MAX_CMD_LEN || extra > NNTP_MAX_CMD_LEN - verb.len) {
        return null;
    }
    const total = verb.len + extra;
    if (total > out.len) {
        return null;
    }
    let pos = writeBytes(out, 0, verb) ?? return null;
    if (arg.len > 0) {
        pos = writeBytes(out, pos, " ") ?? return null;
        pos = writeBytes(out, pos, arg) ?? return null;
    }
    pos = writeBytes(out, pos, "\r\n") ?? return null;
    return pos;
}
