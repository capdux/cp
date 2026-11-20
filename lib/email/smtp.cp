// Source: cp/lib/email/smtp.cp (RFC 5321 SMTP Command Encoder, Dot-Stuffing & Reply Parser)
// Pure CP RFC 5321 SMTP wire command encoder, RFC 4616 AUTH PLAIN encoder,
// RFC 5321 §4.5.2 dot-stuffing/unstuffing codec, and single/multi-line reply parser.

const std = @import("std");
import { Base64 } from "cap:encoding";
import { EmailError, EmailAddress, containsHeaderInjection } from "./address.cp";

fn trimTrailingCrLf(s: string): string {
    let end = s.len;
    while (end > 0 && (s[end - 1] == '\r' || s[end - 1] == '\n')) : (end -= 1) {}
    return s[0..end];
}

fn parseThreeDigitCode(s: string): u16? {
    if (s.len < 3) return null;
    if (s[0] < '0' || s[0] > '9' || s[1] < '0' || s[1] > '9' || s[2] < '0' || s[2] > '9') {
        return null;
    }
    const code = ((s[0] - '0') as u16) * 100 + ((s[1] - '0') as u16) * 10 + ((s[2] - '0') as u16);
    if (code < 100 || code > 599) return null;
    return code;
}

fn isValidEhloDomain(domain: string): bool {
    if (domain.len == 0 || domain.len > 255) return false;
    let i: usize = 0;
    while (i < domain.len) : (i += 1) {
        const ch = domain[i];
        if (ch == ' ' || ch == '\t' || ch <= 0x20 || ch >= 0x7F) {
            return false;
        }
    }
    return true;
}

/// Parsed RFC 5321 SMTP server reply (supports both single-line `250 OK` and multi-line `250-...` responses).
pub struct SmtpReply {
    code: u16 = 0,
    is_multiline: bool = false,
    message: string = "",

    /// Returns `true` for RFC 5321 positive completion (`2xx`) and positive intermediate (`3xx`) replies.
    pub fn isPositive(): bool {
        return _.code >= 200 && _.code < 400;
    }

    /// Returns `true` strictly for `2xx` positive completion replies.
    pub fn isPositiveCompletion(): bool {
        return _.code >= 200 && _.code < 300;
    }

    /// Returns `true` for `3xx` positive intermediate replies (e.g. `334`, `354`).
    pub fn isPositiveIntermediate(): bool {
        return _.code >= 300 && _.code < 400;
    }

    /// Returns `true` for `4xx` transient negative completion replies.
    pub fn isTransientNegative(): bool {
        return _.code >= 400 && _.code < 500;
    }

    /// Returns `true` for `5xx` permanent negative completion replies.
    pub fn isPermanentNegative(): bool {
        return _.code >= 500 && _.code < 600;
    }
}

/// RFC 5321 SMTP wire command encoder, dot-stuffing transparency codec, and reply parser.
pub static struct SmtpCodec {
    /// Encodes `EHLO <domain>\r\n` after rejecting whitespace and CRLF/NUL injection.
    pub static fn encodeEhlo(alloc: std.mem.Allocator, domain: string): ([]u8) ! EmailError {
        if (containsHeaderInjection(domain)) {
            return EmailError.HeaderInjection;
        }
        if (!isValidEhloDomain(domain)) {
            return EmailError.InvalidAddress;
        }
        return std.fmt.allocPrint(alloc, "EHLO {s}\r\n", .{ domain }) catch return EmailError.OutOfMemory;
    }

    /// Encodes `MAIL FROM:<local@domain>\r\n` (accepts empty `""` for null reverse-path bounce `MAIL FROM:<>`).
    pub static fn encodeMailFrom(alloc: std.mem.Allocator, reverse_path: string): ([]u8) ! EmailError {
        if (containsHeaderInjection(reverse_path)) {
            return EmailError.HeaderInjection;
        }
        if (reverse_path.len == 0) {
            return alloc.dupe(u8, "MAIL FROM:<>\r\n") catch return EmailError.OutOfMemory;
        }
        const parsed = EmailAddress.parse(reverse_path)?;
        const bare = parsed.emailOnly(alloc)?;
        defer alloc.free(bare);
        return std.fmt.allocPrint(alloc, "MAIL FROM:<{s}>\r\n", .{ bare }) catch return EmailError.OutOfMemory;
    }

    /// Encodes `RCPT TO:<local@domain>\r\n` after RFC 5322 validation and CRLF injection checks.
    pub static fn encodeRcptTo(alloc: std.mem.Allocator, forward_path: string): ([]u8) ! EmailError {
        if (containsHeaderInjection(forward_path)) {
            return EmailError.HeaderInjection;
        }
        const parsed = EmailAddress.parse(forward_path)?;
        const bare = parsed.emailOnly(alloc)?;
        defer alloc.free(bare);
        return std.fmt.allocPrint(alloc, "RCPT TO:<{s}>\r\n", .{ bare }) catch return EmailError.OutOfMemory;
    }

    /// Encodes `AUTH PLAIN <base64(\0user\0pass)>\r\n` per RFC 4616.
    pub static fn encodeAuthPlain(alloc: std.mem.Allocator, user: string, pass: string): ([]u8) ! EmailError {
        if (containsHeaderInjection(user) || containsHeaderInjection(pass)) {
            return EmailError.HeaderInjection;
        }
        const raw_len = 1 + user.len + 1 + pass.len;
        let raw = alloc.alloc(u8, raw_len) catch return EmailError.OutOfMemory;
        defer alloc.free(raw);

        raw[0] = 0;
        @memcpy(raw[1 .. 1 + user.len], user);
        raw[1 + user.len] = 0;
        @memcpy(raw[2 + user.len .. raw_len], pass);

        const b64 = Base64.encodeAlloc(alloc, raw) catch return EmailError.OutOfMemory;
        defer alloc.free(b64);

        return std.fmt.allocPrint(alloc, "AUTH PLAIN {s}\r\n", .{ b64 }) catch return EmailError.OutOfMemory;
    }

    /// Applies RFC 5321 §4.5.2 dot-stuffing (doubling leading `.` on any line) and appends `\r\n.\r\n`.
    pub static fn dotStuffBody(alloc: std.mem.Allocator, raw_msg: []const u8): ([]u8) ! EmailError {
        let out: std.ArrayListUnmanaged(u8) = .empty;
        errdefer out.deinit(alloc);

        let at_line_start = true;
        let i: usize = 0;
        while (i < raw_msg.len) : (i += 1) {
            const ch = raw_msg[i];
            if (at_line_start && ch == '.') {
                out.append(alloc, '.') catch return EmailError.OutOfMemory;
            }
            out.append(alloc, ch) catch return EmailError.OutOfMemory;
            at_line_start = (ch == '\n');
        }

        out.appendSlice(alloc, "\r\n.\r\n") catch return EmailError.OutOfMemory;
        return out.toOwnedSlice(alloc) catch return EmailError.OutOfMemory;
    }

    /// Reverses RFC 5321 §4.5.2 dot-stuffing and strips the trailing `\r\n.\r\n` DATA terminator.
    pub static fn dotUnstuffBody(alloc: std.mem.Allocator, stuffed: []const u8): ([]u8) ! EmailError {
        let content = stuffed;
        if (std.mem.endsWith(u8, content, "\r\n.\r\n")) {
            content = content[0 .. content.len - 5];
        } else if (std.mem.endsWith(u8, content, "\n.\n")) {
            content = content[0 .. content.len - 3];
        } else if (std.mem.endsWith(u8, content, "\r\n.")) {
            content = content[0 .. content.len - 3];
        } else if (std.mem.eql(u8, content, ".\r\n") || std.mem.eql(u8, content, ".")) {
            content = "";
        }

        let out: std.ArrayListUnmanaged(u8) = .empty;
        errdefer out.deinit(alloc);

        let at_line_start = true;
        let i: usize = 0;
        while (i < content.len) : (i += 1) {
            const ch = content[i];
            if (at_line_start && ch == '.' && i + 1 < content.len && content[i + 1] == '.') {
                // Skip the stuffed extra leading dot
                at_line_start = false;
                continue;
            }
            out.append(alloc, ch) catch return EmailError.OutOfMemory;
            at_line_start = (ch == '\n');
        }

        return out.toOwnedSlice(alloc) catch return EmailError.OutOfMemory;
    }

    /// Parses single-line (`250 OK\r\n`) or multi-line (`250-...\r\n250 ...\r\n`) RFC 5321 SMTP replies.
    pub static fn parseReply(raw_reply: string): SmtpReply ! EmailError {
        const trimmed = trimTrailingCrLf(raw_reply);
        if (trimmed.len < 3) {
            return EmailError.InvalidSmtpReply;
        }

        let expected_code: u16? = null;
        let is_multiline = false;
        let line_count: usize = 0;
        let saw_final_line = false;

        let line_iter = std.mem.splitScalar(u8, trimmed, '\n');
        while (raw_line := line_iter.next()) {
            const line = trimTrailingCrLf(raw_line);
            if (line.len == 0) {
                return EmailError.InvalidSmtpReply;
            }
            const code_opt = parseThreeDigitCode(line);
            if (code_opt == null) {
                return EmailError.InvalidSmtpReply;
            }
            const code = code_opt.?;

            if (exp := expected_code) {
                if (code != exp) {
                    return EmailError.InvalidSmtpReply;
                }
            } else {
                expected_code = code;
            }

            if (line.len == 3) {
                saw_final_line = true;
            } else {
                const sep = line[3];
                if (sep == '-') {
                    is_multiline = true;
                } else if (sep == ' ') {
                    saw_final_line = true;
                } else {
                    return EmailError.InvalidSmtpReply;
                }
            }
            line_count += 1;
        }

        if (line_count == 0 || expected_code == null) {
            return EmailError.InvalidSmtpReply;
        }
        if (is_multiline && !saw_final_line) {
            return EmailError.InvalidSmtpReply;
        }

        const msg = if (trimmed.len > 4) trimmed[4..] else "";
        return SmtpReply{
            code: expected_code.?,
            is_multiline: is_multiline,
            message: msg,
        };
    }
}
