// Source: cp/lib/smtp/smtp.cp (RFC 5321/5322 SMTP Wire Protocol, Dot-Stuffing & MIME Mail Builder)

const std = @import("std");
import { Base64 } from "cap:encoding";

/// Errors returned by `cap:smtp` wire protocol codecs, reply parser, and MIME mail builder.
pub err SmtpError {
    InvalidAddress,
    HeaderInjection,
    InvalidReply,
    InvalidAttachment,
    TruncatedLine,
    OutOfMemory,
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

fn hasAngleBrackets(s: []const u8): bool {
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        const ch = s[i];
        if (ch == '<' || ch == '>') {
            return true;
        }
    }
    return false;
}

fn validateAddress(addr: []const u8, allow_empty: bool): void ! SmtpError {
    if (hasCrLfOrNul(addr) || hasAngleBrackets(addr)) {
        return SmtpError.HeaderInjection;
    }
    if (addr.len == 0) {
        if (allow_empty) return;
        return SmtpError.InvalidAddress;
    }

    let at_pos: usize? = null;
    let i: usize = 0;
    while (i < addr.len) : (i += 1) {
        const ch = addr[i];
        if (ch <= 0x20 || ch >= 0x7F) {
            return SmtpError.InvalidAddress;
        }
        if (ch == '@') {
            if (at_pos != null) {
                return SmtpError.InvalidAddress;
            }
            at_pos = i;
        }
    }

    const at_idx = at_pos ?? return SmtpError.InvalidAddress;
    if (at_idx == 0 || at_idx + 1 >= addr.len) {
        return SmtpError.InvalidAddress;
    }

    const domain = addr[at_idx + 1 ..];
    if (domain[0] == '.' || domain[domain.len - 1] == '.') {
        return SmtpError.InvalidAddress;
    }
    let has_dot = false;
    let prev_dot = false;
    let j: usize = 0;
    while (j < domain.len) : (j += 1) {
        const dch = domain[j];
        if (dch == '.') {
            if (prev_dot) return SmtpError.InvalidAddress;
            has_dot = true;
            prev_dot = true;
        } else {
            prev_dot = false;
        }
    }
    if (!has_dot) {
        return SmtpError.InvalidAddress;
    }
}

fn isValidEhloDomain(domain: []const u8): bool {
    if (domain.len == 0 || domain.len > 255) return false;
    let i: usize = 0;
    while (i < domain.len) : (i += 1) {
        const ch = domain[i];
        if (ch <= 0x20 || ch >= 0x7F || ch == '<' || ch == '>') {
            return false;
        }
    }
    return true;
}

fn trimTrailingCrLf(s: []const u8): []const u8 {
    let end = s.len;
    while (end > 0 && (s[end - 1] == '\r' || s[end - 1] == '\n')) : (end -= 1) {}
    return s[0..end];
}

fn parseThreeDigitCode(s: []const u8): u16? {
    if (s.len < 3) return null;
    if (s[0] < '0' || s[0] > '9' || s[1] < '0' || s[1] > '9' || s[2] < '0' || s[2] > '9') {
        return null;
    }
    const code = ((s[0] - '0') as u16) * 100 + ((s[1] - '0') as u16) * 10 + ((s[2] - '0') as u16);
    if (code < 100 || code > 599) return null;
    return code;
}

fn isDigit(ch: u8): bool {
    return ch >= '0' && ch <= '9';
}

fn extractEnhancedCode(text: []const u8): ([]const u8)? {
    if (text.len < 5) return null;
    if (text[0] != '2' && text[0] != '4' && text[0] != '5') return null;
    if (text[1] != '.') return null;

    let pos: usize = 2;
    let subj_digits: usize = 0;
    while (pos < text.len && isDigit(text[pos])) : (pos += 1) {
        subj_digits += 1;
    }
    if (subj_digits == 0 || subj_digits > 3 || pos >= text.len || text[pos] != '.') {
        return null;
    }
    pos += 1;

    let detail_digits: usize = 0;
    while (pos < text.len && isDigit(text[pos])) : (pos += 1) {
        detail_digits += 1;
    }
    if (detail_digits == 0 || detail_digits > 3) {
        return null;
    }
    if (pos < text.len && text[pos] != ' ' && text[pos] != '\t') {
        return null;
    }
    return text[0..pos];
}

/// Parsed RFC 5321 SMTP server reply (supports single-line, multi-line `XYZ-`, and RFC 2034 enhanced status codes).
pub struct SmtpReply {
    code: u16 = 0;
    enhanced: ([]const u8)? = null;
    lines: [][]const u8 = [];

    /// Frees all heap-allocated line slices and enhanced status code owned by this reply.
    pub fn deinit(*_, allocator: std.mem.Allocator) {
        if (enh := _.enhanced) {
            allocator.free(enh);
            _.enhanced = null;
        }
        for (line of _.lines) {
            allocator.free(line);
        }
        if (_.lines.len > 0) {
            allocator.free(_.lines);
            _.lines = [];
        }
    }

    /// Returns `true` for 2xx positive completion replies.
    pub fn isSuccess(): bool {
        return _.code >= 200 && _.code < 300;
    }

    /// Returns `true` for 3xx positive intermediate replies (e.g. `334`, `354`).
    pub fn isIntermediate(): bool {
        return _.code >= 300 && _.code < 400;
    }

    /// Returns `true` for 4xx transient negative completion replies.
    pub fn isTransientError(): bool {
        return _.code >= 400 && _.code < 500;
    }

    /// Returns `true` for 5xx permanent negative completion replies.
    pub fn isPermanentError(): bool {
        return _.code >= 500 && _.code < 600;
    }
}

/// Parses a single-line (`250 OK`) or multi-line (`250-...\r\n250 ...`) RFC 5321 SMTP reply.
pub fn parseReply(allocator: std.mem.Allocator, raw: []const u8): SmtpReply ! SmtpError {
    const trimmed = trimTrailingCrLf(raw);
    if (trimmed.len == 0) {
        return SmtpError.TruncatedLine;
    }
    if (trimmed.len < 3) {
        if (trimmed[0] >= '0' && trimmed[0] <= '9') {
            return SmtpError.TruncatedLine;
        }
        return SmtpError.InvalidReply;
    }

    let expected_code: u16? = null;
    let saw_final_line = false;
    let enhanced_slice: ([]const u8)? = null;

    let collected: std.ArrayListUnmanaged(string) = .empty;
    errdefer {
        for (item of collected.items) {
            allocator.free(item);
        }
        collected.deinit(allocator);
        if (enh := enhanced_slice) {
            allocator.free(enh);
        }
    }

    let line_iter = std.mem.splitScalar(u8, trimmed, '\n');
    while (raw_line := line_iter.next()) {
        const line = trimTrailingCrLf(raw_line);
        if (line.len == 0) {
            return SmtpError.InvalidReply;
        }
        if (line.len < 3) {
            return SmtpError.TruncatedLine;
        }

        const code = parseThreeDigitCode(line) ?? return SmtpError.InvalidReply;
        if (exp := expected_code) {
            if (code != exp) {
                return SmtpError.InvalidReply;
            }
        } else {
            expected_code = code;
        }

        if (saw_final_line) {
            return SmtpError.InvalidReply;
        }

        let body: []const u8 = "";
        if (line.len == 3) {
            saw_final_line = true;
        } else {
            const sep = line[3];
            if (sep == '-') {
                body = line[4..];
            } else if (sep == ' ') {
                saw_final_line = true;
                body = line[4..];
            } else {
                return SmtpError.InvalidReply;
            }
        }

        if (enhanced_slice == null) {
            if (enh := extractEnhancedCode(body)) {
                enhanced_slice = allocator.dupe(u8, enh) catch return SmtpError.OutOfMemory;
            }
        }

        const duped_line = allocator.dupe(u8, body) catch return SmtpError.OutOfMemory;
        collected.append(allocator, duped_line) catch {
            allocator.free(duped_line);
            return SmtpError.OutOfMemory;
        };
    }

    if (expected_code == null || collected.items.len == 0) {
        return SmtpError.TruncatedLine;
    }
    if (!saw_final_line) {
        return SmtpError.TruncatedLine;
    }

    const owned_lines = collected.toOwnedSlice(allocator) catch return SmtpError.OutOfMemory;
    return SmtpReply{
        code: expected_code.?,
        enhanced: enhanced_slice,
        lines: owned_lines,
    };
}

/// RFC 5321 & RFC 4616 SMTP wire command encoder with strict CRLF/angle-bracket injection guards.
pub static struct SmtpCommandCodec {
    /// Encodes `EHLO <domain>\r\n`.
    pub static fn ehlo(allocator: std.mem.Allocator, domain: []const u8): ([]u8) ! SmtpError {
        if (hasCrLfOrNul(domain) || hasAngleBrackets(domain)) {
            return SmtpError.HeaderInjection;
        }
        if (!isValidEhloDomain(domain)) {
            return SmtpError.InvalidAddress;
        }
        return std.fmt.allocPrint(allocator, "EHLO {s}\r\n", .{ domain }) catch return SmtpError.OutOfMemory;
    }

    /// Encodes `AUTH PLAIN <base64(\0user\0pass)>\r\n` per RFC 4616.
    pub static fn authPlain(allocator: std.mem.Allocator, user: []const u8, pass: []const u8): ([]u8) ! SmtpError {
        if (hasCrLfOrNul(user) || hasCrLfOrNul(pass)) {
            return SmtpError.HeaderInjection;
        }
        const raw_len = 1 + user.len + 1 + pass.len;
        let raw = allocator.alloc(u8, raw_len) catch return SmtpError.OutOfMemory;
        defer allocator.free(raw);

        raw[0] = 0;
        if (user.len > 0) {
            @memcpy(raw[1 .. 1 + user.len], user);
        }
        raw[1 + user.len] = 0;
        if (pass.len > 0) {
            @memcpy(raw[2 + user.len .. raw_len], pass);
        }

        const b64 = Base64.encodeAlloc(allocator, raw) catch return SmtpError.OutOfMemory;
        defer allocator.free(b64);

        return std.fmt.allocPrint(allocator, "AUTH PLAIN {s}\r\n", .{ b64 }) catch return SmtpError.OutOfMemory;
    }

    /// Encodes `MAIL FROM:<addr>\r\n` (empty `addr` encodes null reverse-path `MAIL FROM:<>\r\n`).
    pub static fn mailFrom(allocator: std.mem.Allocator, addr: []const u8): ([]u8) ! SmtpError {
        validateAddress(addr, true)?;
        return std.fmt.allocPrint(allocator, "MAIL FROM:<{s}>\r\n", .{ addr }) catch return SmtpError.OutOfMemory;
    }

    /// Encodes `RCPT TO:<addr>\r\n`.
    pub static fn rcptTo(allocator: std.mem.Allocator, addr: []const u8): ([]u8) ! SmtpError {
        validateAddress(addr, false)?;
        return std.fmt.allocPrint(allocator, "RCPT TO:<{s}>\r\n", .{ addr }) catch return SmtpError.OutOfMemory;
    }

    /// Returns static `DATA\r\n` command.
    pub static fn dataStart(): []const u8 {
        return "DATA\r\n";
    }

    /// Returns static `QUIT\r\n` command.
    pub static fn quit(): []const u8 {
        return "QUIT\r\n";
    }
}

/// Applies RFC 5321 §4.5.2 dot-stuffing (doubling leading `.` on any line) and appends `\r\n.\r\n`.
pub fn dotStuff(allocator: std.mem.Allocator, raw_mail: []const u8): ([]u8) ! SmtpError {
    let out: std.ArrayListUnmanaged(u8) = .empty;
    errdefer out.deinit(allocator);

    let at_line_start = true;
    let i: usize = 0;
    while (i < raw_mail.len) : (i += 1) {
        const ch = raw_mail[i];
        if (at_line_start && ch == '.') {
            out.append(allocator, '.') catch return SmtpError.OutOfMemory;
        }
        out.append(allocator, ch) catch return SmtpError.OutOfMemory;
        at_line_start = (ch == '\n');
    }

    out.appendSlice(allocator, "\r\n.\r\n") catch return SmtpError.OutOfMemory;
    return out.toOwnedSlice(allocator) catch return SmtpError.OutOfMemory;
}

/// Reverses RFC 5321 §4.5.2 dot-stuffing and strips the trailing `\r\n.\r\n` DATA terminator.
pub fn dotUnstuff(allocator: std.mem.Allocator, stuffed_wire: []const u8): ([]u8) ! SmtpError {
    let content: []const u8 = "";
    if (std.mem.endsWith(u8, stuffed_wire, "\r\n.\r\n")) {
        content = stuffed_wire[0 .. stuffed_wire.len - 5];
    } else if (std.mem.eql(u8, stuffed_wire, ".\r\n")) {
        content = "";
    } else {
        return SmtpError.TruncatedLine;
    }

    let out: std.ArrayListUnmanaged(u8) = .empty;
    errdefer out.deinit(allocator);

    let at_line_start = true;
    let i: usize = 0;
    while (i < content.len) : (i += 1) {
        const ch = content[i];
        if (at_line_start && ch == '.' && i + 1 < content.len && content[i + 1] == '.') {
            at_line_start = false;
            continue;
        }
        out.append(allocator, ch) catch return SmtpError.OutOfMemory;
        at_line_start = (ch == '\n');
    }

    return out.toOwnedSlice(allocator) catch return SmtpError.OutOfMemory;
}

/// MIME attachment descriptor for `MailBuilder`.
pub struct SmtpAttachment {
    filename: []const u8;
    content_type: []const u8;
    data: []const u8;
}

fn containsNonAscii(s: []const u8): bool {
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        if (s[i] >= 0x80) return true;
    }
    return false;
}

fn hasQuoteOrControl(s: []const u8): bool {
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        const ch = s[i];
        if (ch == '\r' || ch == '\n' || ch == 0 || ch == '"') {
            return true;
        }
    }
    return false;
}

fn appendBase64Wrapped76(allocator: std.mem.Allocator, out: *std.ArrayListUnmanaged(u8), data: []const u8): void ! SmtpError {
    if (data.len == 0) return;
    const b64 = Base64.encodeAlloc(allocator, data) catch return SmtpError.OutOfMemory;
    defer allocator.free(b64);

    let offset: usize = 0;
    while (offset < b64.len) {
        const end = if (offset + 76 < b64.len) offset + 76 else b64.len;
        out.appendSlice(allocator, b64[offset..end]) catch return SmtpError.OutOfMemory;
        out.appendSlice(allocator, "\r\n") catch return SmtpError.OutOfMemory;
        offset = end;
    }
}

fn appendAddressListHeader(
    allocator: std.mem.Allocator,
    out: *std.ArrayListUnmanaged(u8),
    header_name: []const u8,
    addrs: [][]const u8,
): void ! SmtpError {
    if (addrs.len == 0) return;
    out.appendSlice(allocator, header_name) catch return SmtpError.OutOfMemory;
    out.appendSlice(allocator, ": ") catch return SmtpError.OutOfMemory;
    let i: usize = 0;
    while (i < addrs.len) : (i += 1) {
        if (i > 0) {
            out.appendSlice(allocator, ", ") catch return SmtpError.OutOfMemory;
        }
        out.appendSlice(allocator, addrs[i]) catch return SmtpError.OutOfMemory;
    }
    out.appendSlice(allocator, "\r\n") catch return SmtpError.OutOfMemory;
}

/// RFC 5322 & MIME (RFC 2045/2046/2047) email message builder with strict header injection protection.
pub struct MailBuilder {
    allocator: std.mem.Allocator;
    from_addr: []const u8;
    subject: []const u8;
    to_list: std.ArrayListUnmanaged(string) = .empty;
    cc_list: std.ArrayListUnmanaged(string) = .empty;
    text_body: ([]const u8)? = null;
    html_body: ([]const u8)? = null;
    attachments: std.ArrayListUnmanaged(SmtpAttachment) = .empty;

    /// Initializes a new `MailBuilder` with sender address and subject.
    pub static fn init(allocator: std.mem.Allocator, from_addr: []const u8, subject: []const u8): MailBuilder {
        return MailBuilder{
            allocator: allocator,
            from_addr: from_addr,
            subject: subject,
        };
    }

    /// Releases internal recipient and attachment lists.
    pub fn deinit(*_) {
        _.to_list.deinit(_.allocator);
        _.cc_list.deinit(_.allocator);
        _.attachments.deinit(_.allocator);
    }

    /// Adds a `To` recipient after validating against header injection and invalid email syntax.
    pub fn addTo(*_, addr: []const u8): void ! SmtpError {
        validateAddress(addr, false)?;
        _.to_list.append(_.allocator, addr) catch return SmtpError.OutOfMemory;
    }

    /// Adds a `Cc` recipient after validating against header injection and invalid email syntax.
    pub fn addCc(*_, addr: []const u8): void ! SmtpError {
        validateAddress(addr, false)?;
        _.cc_list.append(_.allocator, addr) catch return SmtpError.OutOfMemory;
    }

    /// Sets the `text/plain; charset="UTF-8"` body.
    pub fn setTextBody(*_, text: []const u8) {
        _.text_body = text;
    }

    /// Sets the `text/html; charset="UTF-8"` body.
    pub fn setHtmlBody(*_, html: []const u8) {
        _.html_body = html;
    }

    /// Attaches a binary or text payload after validating filename and content-type against injection.
    pub fn addAttachment(*_, att: SmtpAttachment): void ! SmtpError {
        if (hasCrLfOrNul(att.filename) || hasCrLfOrNul(att.content_type)) {
            return SmtpError.HeaderInjection;
        }
        if (att.filename.len == 0 || att.content_type.len == 0 || hasQuoteOrControl(att.filename) || hasQuoteOrControl(att.content_type)) {
            return SmtpError.InvalidAttachment;
        }
        _.attachments.append(_.allocator, att) catch return SmtpError.OutOfMemory;
    }

    /// Builds the complete RFC 5322 / MIME wire message (caller owns returned slice).
    pub fn build(allocator: std.mem.Allocator, boundary: []const u8): ([]u8) ! SmtpError {
        validateAddress(_.from_addr, false)?;
        if (hasCrLfOrNul(_.subject) || hasCrLfOrNul(boundary)) {
            return SmtpError.HeaderInjection;
        }
        if (boundary.len == 0 || hasQuoteOrControl(boundary)) {
            return SmtpError.InvalidAttachment;
        }
        for (addr of _.to_list.items) {
            validateAddress(addr, false)?;
        }
        for (addr of _.cc_list.items) {
            validateAddress(addr, false)?;
        }
        for (att of _.attachments.items) {
            if (hasCrLfOrNul(att.filename) || hasCrLfOrNul(att.content_type)) {
                return SmtpError.HeaderInjection;
            }
            if (att.filename.len == 0 || att.content_type.len == 0 || hasQuoteOrControl(att.filename) || hasQuoteOrControl(att.content_type)) {
                return SmtpError.InvalidAttachment;
            }
        }

        let out: std.ArrayListUnmanaged(u8) = .empty;
        errdefer out.deinit(allocator);

        // From
        out.appendSlice(allocator, "From: ") catch return SmtpError.OutOfMemory;
        out.appendSlice(allocator, _.from_addr) catch return SmtpError.OutOfMemory;
        out.appendSlice(allocator, "\r\n") catch return SmtpError.OutOfMemory;

        // To & Cc
        appendAddressListHeader(allocator, &out, "To", _.to_list.items)?;
        appendAddressListHeader(allocator, &out, "Cc", _.cc_list.items)?;

        // Subject (RFC 2047 UTF-8 Base64 if non-ASCII)
        out.appendSlice(allocator, "Subject: ") catch return SmtpError.OutOfMemory;
        if (containsNonAscii(_.subject)) {
            const subj_b64 = Base64.encodeAlloc(allocator, _.subject) catch return SmtpError.OutOfMemory;
            defer allocator.free(subj_b64);
            out.appendSlice(allocator, "=?UTF-8?B?") catch return SmtpError.OutOfMemory;
            out.appendSlice(allocator, subj_b64) catch return SmtpError.OutOfMemory;
            out.appendSlice(allocator, "?=") catch return SmtpError.OutOfMemory;
        } else {
            out.appendSlice(allocator, _.subject) catch return SmtpError.OutOfMemory;
        }
        out.appendSlice(allocator, "\r\nMIME-Version: 1.0\r\n") catch return SmtpError.OutOfMemory;

        const has_atts = _.attachments.items.len > 0;
        const has_text = _.text_body != null;
        const has_html = _.html_body != null;

        if (has_atts) {
            out.appendSlice(allocator, "Content-Type: multipart/mixed; boundary=\"") catch return SmtpError.OutOfMemory;
            out.appendSlice(allocator, boundary) catch return SmtpError.OutOfMemory;
            out.appendSlice(allocator, "\"\r\n\r\n") catch return SmtpError.OutOfMemory;

            if (has_text && has_html) {
                const alt_boundary = std.fmt.allocPrint(allocator, "{s}_alt", .{ boundary }) catch return SmtpError.OutOfMemory;
                defer allocator.free(alt_boundary);

                out.appendSlice(allocator, "--") catch return SmtpError.OutOfMemory;
                out.appendSlice(allocator, boundary) catch return SmtpError.OutOfMemory;
                out.appendSlice(allocator, "\r\nContent-Type: multipart/alternative; boundary=\"") catch return SmtpError.OutOfMemory;
                out.appendSlice(allocator, alt_boundary) catch return SmtpError.OutOfMemory;
                out.appendSlice(allocator, "\"\r\n\r\n") catch return SmtpError.OutOfMemory;

                out.appendSlice(allocator, "--") catch return SmtpError.OutOfMemory;
                out.appendSlice(allocator, alt_boundary) catch return SmtpError.OutOfMemory;
                out.appendSlice(allocator, "\r\nContent-Type: text/plain; charset=\"UTF-8\"\r\nContent-Transfer-Encoding: 8bit\r\n\r\n") catch return SmtpError.OutOfMemory;
                out.appendSlice(allocator, _.text_body.?) catch return SmtpError.OutOfMemory;
                out.appendSlice(allocator, "\r\n") catch return SmtpError.OutOfMemory;

                out.appendSlice(allocator, "--") catch return SmtpError.OutOfMemory;
                out.appendSlice(allocator, alt_boundary) catch return SmtpError.OutOfMemory;
                out.appendSlice(allocator, "\r\nContent-Type: text/html; charset=\"UTF-8\"\r\nContent-Transfer-Encoding: 8bit\r\n\r\n") catch return SmtpError.OutOfMemory;
                out.appendSlice(allocator, _.html_body.?) catch return SmtpError.OutOfMemory;
                out.appendSlice(allocator, "\r\n--") catch return SmtpError.OutOfMemory;
                out.appendSlice(allocator, alt_boundary) catch return SmtpError.OutOfMemory;
                out.appendSlice(allocator, "--\r\n") catch return SmtpError.OutOfMemory;
            } else if (has_text) {
                out.appendSlice(allocator, "--") catch return SmtpError.OutOfMemory;
                out.appendSlice(allocator, boundary) catch return SmtpError.OutOfMemory;
                out.appendSlice(allocator, "\r\nContent-Type: text/plain; charset=\"UTF-8\"\r\nContent-Transfer-Encoding: 8bit\r\n\r\n") catch return SmtpError.OutOfMemory;
                out.appendSlice(allocator, _.text_body.?) catch return SmtpError.OutOfMemory;
                out.appendSlice(allocator, "\r\n") catch return SmtpError.OutOfMemory;
            } else if (has_html) {
                out.appendSlice(allocator, "--") catch return SmtpError.OutOfMemory;
                out.appendSlice(allocator, boundary) catch return SmtpError.OutOfMemory;
                out.appendSlice(allocator, "\r\nContent-Type: text/html; charset=\"UTF-8\"\r\nContent-Transfer-Encoding: 8bit\r\n\r\n") catch return SmtpError.OutOfMemory;
                out.appendSlice(allocator, _.html_body.?) catch return SmtpError.OutOfMemory;
                out.appendSlice(allocator, "\r\n") catch return SmtpError.OutOfMemory;
            }

            for (att of _.attachments.items) {
                out.appendSlice(allocator, "--") catch return SmtpError.OutOfMemory;
                out.appendSlice(allocator, boundary) catch return SmtpError.OutOfMemory;
                out.appendSlice(allocator, "\r\nContent-Type: ") catch return SmtpError.OutOfMemory;
                out.appendSlice(allocator, att.content_type) catch return SmtpError.OutOfMemory;
                out.appendSlice(allocator, "; name=\"") catch return SmtpError.OutOfMemory;
                out.appendSlice(allocator, att.filename) catch return SmtpError.OutOfMemory;
                out.appendSlice(allocator, "\"\r\nContent-Disposition: attachment; filename=\"") catch return SmtpError.OutOfMemory;
                out.appendSlice(allocator, att.filename) catch return SmtpError.OutOfMemory;
                out.appendSlice(allocator, "\"\r\nContent-Transfer-Encoding: base64\r\n\r\n") catch return SmtpError.OutOfMemory;
                appendBase64Wrapped76(allocator, &out, att.data)?;
            }

            out.appendSlice(allocator, "--") catch return SmtpError.OutOfMemory;
            out.appendSlice(allocator, boundary) catch return SmtpError.OutOfMemory;
            out.appendSlice(allocator, "--\r\n") catch return SmtpError.OutOfMemory;
        } else if (has_text && has_html) {
            out.appendSlice(allocator, "Content-Type: multipart/alternative; boundary=\"") catch return SmtpError.OutOfMemory;
            out.appendSlice(allocator, boundary) catch return SmtpError.OutOfMemory;
            out.appendSlice(allocator, "\"\r\n\r\n") catch return SmtpError.OutOfMemory;

            out.appendSlice(allocator, "--") catch return SmtpError.OutOfMemory;
            out.appendSlice(allocator, boundary) catch return SmtpError.OutOfMemory;
            out.appendSlice(allocator, "\r\nContent-Type: text/plain; charset=\"UTF-8\"\r\nContent-Transfer-Encoding: 8bit\r\n\r\n") catch return SmtpError.OutOfMemory;
            out.appendSlice(allocator, _.text_body.?) catch return SmtpError.OutOfMemory;
            out.appendSlice(allocator, "\r\n") catch return SmtpError.OutOfMemory;

            out.appendSlice(allocator, "--") catch return SmtpError.OutOfMemory;
            out.appendSlice(allocator, boundary) catch return SmtpError.OutOfMemory;
            out.appendSlice(allocator, "\r\nContent-Type: text/html; charset=\"UTF-8\"\r\nContent-Transfer-Encoding: 8bit\r\n\r\n") catch return SmtpError.OutOfMemory;
            out.appendSlice(allocator, _.html_body.?) catch return SmtpError.OutOfMemory;
            out.appendSlice(allocator, "\r\n--") catch return SmtpError.OutOfMemory;
            out.appendSlice(allocator, boundary) catch return SmtpError.OutOfMemory;
            out.appendSlice(allocator, "--\r\n") catch return SmtpError.OutOfMemory;
        } else if (has_html) {
            out.appendSlice(allocator, "Content-Type: text/html; charset=\"UTF-8\"\r\nContent-Transfer-Encoding: 8bit\r\n\r\n") catch return SmtpError.OutOfMemory;
            out.appendSlice(allocator, _.html_body.?) catch return SmtpError.OutOfMemory;
            out.appendSlice(allocator, "\r\n") catch return SmtpError.OutOfMemory;
        } else {
            out.appendSlice(allocator, "Content-Type: text/plain; charset=\"UTF-8\"\r\nContent-Transfer-Encoding: 8bit\r\n\r\n") catch return SmtpError.OutOfMemory;
            if (txt := _.text_body) {
                out.appendSlice(allocator, txt) catch return SmtpError.OutOfMemory;
            }
            out.appendSlice(allocator, "\r\n") catch return SmtpError.OutOfMemory;
        }

        return out.toOwnedSlice(allocator) catch return SmtpError.OutOfMemory;
    }
}
