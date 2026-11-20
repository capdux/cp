// Source: cp/lib/imap/imap.cp (RFC 3501 / RFC 9051 IMAP4rev1/2 Wire Codec, Literal Framing & Response Parser)

const std = @import("std");

/// Errors returned by `cap:imap` stream parser, semantic helpers, and command codec.
pub err ImapError {
    InvalidTag,
    InvalidResponse,
    UnexpectedEof,
    TruncatedLiteral,
    CommandInjection,
    InvalidNumber,
    OutOfMemory,
}

/// IMAP response status codes (RFC 3501 §7.1).
pub enum ImapStatus {
    Ok,
    No,
    Bad,
    Preauth,
    Bye,

    /// Parses a case-insensitive IMAP status keyword.
    pub static fn fromBytes(s: []const u8): ImapStatus? {
        if (std.ascii.eqlIgnoreCase(s, "OK")) return ImapStatus.Ok;
        if (std.ascii.eqlIgnoreCase(s, "NO")) return ImapStatus.No;
        if (std.ascii.eqlIgnoreCase(s, "BAD")) return ImapStatus.Bad;
        if (std.ascii.eqlIgnoreCase(s, "PREAUTH")) return ImapStatus.Preauth;
        if (std.ascii.eqlIgnoreCase(s, "BYE")) return ImapStatus.Bye;
        return null;
    }

    /// Returns the canonical uppercase IMAP status keyword.
    pub fn toString(): []const u8 {
        switch (_) {
            .Ok => return "OK",
            .No => return "NO",
            .Bad => return "BAD",
            .Preauth => return "PREAUTH",
            .Bye => return "BYE",
        }
    }
}

/// Parsed IMAP server response frame with owned string/literal slices.
pub enum ImapResponseKind {
    tagged: {
        tag: []const u8,
        status: ImapStatus,
        code: ([]const u8)?,
        text: []const u8,
    },
    untagged_status: {
        status: ImapStatus,
        code: ([]const u8)?,
        text: []const u8,
    },
    untagged_data: {
        keyword: []const u8,
        rest: []const u8,
        literals: [][]const u8,
    },
    continuation: []const u8,

    /// Releases all heap-allocated slices owned by this response.
    pub fn deinit(*_, allocator: std.mem.Allocator) {
        switch (_.*) {
            .tagged t => {
                allocator.free(t.tag);
                if (c := t.code) {
                    allocator.free(c);
                }
                allocator.free(t.text);
            },
            .untagged_status u => {
                if (c := u.code) {
                    allocator.free(c);
                }
                allocator.free(u.text);
            },
            .untagged_data d => {
                allocator.free(d.keyword);
                allocator.free(d.rest);
                let i: usize = 0;
                while (i < d.literals.len) : (i += 1) {
                    allocator.free(d.literals[i]);
                }
                allocator.free(d.literals);
            },
            .continuation c => {
                allocator.free(c);
            },
        }
    }
}

struct ParsedStatusCode {
    code: ([]const u8)?,
    text: []const u8,
}

struct TokenSplit {
    tok: []const u8,
    rem: []const u8,
}

fn hasCrLfOrNul(s: []const u8): bool {
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        const ch = s[i];
        if (ch == '\r' || ch == '\n' || ch == 0) {
            return true;
        }
    }
    return false;
}

fn isValidTag(tag: []const u8): bool {
    if (tag.len == 0) return false;
    let i: usize = 0;
    while (i < tag.len) : (i += 1) {
        const ch = tag[i];
        if (ch <= 0x20 || ch >= 0x7F) return false;
        if (ch == '(' || ch == ')' || ch == '{' || ch == '*' || ch == '%' || ch == '"' || ch == '\\' || ch == '+' || ch == ']') {
            return false;
        }
    }
    return true;
}

fn parseUint(s: []const u8): usize? {
    if (s.len == 0) return null;
    let acc: usize = 0;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        const ch = s[i];
        if (ch < '0' || ch > '9') return null;
        const d = (ch - '0') as usize;
        if (acc > (std.math.maxInt(usize) - d) / 10) {
            return null;
        }
        acc = acc * 10 + d;
    }
    return acc;
}

fn parseUint32(s: []const u8): u32? {
    if (s.len == 0) return null;
    let acc: u32 = 0;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        const ch = s[i];
        if (ch < '0' || ch > '9') return null;
        const d = (ch - '0') as u32;
        if (acc > (std.math.maxInt(u32) - d) / 10) {
            return null;
        }
        acc = acc * 10 + d;
    }
    return acc;
}

fn isAllDigits(s: []const u8): bool {
    if (s.len == 0) return false;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        if (s[i] < '0' || s[i] > '9') return false;
    }
    return true;
}

/// Checks whether `line` (without trailing `\r\n`) ends with `{N}` or `{N+}`.
fn detectLiteralMarker(line: []const u8): (usize?) ! ImapError {
    if (line.len < 3 || line[line.len - 1] != '}') {
        return null;
    }
    let open_idx: usize? = null;
    let i = line.len - 1;
    while (i > 0) {
        i -= 1;
        if (line[i] == '{') {
            open_idx = i;
            break;
        }
        if (line[i] == '}' || line[i] == ' ' || line[i] == '\t') {
            break;
        }
    }
    const start = open_idx ?? return null;
    let num_end = line.len - 1;
    if (num_end > start + 1 && line[num_end - 1] == '+') {
        num_end -= 1;
    }
    const digits = line[start + 1 .. num_end];
    const n = parseUint(digits) ?? return ImapError.InvalidNumber;
    return n;
}

fn freeStringList(allocator: std.mem.Allocator, list: *std.ArrayListUnmanaged(string)) {
    let i: usize = 0;
    while (i < list.items.len) : (i += 1) {
        allocator.free(list.items[i]);
    }
    list.deinit(allocator);
}

fn parseStatusAndCode(allocator: std.mem.Allocator, after_status: []const u8): ParsedStatusCode ! ImapError {
    let rest = after_status;
    if (rest.len > 0 && rest[0] == ' ') {
        rest = rest[1..];
    }

    let code_slice: ([]const u8)? = null;
    if (rest.len > 0 && rest[0] == '[') {
        const close_idx = std.mem.indexOfScalar(u8, rest, ']') ?? return ImapError.InvalidResponse;
        code_slice = allocator.dupe(u8, rest[1..close_idx]) catch return ImapError.OutOfMemory;
        rest = rest[close_idx + 1 ..];
        if (rest.len > 0 && rest[0] == ' ') {
            rest = rest[1..];
        }
    }

    const text_slice = allocator.dupe(u8, rest) catch {
        if (c := code_slice) {
            allocator.free(c);
        }
        return ImapError.OutOfMemory;
    };

    return ParsedStatusCode{ code: code_slice, text: text_slice };
}

fn splitFirstToken(s: []const u8): TokenSplit {
    let i: usize = 0;
    while (i < s.len && s[i] != ' ') : (i += 1) {}
    const tok = s[0..i];
    let rem = s[i..];
    if (rem.len > 0 && rem[0] == ' ') {
        rem = rem[1..];
    }
    return TokenSplit{ tok: tok, rem: rem };
}

/// Incremental chunked IMAP4rev1/2 response parser supporting `\r\n` framing and `{N}\r\n` / `{N+}\r\n` literals.
pub struct ImapStreamParser {
    allocator: std.mem.Allocator,
    buf: std.ArrayListUnmanaged(u8) = .empty,
    cursor: usize = 0,
    text_acc: std.ArrayListUnmanaged(u8) = .empty,
    literals: std.ArrayListUnmanaged(string) = .empty,
    pending_literal: usize? = null,
    eof_reached: bool = false,

    pub static fn init(allocator: std.mem.Allocator): ImapStreamParser {
        return ImapStreamParser{
            allocator: allocator,
        };
    }

    pub fn deinit(*_) {
        _.buf.deinit(_.allocator);
        _.text_acc.deinit(_.allocator);
        freeStringList(_.allocator, &_.literals);
    }

    /// Appends a raw wire chunk into the parser's internal buffer.
    pub fn feed(*_, chunk: []const u8): void ! ImapError {
        if (chunk.len == 0) return;
        if (_.cursor > 0) {
            const remaining = _.buf.items.len - _.cursor;
            if (remaining > 0) {
                std.mem.copyForwards(u8, _.buf.items[0..remaining], _.buf.items[_.cursor..]);
            }
            _.buf.items.len = remaining;
            _.cursor = 0;
        }
        _.buf.appendSlice(_.allocator, chunk) catch return ImapError.OutOfMemory;
    }

    /// Marks end-of-stream so subsequent `nextResponse()` calls report `TruncatedLiteral` or `UnexpectedEof`.
    pub fn markEof(*_) {
        _.eof_reached = true;
    }

    /// Verifies that no incomplete frame or literal remains at EOF.
    pub fn finish(*_): void ! ImapError {
        _.eof_reached = true;
        if (_.pending_literal != null) {
            return ImapError.TruncatedLiteral;
        }
        if (_.cursor < _.buf.items.len || _.text_acc.items.len > 0) {
            return ImapError.UnexpectedEof;
        }
    }

    fn resetCurrentFrame(*_) {
        _.text_acc.items.len = 0;
        freeStringList(_.allocator, &_.literals);
        _.literals = .empty;
        _.pending_literal = null;
    }

    /// Parses and returns the next complete `ImapResponseKind`, or `null` if more bytes are needed.
    pub fn nextResponse(*_): (ImapResponseKind?) ! ImapError {
        while (true) {
            if (lit_len := _.pending_literal) {
                const avail = _.buf.items.len - _.cursor;
                if (avail < lit_len) {
                    if (_.eof_reached) {
                        return ImapError.TruncatedLiteral;
                    }
                    return null;
                }
                const lit_slice = _.buf.items[_.cursor .. _.cursor + lit_len];
                const duped = _.allocator.dupe(u8, lit_slice) catch return ImapError.OutOfMemory;
                _.literals.append(_.allocator, duped) catch {
                    _.allocator.free(duped);
                    return ImapError.OutOfMemory;
                };
                _.cursor += lit_len;
                _.pending_literal = null;
                continue;
            }

            const unread = _.buf.items[_.cursor..];
            const crlf_pos = std.mem.indexOf(u8, unread, "\r\n");
            if (crlf_pos == null) {
                if (_.eof_reached && (unread.len > 0 || _.text_acc.items.len > 0)) {
                    return ImapError.UnexpectedEof;
                }
                return null;
            }

            const line_end = crlf_pos.?;
            const line = unread[0..line_end];
            _.cursor += line_end + 2;

            const lit_marker = detectLiteralMarker(line) catch |e| {
                _.resetCurrentFrame();
                return e;
            };
            if (lit_n := lit_marker) {
                _.text_acc.appendSlice(_.allocator, line) catch return ImapError.OutOfMemory;
                _.pending_literal = lit_n;
                continue;
            }

            _.text_acc.appendSlice(_.allocator, line) catch return ImapError.OutOfMemory;
            const full_line = _.text_acc.items;
            const resp = _.parseCompleteResponse(full_line) catch |e| {
                _.resetCurrentFrame();
                return e;
            };
            _.text_acc.items.len = 0;
            _.literals = .empty;
            return resp;
        }
    }

    fn parseCompleteResponse(*_, full_line: []const u8): ImapResponseKind ! ImapError {
        if (full_line.len == 0) {
            return ImapError.InvalidResponse;
        }

        // 1. Continuation response: `+` or `+ <text>`
        if (full_line[0] == '+') {
            if (_.literals.items.len > 0) {
                return ImapError.InvalidResponse;
            }
            let text: []const u8 = "";
            if (full_line.len > 1) {
                if (full_line[1] != ' ') {
                    return ImapError.InvalidResponse;
                }
                text = full_line[2..];
            }
            const duped = _.allocator.dupe(u8, text) catch return ImapError.OutOfMemory;
            return ImapResponseKind{ continuation: duped };
        }

        // 2. Untagged response: `* <payload>`
        if (full_line[0] == '*') {
            if (full_line.len < 2 || full_line[1] != ' ') {
                return ImapError.InvalidResponse;
            }
            const payload = full_line[2..];
            if (payload.len == 0) {
                return ImapError.InvalidResponse;
            }

            const first = splitFirstToken(payload);
            if (status := ImapStatus.fromBytes(first.tok)) {
                if (_.literals.items.len > 0) {
                    return ImapError.InvalidResponse;
                }
                const parsed = parseStatusAndCode(_.allocator, first.rem)?;
                return ImapResponseKind{
                    untagged_status: .{
                        .status = status,
                        .code = parsed.code,
                        .text = parsed.text,
                    },
                };
            }

            let kw: []const u8 = first.tok;
            let rest_str: []const u8 = first.rem;
            let owned_rest: []const u8 = "";

            if (isAllDigits(first.tok) && first.rem.len > 0) {
                const second = splitFirstToken(first.rem);
                kw = second.tok;
                if (second.rem.len > 0) {
                    owned_rest = std.fmt.allocPrint(_.allocator, "{s} {s}", .{ first.tok, second.rem }) catch return ImapError.OutOfMemory;
                } else {
                    owned_rest = _.allocator.dupe(u8, first.tok) catch return ImapError.OutOfMemory;
                }
            } else {
                owned_rest = _.allocator.dupe(u8, rest_str) catch return ImapError.OutOfMemory;
            }

            const owned_kw = _.allocator.dupe(u8, kw) catch {
                _.allocator.free(owned_rest);
                return ImapError.OutOfMemory;
            };
            const owned_lits = _.literals.toOwnedSlice(_.allocator) catch {
                _.allocator.free(owned_kw);
                _.allocator.free(owned_rest);
                return ImapError.OutOfMemory;
            };

            return ImapResponseKind{
                untagged_data: .{
                    .keyword = owned_kw,
                    .rest = owned_rest,
                    .literals = owned_lits,
                },
            };
        }

        // 3. Tagged response: `<tag> <status> [<code...>] <text>`
        if (_.literals.items.len > 0) {
            return ImapError.InvalidResponse;
        }
        const tag_split = splitFirstToken(full_line);
        if (!isValidTag(tag_split.tok)) {
            return ImapError.InvalidTag;
        }
        if (tag_split.rem.len == 0) {
            return ImapError.InvalidResponse;
        }

        const status_split = splitFirstToken(tag_split.rem);
        const status_opt = ImapStatus.fromBytes(status_split.tok);
        if (status_opt == null) {
            return ImapError.InvalidResponse;
        }
        const status = status_opt.?;

        const parsed = parseStatusAndCode(_.allocator, status_split.rem)?;
        const owned_tag = _.allocator.dupe(u8, tag_split.tok) catch {
            if (c := parsed.code) {
                _.allocator.free(c);
            }
            _.allocator.free(parsed.text);
            return ImapError.OutOfMemory;
        };

        return ImapResponseKind{
            tagged: .{
                .tag = owned_tag,
                .status = status,
                .code = parsed.code,
                .text = parsed.text,
            },
        };
    }
}

/// Parses `* CAPABILITY ...` or `CAPABILITY ...` into an owned slice of capability tokens.
pub fn parseCapability(allocator: std.mem.Allocator, line: []const u8): ([][]const u8) ! ImapError {
    let caps: std.ArrayListUnmanaged(string) = .empty;
    errdefer freeStringList(allocator, &caps);

    let it = std.mem.tokenizeScalar(u8, line, ' ');
    let idx: usize = 0;
    while (tok := it.next()) {
        if (idx == 0 && std.mem.eql(u8, tok, "*")) {
            idx += 1;
            continue;
        }
        if (idx <= 1 && std.ascii.eqlIgnoreCase(tok, "CAPABILITY")) {
            idx += 1;
            continue;
        }
        idx += 1;
        const duped = allocator.dupe(u8, tok) catch return ImapError.OutOfMemory;
        caps.append(allocator, duped) catch {
            allocator.free(duped);
            return ImapError.OutOfMemory;
        };
    }

    return caps.toOwnedSlice(allocator) catch return ImapError.OutOfMemory;
}

/// Parses `* SEARCH 1 5 42 108` (or `SEARCH ...` / space-separated UIDs) into an owned `[]u32`.
pub fn parseSearchUids(allocator: std.mem.Allocator, line: []const u8): ([]u32) ! ImapError {
    let uids: std.ArrayListUnmanaged(u32) = .empty;
    errdefer uids.deinit(allocator);

    let it = std.mem.tokenizeScalar(u8, line, ' ');
    let idx: usize = 0;
    while (tok := it.next()) {
        if (idx == 0 && std.mem.eql(u8, tok, "*")) {
            idx += 1;
            continue;
        }
        if (idx <= 1 && std.ascii.eqlIgnoreCase(tok, "SEARCH")) {
            idx += 1;
            continue;
        }
        idx += 1;
        const uid = parseUint32(tok) ?? return ImapError.InvalidNumber;
        uids.append(allocator, uid) catch return ImapError.OutOfMemory;
    }

    return uids.toOwnedSlice(allocator) catch return ImapError.OutOfMemory;
}

/// Parses `* 23 EXISTS` or `* 2 RECENT` (with or without leading `* `) and returns the count.
pub fn parseExistsCount(line: []const u8): u32? {
    let it = std.mem.tokenizeScalar(u8, line, ' ');
    let first = it.next() ?? return null;
    if (std.mem.eql(u8, first, "*")) {
        first = it.next() ?? return null;
    }
    const count = parseUint32(first) ?? return null;
    const kw = it.next() ?? return null;
    if (!std.ascii.eqlIgnoreCase(kw, "EXISTS") && !std.ascii.eqlIgnoreCase(kw, "RECENT")) {
        return null;
    }
    if (it.next() != null) {
        return null;
    }
    return count;
}

/// Extracts flag tokens from `FLAGS (\Seen \Flagged ...)` inside a FETCH response line.
pub fn parseFetchFlags(allocator: std.mem.Allocator, line: []const u8): ([][]const u8) ! ImapError {
    let i: usize = 0;
    let flags_start: usize? = null;
    while (i + 5 <= line.len) : (i += 1) {
        const boundary_ok = (i == 0) || (line[i - 1] == ' ' || line[i - 1] == '(');
        if (boundary_ok && std.ascii.eqlIgnoreCase(line[i .. i + 5], "FLAGS")) {
            let j = i + 5;
            while (j < line.len && line[j] == ' ') : (j += 1) {}
            if (j < line.len && line[j] == '(') {
                flags_start = j + 1;
                break;
            }
        }
    }

    if (flags_start == null) {
        return allocator.alloc(string, 0) catch return ImapError.OutOfMemory;
    }
    const start = flags_start.?;
    const rel_end = std.mem.indexOfScalar(u8, line[start..], ')') ?? return ImapError.InvalidResponse;
    const inner = line[start .. start + rel_end];

    let flags: std.ArrayListUnmanaged(string) = .empty;
    errdefer freeStringList(allocator, &flags);

    let it = std.mem.tokenizeScalar(u8, inner, ' ');
    while (tok := it.next()) {
        const duped = allocator.dupe(u8, tok) catch return ImapError.OutOfMemory;
        flags.append(allocator, duped) catch {
            allocator.free(duped);
            return ImapError.OutOfMemory;
        };
    }

    return flags.toOwnedSlice(allocator) catch return ImapError.OutOfMemory;
}

fn appendQuotedString(buf: *std.ArrayListUnmanaged(u8), allocator: std.mem.Allocator, s: []const u8): void ! ImapError {
    if (hasCrLfOrNul(s)) {
        return ImapError.CommandInjection;
    }
    buf.append(allocator, '"') catch return ImapError.OutOfMemory;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        const ch = s[i];
        if (ch == '"' || ch == '\\') {
            buf.append(allocator, '\\') catch return ImapError.OutOfMemory;
        }
        buf.append(allocator, ch) catch return ImapError.OutOfMemory;
    }
    buf.append(allocator, '"') catch return ImapError.OutOfMemory;
}

/// RFC 3501 IMAP command encoder with monotonic `A0001`..`A9999` tags, strict string quoting, and CRLF/NUL injection guards.
pub struct ImapCommandCodec {
    next_id: u32 = 1,

    pub static fn init(): ImapCommandCodec {
        return ImapCommandCodec{};
    }

    fn allocTag(*_, allocator: std.mem.Allocator): ([]u8) ! ImapError {
        const id = _.next_id;
        if (_.next_id == std.math.maxInt(u32)) {
            _.next_id = 1;
        } else {
            _.next_id += 1;
        }
        return std.fmt.allocPrint(allocator, "A{d:0>4}", .{id}) catch return ImapError.OutOfMemory;
    }

    /// Encodes `Axxxx LOGIN "<user>" "<pass>"\r\n`.
    pub fn login(*_, allocator: std.mem.Allocator, user: []const u8, pass: []const u8): ([]u8) ! ImapError {
        if (hasCrLfOrNul(user) || hasCrLfOrNul(pass)) {
            return ImapError.CommandInjection;
        }
        const tag = _.allocTag(allocator)?;
        defer allocator.free(tag);

        let out: std.ArrayListUnmanaged(u8) = .empty;
        errdefer out.deinit(allocator);

        out.appendSlice(allocator, tag) catch return ImapError.OutOfMemory;
        out.appendSlice(allocator, " LOGIN ") catch return ImapError.OutOfMemory;
        appendQuotedString(&out, allocator, user)?;
        out.append(allocator, ' ') catch return ImapError.OutOfMemory;
        appendQuotedString(&out, allocator, pass)?;
        out.appendSlice(allocator, "\r\n") catch return ImapError.OutOfMemory;
        return out.toOwnedSlice(allocator) catch return ImapError.OutOfMemory;
    }

    fn encodeMailboxCmd(*_, allocator: std.mem.Allocator, cmd: []const u8, mailbox: []const u8): ([]u8) ! ImapError {
        if (hasCrLfOrNul(mailbox)) {
            return ImapError.CommandInjection;
        }
        const tag = _.allocTag(allocator)?;
        defer allocator.free(tag);

        let out: std.ArrayListUnmanaged(u8) = .empty;
        errdefer out.deinit(allocator);

        out.appendSlice(allocator, tag) catch return ImapError.OutOfMemory;
        out.append(allocator, ' ') catch return ImapError.OutOfMemory;
        out.appendSlice(allocator, cmd) catch return ImapError.OutOfMemory;
        out.append(allocator, ' ') catch return ImapError.OutOfMemory;
        appendQuotedString(&out, allocator, mailbox)?;
        out.appendSlice(allocator, "\r\n") catch return ImapError.OutOfMemory;
        return out.toOwnedSlice(allocator) catch return ImapError.OutOfMemory;
    }

    /// Encodes `Axxxx SELECT "<mailbox>"\r\n`.
    pub fn select(*_, allocator: std.mem.Allocator, mailbox: []const u8): ([]u8) ! ImapError {
        return _.encodeMailboxCmd(allocator, "SELECT", mailbox);
    }

    /// Encodes `Axxxx EXAMINE "<mailbox>"\r\n`.
    pub fn examine(*_, allocator: std.mem.Allocator, mailbox: []const u8): ([]u8) ! ImapError {
        return _.encodeMailboxCmd(allocator, "EXAMINE", mailbox);
    }

    /// Encodes `Axxxx SEARCH <criteria>\r\n`.
    pub fn search(*_, allocator: std.mem.Allocator, criteria: []const u8): ([]u8) ! ImapError {
        if (hasCrLfOrNul(criteria)) {
            return ImapError.CommandInjection;
        }
        if (criteria.len == 0) {
            return ImapError.InvalidResponse;
        }
        const tag = _.allocTag(allocator)?;
        defer allocator.free(tag);
        return std.fmt.allocPrint(allocator, "{s} SEARCH {s}\r\n", .{ tag, criteria }) catch return ImapError.OutOfMemory;
    }

    /// Encodes `Axxxx FETCH <sequence> <items>\r\n`.
    pub fn fetch(*_, allocator: std.mem.Allocator, sequence: []const u8, items: []const u8): ([]u8) ! ImapError {
        if (hasCrLfOrNul(sequence) || hasCrLfOrNul(items)) {
            return ImapError.CommandInjection;
        }
        if (sequence.len == 0 || items.len == 0) {
            return ImapError.InvalidResponse;
        }
        const tag = _.allocTag(allocator)?;
        defer allocator.free(tag);
        return std.fmt.allocPrint(allocator, "{s} FETCH {s} {s}\r\n", .{ tag, sequence, items }) catch return ImapError.OutOfMemory;
    }

    /// Encodes `Axxxx STORE <sequence> <action> (<flags>)\r\n`.
    pub fn store(*_, allocator: std.mem.Allocator, sequence: []const u8, action: []const u8, flags: []const u8): ([]u8) ! ImapError {
        if (hasCrLfOrNul(sequence) || hasCrLfOrNul(action) || hasCrLfOrNul(flags)) {
            return ImapError.CommandInjection;
        }
        if (sequence.len == 0 || action.len == 0) {
            return ImapError.InvalidResponse;
        }
        const tag = _.allocTag(allocator)?;
        defer allocator.free(tag);
        if (flags.len >= 2 && flags[0] == '(' && flags[flags.len - 1] == ')') {
            return std.fmt.allocPrint(allocator, "{s} STORE {s} {s} {s}\r\n", .{ tag, sequence, action, flags }) catch return ImapError.OutOfMemory;
        }
        return std.fmt.allocPrint(allocator, "{s} STORE {s} {s} ({s})\r\n", .{ tag, sequence, action, flags }) catch return ImapError.OutOfMemory;
    }

    /// Encodes `Axxxx EXPUNGE\r\n`.
    pub fn expunge(*_, allocator: std.mem.Allocator): ([]u8) ! ImapError {
        const tag = _.allocTag(allocator)?;
        defer allocator.free(tag);
        return std.fmt.allocPrint(allocator, "{s} EXPUNGE\r\n", .{tag}) catch return ImapError.OutOfMemory;
    }

    /// Encodes `Axxxx LOGOUT\r\n`.
    pub fn logout(*_, allocator: std.mem.Allocator): ([]u8) ! ImapError {
        const tag = _.allocTag(allocator)?;
        defer allocator.free(tag);
        return std.fmt.allocPrint(allocator, "{s} LOGOUT\r\n", .{tag}) catch return ImapError.OutOfMemory;
    }

    /// Encodes `Axxxx NOOP\r\n`.
    pub fn noop(*_, allocator: std.mem.Allocator): ([]u8) ! ImapError {
        const tag = _.allocTag(allocator)?;
        defer allocator.free(tag);
        return std.fmt.allocPrint(allocator, "{s} NOOP\r\n", .{tag}) catch return ImapError.OutOfMemory;
    }
}
