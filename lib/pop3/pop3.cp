// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc RFC 1939 / RFC 2449 POP3 Wire Protocol Codec)

pub const POP3_TCP_PORT: u16 = 110;
pub const POP3S_TCP_PORT: u16 = 995;
pub const POP3_MAX_CMD_LEN: usize = 255;
pub const POP3_MAX_LINE_LEN: usize = 512;

fn toLowerAscii(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') {
        return c + 32;
    }
    return c;
}

fn asciiEqIgnoreCase(a: []const u8, b: []const u8): bool {
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

fn stripLineEnding(line: []const u8): []const u8 {
    let end = line.len;
    if (end >= 2 && line[end - 2] == '\r' && line[end - 1] == '\n') {
        end -= 2;
    } else if (end >= 1 && line[end - 1] == '\n') {
        end -= 1;
    }
    return line[0..end];
}

fn hasCrLfOrNul(s: []const u8): bool {
    let i: usize = 0;
    while (i < s.len) {
        const c = s[i];
        if (c == '\r' || c == '\n' || c == 0) {
            return true;
        }
        i += 1;
    }
    return false;
}

fn isValidVerbChar(c: u8): bool {
    if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-') {
        return true;
    }
    return false;
}

fn isValidRespCodeChar(c: u8): bool {
    if (c < 0x21 || c > 0x7E) {
        return false;
    }
    if (c == '[' || c == ']') {
        return false;
    }
    return true;
}

fn isValidUidChar(c: u8): bool {
    return c >= 0x21 && c <= 0x7E;
}

struct ParsedU64 {
    value: u64,
    next_pos: usize,
}

fn parseDecimalU64(s: []const u8, start: usize): ParsedU64? {
    if (start >= s.len) {
        return null;
    }
    let pos = start;
    if (s[pos] < '0' || s[pos] > '9') {
        return null;
    }
    let acc: u64 = 0;
    const max_u64: u64 = 18446744073709551615;
    while (pos < s.len) {
        const c = s[pos];
        if (c < '0' || c > '9') {
            break;
        }
        const digit = (c - '0') as u64;
        if (acc > (max_u64 - digit) / 10) {
            return null;
        }
        acc = acc * 10 + digit;
        pos += 1;
    }
    return ParsedU64{
        .value = acc,
        .next_pos = pos,
    };
}

/// Fast prefix check for POP3 status indicators (`+OK` or `-ERR`).
pub fn isPop3ResponsePrefix(data: []const u8): bool {
    if (data.len >= 3 && data[0] == '+' && data[1] == 'O' && data[2] == 'K') {
        if (data.len == 3) {
            return true;
        }
        const c3 = data[3];
        return c3 == ' ' || c3 == '\r' || c3 == '\n';
    }
    if (data.len >= 4 && data[0] == '-' && data[1] == 'E' && data[2] == 'R' && data[3] == 'R') {
        if (data.len == 4) {
            return true;
        }
        const c4 = data[4];
        return c4 == ' ' || c4 == '\r' || c4 == '\n';
    }
    return false;
}

/// Parsed POP3 single-line status response (RFC 1939 §3 & RFC 2449 §8).
pub struct Pop3Response {
    ok: bool,
    resp_code: []const u8,
    message: []const u8,

    fn isOk(): bool {
        return _.ok;
    }

    fn isErr(): bool {
        return !_.ok;
    }

    fn hasRespCode(): bool {
        return _.resp_code.len > 0;
    }
}

/// Parses a POP3 status line (`+OK` / `-ERR`) with optional RFC 2449 `[RESP-CODE]`.
pub fn parsePop3Response(line: []const u8): Pop3Response? {
    const clean = stripLineEnding(line);
    if (hasCrLfOrNul(clean)) {
        return null;
    }

    let ok = false;
    let rest_start: usize = 0;

    if (clean.len >= 3 && clean[0] == '+' && clean[1] == 'O' && clean[2] == 'K') {
        ok = true;
        if (clean.len == 3) {
            rest_start = 3;
        } else if (clean[3] == ' ') {
            rest_start = 4;
        } else {
            return null;
        }
    } else if (clean.len >= 4 && clean[0] == '-' && clean[1] == 'E' && clean[2] == 'R' && clean[3] == 'R') {
        ok = false;
        if (clean.len == 4) {
            rest_start = 4;
        } else if (clean[4] == ' ') {
            rest_start = 5;
        } else {
            return null;
        }
    } else {
        return null;
    }

    const rest = clean[rest_start..];

    // Parse optional RFC 2449 [RESP-CODE] at the start of rest
    if (rest.len > 0 && rest[0] == '[') {
        let close_pos: usize = 0;
        let found = false;
        let i: usize = 1;
        while (i < rest.len) {
            const c = rest[i];
            if (c == ']') {
                close_pos = i;
                found = true;
                break;
            }
            if (!isValidRespCodeChar(c)) {
                return null;
            }
            i += 1;
        }
        if (!found || close_pos <= 1) {
            return null;
        }
        const code = rest[1..close_pos];
        const after = close_pos + 1;
        if (after == rest.len) {
            return Pop3Response{
                .ok = ok,
                .resp_code = code,
                .message = rest[after..after],
            };
        }
        if (rest[after] != ' ') {
            return null;
        }
        return Pop3Response{
            .ok = ok,
            .resp_code = code,
            .message = rest[after + 1 ..],
        };
    }

    return Pop3Response{
        .ok = ok,
        .resp_code = clean[0..0],
        .message = rest,
    };
}

/// Parsed POP3 `STAT` response payload (`message_count total_octets`).
pub struct Pop3Stat {
    message_count: u64,
    total_octets: u64,
}

/// Parses a `STAT` response message (e.g. `"2 320"` from `Pop3Response.message`).
pub fn parsePop3Stat(resp_msg: []const u8): Pop3Stat? {
    const clean = stripLineEnding(resp_msg);
    if (clean.len == 0 || hasCrLfOrNul(clean)) {
        return null;
    }
    const first = parseDecimalU64(clean, 0) ?? return null;
    let pos = first.next_pos;
    if (pos >= clean.len || clean[pos] != ' ') {
        return null;
    }
    while (pos < clean.len && clean[pos] == ' ') {
        pos += 1;
    }
    const second = parseDecimalU64(clean, pos) ?? return null;
    if (second.next_pos < clean.len && clean[second.next_pos] != ' ') {
        return null;
    }
    return Pop3Stat{
        .message_count = first.value,
        .total_octets = second.value,
    };
}

/// Parsed POP3 `LIST` scan listing entry (`msg_num octets`).
pub struct Pop3ListItem {
    msg_num: u64,
    octets: u64,
}

/// Parses a single `LIST` scan listing line (e.g. `"1 120"`).
pub fn parsePop3ListLine(line: []const u8): Pop3ListItem? {
    const clean = stripLineEnding(line);
    if (clean.len == 0 || hasCrLfOrNul(clean)) {
        return null;
    }
    const first = parseDecimalU64(clean, 0) ?? return null;
    if (first.value == 0) {
        return null;
    }
    let pos = first.next_pos;
    if (pos >= clean.len || clean[pos] != ' ') {
        return null;
    }
    while (pos < clean.len && clean[pos] == ' ') {
        pos += 1;
    }
    const second = parseDecimalU64(clean, pos) ?? return null;
    if (second.next_pos < clean.len && clean[second.next_pos] != ' ') {
        return null;
    }
    return Pop3ListItem{
        .msg_num = first.value,
        .octets = second.value,
    };
}

/// Parsed POP3 `UIDL` unique-id listing entry (`msg_num uid`).
pub struct Pop3UidlItem {
    msg_num: u64,
    uid: []const u8,
}

/// Parses a single `UIDL` listing line (RFC 1939 §7: `msg_num SP unique-id`, UID 1..70 octets in 0x21..0x7E).
pub fn parsePop3UidlLine(line: []const u8): Pop3UidlItem? {
    const clean = stripLineEnding(line);
    if (clean.len == 0 || hasCrLfOrNul(clean)) {
        return null;
    }
    const first = parseDecimalU64(clean, 0) ?? return null;
    if (first.value == 0) {
        return null;
    }
    let pos = first.next_pos;
    if (pos >= clean.len || clean[pos] != ' ') {
        return null;
    }
    while (pos < clean.len && clean[pos] == ' ') {
        pos += 1;
    }
    if (pos >= clean.len) {
        return null;
    }
    const uid = clean[pos..];
    if (uid.len == 0 || uid.len > 70) {
        return null;
    }
    let i: usize = 0;
    while (i < uid.len) {
        if (!isValidUidChar(uid[i])) {
            return null;
        }
        i += 1;
    }
    return Pop3UidlItem{
        .msg_num = first.value,
        .uid = uid,
    };
}

/// Parsed POP3 client command (`VERB [arg1 [arg2]]`).
pub struct Pop3Command {
    verb: []const u8,
    arg1: []const u8,
    arg2: []const u8,

    fn verbEq(target: []const u8): bool {
        return asciiEqIgnoreCase(_.verb, target);
    }
}

/// Parses a POP3 command line (RFC 1939 / RFC 2449).
pub fn parsePop3Command(line: []const u8): Pop3Command? {
    if (line.len > POP3_MAX_CMD_LEN) {
        return null;
    }
    const clean = stripLineEnding(line);
    if (clean.len == 0 || hasCrLfOrNul(clean)) {
        return null;
    }

    // Parse command verb
    let pos: usize = 0;
    while (pos < clean.len && clean[pos] != ' ') {
        if (!isValidVerbChar(clean[pos])) {
            return null;
        }
        pos += 1;
    }
    if (pos == 0 || pos > 16) {
        return null;
    }
    const verb = clean[0..pos];

    if (pos == clean.len) {
        return Pop3Command{
            .verb = verb,
            .arg1 = clean[pos..pos],
            .arg2 = clean[pos..pos],
        };
    }

    // Skip single space separator
    pos += 1;
    if (pos == clean.len) {
        return null;
    }

    // PASS password may contain spaces; keep entire remainder as arg1
    if (asciiEqIgnoreCase(verb, "PASS")) {
        return Pop3Command{
            .verb = verb,
            .arg1 = clean[pos..],
            .arg2 = clean[clean.len..clean.len],
        };
    }

    // Find optional space separating arg1 and arg2
    let sp: usize = pos;
    while (sp < clean.len && clean[sp] != ' ') {
        sp += 1;
    }
    if (sp == pos) {
        return null;
    }
    const arg1 = clean[pos..sp];
    if (sp == clean.len) {
        return Pop3Command{
            .verb = verb,
            .arg1 = arg1,
            .arg2 = clean[sp..sp],
        };
    }

    const arg2_start = sp + 1;
    if (arg2_start == clean.len) {
        return null;
    }
    return Pop3Command{
        .verb = verb,
        .arg1 = arg1,
        .arg2 = clean[arg2_start..],
    };
}

/// Zero-copy iterator over RFC 1939 multi-line POP3 responses.
/// Splits on `\r\n`, stops on the standalone termination line `.\r\n`,
/// and performs byte-unstuffing (`..` -> `.`) without heap allocation.
pub struct Pop3MultiLineIter {
    data: []const u8,
    offset: usize,
    done: bool,
    failed: bool,

    fn init(body: []const u8): Pop3MultiLineIter {
        return Pop3MultiLineIter{
            .data = body,
            .offset = 0,
            .done = false,
            .failed = false,
        };
    }

    fn isComplete(): bool {
        return _.done && !_.failed;
    }

    fn hasError(): bool {
        return _.failed;
    }

    fn remaining(): []const u8 {
        return _.data[_.offset..];
    }

    fn next(*_): ([]const u8)? {
        if (_.done || _.failed) {
            return null;
        }
        if (_.offset >= _.data.len) {
            _.failed = true;
            return null;
        }

        // Find next CRLF
        let i = _.offset;
        let crlf_pos: usize = 0;
        let found = false;
        while (i + 1 < _.data.len) {
            if (_.data[i] == '\r' && _.data[i + 1] == '\n') {
                crlf_pos = i;
                found = true;
                break;
            }
            i += 1;
        }
        if (!found) {
            _.failed = true;
            return null;
        }

        const raw_line = _.data[_.offset..crlf_pos];
        _.offset = crlf_pos + 2;

        // Standalone termination line: "."
        if (raw_line.len == 1 && raw_line[0] == '.') {
            _.done = true;
            return null;
        }

        // RFC 1939 byte-unstuffing: leading ".." -> "."
        if (raw_line.len >= 2 && raw_line[0] == '.' && raw_line[1] == '.') {
            return raw_line[1..];
        }

        return raw_line;
    }
}

/// Zero-alloc POP3 command encoder (`VERB [arg1 [arg2]]\r\n`).
/// Rejects CRLF/NUL injection, invalid verbs, missing arg1 with arg2, and commands > 255 bytes.
pub fn encodePop3Command(verb: []const u8, arg1: []const u8, arg2: []const u8, out: []u8): usize? {
    if (verb.len == 0 || verb.len > 16) {
        return null;
    }
    let v_i: usize = 0;
    while (v_i < verb.len) {
        if (!isValidVerbChar(verb[v_i])) {
            return null;
        }
        v_i += 1;
    }
    if (hasCrLfOrNul(arg1) || hasCrLfOrNul(arg2)) {
        return null;
    }
    if (arg1.len == 0 && arg2.len > 0) {
        return null;
    }

    let total: usize = verb.len + 2;
    if (arg1.len > 0) {
        total += 1 + arg1.len;
    }
    if (arg2.len > 0) {
        total += 1 + arg2.len;
    }
    if (total > POP3_MAX_CMD_LEN || total > out.len) {
        return null;
    }

    let pos: usize = 0;
    let i: usize = 0;
    while (i < verb.len) {
        out[pos] = verb[i];
        pos += 1;
        i += 1;
    }
    if (arg1.len > 0) {
        out[pos] = ' ';
        pos += 1;
        let j: usize = 0;
        while (j < arg1.len) {
            out[pos] = arg1[j];
            pos += 1;
            j += 1;
        }
    }
    if (arg2.len > 0) {
        out[pos] = ' ';
        pos += 1;
        let k: usize = 0;
        while (k < arg2.len) {
            out[pos] = arg2[k];
            pos += 1;
            k += 1;
        }
    }
    out[pos] = '\r';
    out[pos + 1] = '\n';
    return pos + 2;
}

/// Zero-alloc RFC 1939 dot-stuffing encoder for POP3 multi-line bodies.
/// Prepends an extra `.` to any line starting with `.`, normalizes line endings to `\r\n`,
/// and appends the terminating `.\r\n` sequence.
pub fn pop3DotStuff(src: []const u8, out: []u8): usize? {
    let pos: usize = 0;

    if (src.len == 0) {
        if (out.len < 3) {
            return null;
        }
        out[0] = '.';
        out[1] = '\r';
        out[2] = '\n';
        return 3;
    }

    let i: usize = 0;
    let at_line_start = true;
    while (i < src.len) {
        const c = src[i];
        if (at_line_start && c == '.') {
            if (pos >= out.len) {
                return null;
            }
            out[pos] = '.';
            pos += 1;
        }

        if (c == '\r' && i + 1 < src.len && src[i + 1] == '\n') {
            if (pos + 2 > out.len) {
                return null;
            }
            out[pos] = '\r';
            out[pos + 1] = '\n';
            pos += 2;
            i += 2;
            at_line_start = true;
        } else if (c == '\n') {
            if (pos + 2 > out.len) {
                return null;
            }
            out[pos] = '\r';
            out[pos + 1] = '\n';
            pos += 2;
            i += 1;
            at_line_start = true;
        } else {
            if (pos >= out.len) {
                return null;
            }
            out[pos] = c;
            pos += 1;
            i += 1;
            at_line_start = false;
        }
    }

    // Ensure final line ends with CRLF before the standalone termination line
    if (!at_line_start) {
        if (pos + 2 > out.len) {
            return null;
        }
        out[pos] = '\r';
        out[pos + 1] = '\n';
        pos += 2;
    }

    // Append termination line ".\r\n"
    if (pos + 3 > out.len) {
        return null;
    }
    out[pos] = '.';
    out[pos + 1] = '\r';
    out[pos + 2] = '\n';
    return pos + 3;
}
