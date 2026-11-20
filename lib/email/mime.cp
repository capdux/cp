// Source: cp/lib/email/mime.cp (RFC 5322 / RFC 2045 MIME Composer, Quoted-Printable & Parser)
// Pure CP RFC 5322 / RFC 2045 / RFC 2047 MIME builder, Quoted-Printable codec, and recursive MIME parser.

const std = @import("std");
import { Base64 } from "cap:encoding";
import { EmailError, EmailAddress, containsHeaderInjection } from "./address.cp";

const HEX_UPPER: string = "0123456789ABCDEF";
const QP_MAX_LINE: usize = 76;
const B64_WRAP_COLS: usize = 76;
const RFC2047_CHUNK_MAX_BYTES: usize = 45;

fn hexDigitVal(ch: u8): u8? {
    if (ch >= '0' && ch <= '9') return ch - '0';
    if (ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
    if (ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
    return null;
}

fn isAsciiPrintableNonEq(ch: u8): bool {
    return (ch >= 33 && ch <= 60) || (ch >= 62 && ch <= 126);
}

fn hasNonAsciiOrCtl(s: string): bool {
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        const ch = s[i];
        if (ch >= 0x80 || ch < 0x20 && ch != '\t' || ch == 0x7F) {
            return true;
        }
    }
    return false;
}

fn utf8SeqLen(first_byte: u8): usize {
    if (first_byte < 0x80) return 1;
    if ((first_byte & 0xE0) == 0xC0) return 2;
    if ((first_byte & 0xF0) == 0xE0) return 3;
    if ((first_byte & 0xF8) == 0xF0) return 4;
    return 1;
}

fn appendBase64Slice(buf: *std.ArrayListUnmanaged(u8), alloc: std.mem.Allocator, raw: []const u8): void ! EmailError {
    const enc_len = Base64.encodeLen(raw.len);
    const old_len = buf.items.len;
    buf.resize(alloc, old_len + enc_len) catch return EmailError.OutOfMemory;
    const written = Base64.encode(buf.items[old_len .. old_len + enc_len], raw);
    buf.items.len = old_len + written;
}

fn appendBase64Wrapped(buf: *std.ArrayListUnmanaged(u8), alloc: std.mem.Allocator, raw: []const u8): void ! EmailError {
    if (raw.len > 0) {
        const raw_b64 = Base64.encodeAlloc(alloc, raw) catch return EmailError.OutOfMemory;
        defer alloc.free(raw_b64);

        let pos: usize = 0;
        while (pos < raw_b64.len) {
            const end = @min(pos + B64_WRAP_COLS, raw_b64.len);
            buf.appendSlice(alloc, raw_b64[pos..end]) catch return EmailError.OutOfMemory;
            buf.appendSlice(alloc, "\r\n") catch return EmailError.OutOfMemory;
            pos = end;
        }
    }
}

fn appendRfc2047HeaderValue(buf: *std.ArrayListUnmanaged(u8), alloc: std.mem.Allocator, value: string): void ! EmailError {
    if (!hasNonAsciiOrCtl(value)) {
        buf.appendSlice(alloc, value) catch return EmailError.OutOfMemory;
    } else {
        let pos: usize = 0;
        let first = true;
        while (pos < value.len) {
            let end = pos;
            while (end < value.len) {
                const step = utf8SeqLen(value[end]);
                const next_end = @min(end + step, value.len);
                if (next_end - pos > RFC2047_CHUNK_MAX_BYTES && end > pos) {
                    break;
                }
                end = next_end;
                if (end - pos >= RFC2047_CHUNK_MAX_BYTES) {
                    break;
                }
            }
            if (end == pos) {
                end = @min(pos + RFC2047_CHUNK_MAX_BYTES, value.len);
            }

            if (!first) {
                buf.appendSlice(alloc, "\r\n ") catch return EmailError.OutOfMemory;
            }
            first = false;

            buf.appendSlice(alloc, "=?UTF-8?B?") catch return EmailError.OutOfMemory;
            appendBase64Slice(buf, alloc, value[pos..end])?;
            buf.appendSlice(alloc, "?=") catch return EmailError.OutOfMemory;
            pos = end;
        }
    }
}

fn decodeBase64CleanAlloc(alloc: std.mem.Allocator, raw: []const u8): ([]u8) ! EmailError {
    let clean: std.ArrayListUnmanaged(u8) = .empty;
    defer clean.deinit(alloc);

    let i: usize = 0;
    while (i < raw.len) : (i += 1) {
        const ch = raw[i];
        if (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n') continue;
        clean.append(alloc, ch) catch return EmailError.OutOfMemory;
    }

    if (clean.items.len == 0) {
        return alloc.alloc(u8, 0) catch return EmailError.OutOfMemory;
    }

    if (!Base64.isValid(clean.items)) {
        return EmailError.InvalidMime;
    }

    const exact_len = Base64.exactDecodedLen(clean.items);
    let out = alloc.alloc(u8, exact_len) catch return EmailError.OutOfMemory;
    errdefer alloc.free(out);

    const decoded_len = Base64.decode(out, clean.items) catch return EmailError.InvalidMime;
    if (decoded_len == out.len) {
        return out;
    }
    const shrunk = alloc.dupe(u8, out[0..decoded_len]) catch return EmailError.OutOfMemory;
    alloc.free(out);
    return shrunk;
}

fn decodeRfc2047WordQ(alloc: std.mem.Allocator, payload: []const u8): ([]u8) ! EmailError {
    let out: std.ArrayListUnmanaged(u8) = .empty;
    errdefer out.deinit(alloc);

    let i: usize = 0;
    while (i < payload.len) {
        const ch = payload[i];
        if (ch == '_') {
            out.append(alloc, ' ') catch return EmailError.OutOfMemory;
            i += 1;
        } else if (ch == '=' && i + 2 < payload.len) {
            if (hi := hexDigitVal(payload[i + 1])) {
                if (lo := hexDigitVal(payload[i + 2])) {
                    out.append(alloc, (hi << 4) | lo) catch return EmailError.OutOfMemory;
                    i += 3;
                    continue;
                }
            }
            return EmailError.InvalidMime;
        } else {
            out.append(alloc, ch) catch return EmailError.OutOfMemory;
            i += 1;
        }
    }
    return out.toOwnedSlice(alloc) catch return EmailError.OutOfMemory;
}

fn decodeRfc2047Header(alloc: std.mem.Allocator, raw: string): ([]u8) ! EmailError {
    let out: std.ArrayListUnmanaged(u8) = .empty;
    errdefer out.deinit(alloc);

    let pos: usize = 0;
    let prev_was_encoded = false;

    while (pos < raw.len) {
        const start_opt = std.mem.indexOfPos(u8, raw, pos, "=?");
        if (start_opt == null) {
            out.appendSlice(alloc, raw[pos..]) catch return EmailError.OutOfMemory;
            break;
        }
        const start = start_opt.?;

        // Check if gap between two encoded-words is strictly linear whitespace (RFC 2047 §6.2)
        const between = raw[pos..start];
        let only_ws = true;
        let k: usize = 0;
        while (k < between.len) : (k += 1) {
            if (between[k] != ' ' && between[k] != '\t') {
                only_ws = false;
                break;
            }
        }

        const q1_opt = std.mem.indexOfScalarPos(u8, raw, start + 2, '?');
        if (q1_opt == null) {
            out.appendSlice(alloc, raw[pos..]) catch return EmailError.OutOfMemory;
            break;
        }
        const q1 = q1_opt.?;

        const q2_opt = std.mem.indexOfScalarPos(u8, raw, q1 + 1, '?');
        if (q2_opt == null) {
            out.appendSlice(alloc, raw[pos..]) catch return EmailError.OutOfMemory;
            break;
        }
        const q2 = q2_opt.?;

        const end_opt = std.mem.indexOfPos(u8, raw, q2 + 1, "?=");
        if (end_opt == null) {
            out.appendSlice(alloc, raw[pos..]) catch return EmailError.OutOfMemory;
            break;
        }
        const end_marker = end_opt.?;

        const enc = raw[q1 + 1 .. q2];
        const payload = raw[q2 + 1 .. end_marker];

        if (!(prev_was_encoded && only_ws)) {
            out.appendSlice(alloc, between) catch return EmailError.OutOfMemory;
        }

        if (enc.len == 1 && (enc[0] == 'B' || enc[0] == 'b')) {
            const decoded = decodeBase64CleanAlloc(alloc, payload) catch {
                out.appendSlice(alloc, raw[start .. end_marker + 2]) catch return EmailError.OutOfMemory;
                prev_was_encoded = false;
                pos = end_marker + 2;
                continue;
            };
            defer alloc.free(decoded);
            out.appendSlice(alloc, decoded) catch return EmailError.OutOfMemory;
            prev_was_encoded = true;
            pos = end_marker + 2;
        } else if (enc.len == 1 && (enc[0] == 'Q' || enc[0] == 'q')) {
            const decoded = decodeRfc2047WordQ(alloc, payload) catch {
                out.appendSlice(alloc, raw[start .. end_marker + 2]) catch return EmailError.OutOfMemory;
                prev_was_encoded = false;
                pos = end_marker + 2;
                continue;
            };
            defer alloc.free(decoded);
            out.appendSlice(alloc, decoded) catch return EmailError.OutOfMemory;
            prev_was_encoded = true;
            pos = end_marker + 2;
        } else {
            out.appendSlice(alloc, raw[start .. end_marker + 2]) catch return EmailError.OutOfMemory;
            prev_was_encoded = false;
            pos = end_marker + 2;
        }
    }

    return out.toOwnedSlice(alloc) catch return EmailError.OutOfMemory;
}

/// RFC 2045 §6.7 Quoted-Printable encoder & decoder with soft line break (`=\r\n`) support.
pub static struct QuotedPrintable {
    /// Encodes `input` using RFC 2045 Quoted-Printable with 76-column soft line breaks (`=\r\n`).
    pub static fn encode(alloc: std.mem.Allocator, input: []const u8): ([]u8) ! EmailError {
        let out: std.ArrayListUnmanaged(u8) = .empty;
        errdefer out.deinit(alloc);

        let col: usize = 0;
        let i: usize = 0;
        while (i < input.len) {
            const ch = input[i];

            // Preserve CRLF hard line breaks cleanly
            if (ch == '\r' && i + 1 < input.len && input[i + 1] == '\n') {
                out.appendSlice(alloc, "\r\n") catch return EmailError.OutOfMemory;
                col = 0;
                i += 2;
                continue;
            }

            // Space/tab right before CRLF or EOF must be hex-encoded per RFC 2045 §6.7
            const is_ws = (ch == ' ' || ch == '\t');
            const at_eol = (i + 1 == input.len) || (i + 2 < input.len && input[i + 1] == '\r' && input[i + 2] == '\n');

            if ((isAsciiPrintableNonEq(ch) || (is_ws && !at_eol))) {
                const is_last = (i + 1 == input.len);
                if (col + 1 > QP_MAX_LINE || (col + 1 == QP_MAX_LINE && !is_last)) {
                    out.appendSlice(alloc, "=\r\n") catch return EmailError.OutOfMemory;
                    col = 0;
                }
                out.append(alloc, ch) catch return EmailError.OutOfMemory;
                col += 1;
                i += 1;
            } else {
                const is_last = (i + 1 == input.len);
                if (col + 3 > QP_MAX_LINE || (col + 3 == QP_MAX_LINE && !is_last)) {
                    out.appendSlice(alloc, "=\r\n") catch return EmailError.OutOfMemory;
                    col = 0;
                }
                out.append(alloc, '=') catch return EmailError.OutOfMemory;
                out.append(alloc, HEX_UPPER[(ch >> 4) & 0x0F]) catch return EmailError.OutOfMemory;
                out.append(alloc, HEX_UPPER[ch & 0x0F]) catch return EmailError.OutOfMemory;
                col += 3;
                i += 1;
            }
        }

        return out.toOwnedSlice(alloc) catch return EmailError.OutOfMemory;
    }

    /// Decodes RFC 2045 Quoted-Printable `input`, stripping `=\r\n` / `=\n` soft line breaks and decoding `=XX`.
    pub static fn decode(alloc: std.mem.Allocator, input: []const u8): ([]u8) ! EmailError {
        let out: std.ArrayListUnmanaged(u8) = .empty;
        errdefer out.deinit(alloc);

        let i: usize = 0;
        while (i < input.len) {
            const ch = input[i];
            if (ch == '=') {
                // Soft line break (with optional transport padding spaces/tabs before CRLF or LF)
                let j = i + 1;
                while (j < input.len && (input[j] == ' ' || input[j] == '\t')) : (j += 1) {}
                if (j + 1 < input.len && input[j] == '\r' && input[j + 1] == '\n') {
                    i = j + 2;
                    continue;
                }
                if (j < input.len && input[j] == '\n') {
                    i = j + 1;
                    continue;
                }

                if (i + 2 >= input.len) {
                    return EmailError.InvalidMime;
                }
                const hi_opt = hexDigitVal(input[i + 1]);
                if (hi_opt == null) return EmailError.InvalidMime;
                const hi = hi_opt.?;
                const lo_opt = hexDigitVal(input[i + 2]);
                if (lo_opt == null) return EmailError.InvalidMime;
                const lo = lo_opt.?;
                out.append(alloc, (hi << 4) | lo) catch return EmailError.OutOfMemory;
                i += 3;
            } else {
                out.append(alloc, ch) catch return EmailError.OutOfMemory;
                i += 1;
            }
        }

        return out.toOwnedSlice(alloc) catch return EmailError.OutOfMemory;
    }
}

/// Email attachment descriptor carrying filename, MIME content type, and raw payload bytes.
pub struct EmailAttachment {
    filename: string = "",
    content_type: string = "application/octet-stream",
    data: []const u8 = "",

    pub static fn init(filename: string, content_type: string, data: []const u8): EmailAttachment {
        return EmailAttachment{
            filename: filename,
            content_type: content_type,
            data: data,
        };
    }
}

/// Unfolded RFC 5322 header name/value pair.
pub struct EmailHeader {
    name: string = "",
    value: string = "",
}

/// Parsed MIME part with headers and decoded payload.
pub struct ParsedPart {
    content_type: string = "",
    content_transfer_encoding: string = "",
    content_disposition: string = "",
    filename: string? = null,
    headers: []const EmailHeader = [],
    body: []u8 = [],
}

/// Parsed RFC 5322 / MIME email message with unfolded headers, decoded UTF-8 subject, bodies, and attachments.
pub struct ParsedEmail {
    alloc: std.mem.Allocator = undefined,
    headers: []EmailHeader = [],
    from: string = "",
    to: string = "",
    cc: string = "",
    subject: string = "",
    raw_subject: string = "",
    content_type: string = "",
    text_body: string? = null,
    html_body: string? = null,
    raw_body: string = "",
    attachments: []EmailAttachment = [],
    parts: []ParsedPart = [],
    owned_slices: std.ArrayListUnmanaged([]u8) = .empty,
    owned_headers: std.ArrayListUnmanaged([]EmailHeader) = .empty,

    /// Looks up the first header matching `name` (ASCII case-insensitive).
    pub fn getHeader(name: string): string? {
        for (h of _.headers) {
            if (std.ascii.eqlIgnoreCase(h.name, name)) {
                return h.value;
            }
        }
        return null;
    }

    /// Releases all memory allocated during parsing.
    pub fn deinit(*_) {
        for (s of _.owned_slices.items) {
            _.alloc.free(s);
        }
        _.owned_slices.deinit(_.alloc);

        for (hs of _.owned_headers.items) {
            _.alloc.free(hs);
        }
        _.owned_headers.deinit(_.alloc);

        if (_.attachments.len > 0) {
            _.alloc.free(_.attachments);
        }
        if (_.parts.len > 0) {
            _.alloc.free(_.parts);
        }
    }
}

/// RFC 5322 / RFC 2045 MIME email composer with header injection protection, RFC 2047 UTF-8 subject encoding,
/// automatic `multipart/alternative` / `multipart/mixed` layout, and 76-column Base64 attachments.
pub struct EmailBuilder {
    alloc: std.mem.Allocator = undefined,
    from_addr: EmailAddress? = null,
    to_list: std.ArrayListUnmanaged(EmailAddress) = .empty,
    cc_list: std.ArrayListUnmanaged(EmailAddress) = .empty,
    subject: string = "",
    text_body: string? = null,
    html_body: string? = null,
    attachments: std.ArrayListUnmanaged(EmailAttachment) = .empty,
    custom_headers: std.ArrayListUnmanaged(EmailHeader) = .empty,
    boundary_alt: string = "----=_CP_Email_Alt_7a9e31f0",
    boundary_mix: string = "----=_CP_Email_Mix_9c4b12d8",

    /// Initializes a new `EmailBuilder` using `alloc` for internal recipient/attachment lists.
    pub static fn init(alloc: std.mem.Allocator): EmailBuilder {
        return EmailBuilder{
            alloc: alloc,
        };
    }

    /// Releases internal builder lists.
    pub fn deinit(*_) {
        _.to_list.deinit(_.alloc);
        _.cc_list.deinit(_.alloc);
        _.attachments.deinit(_.alloc);
        _.custom_headers.deinit(_.alloc);
    }

    /// Sets the `From` address after RFC 5322 validation and CRLF injection checks.
    pub fn setFrom(*_, raw_addr: string): void ! EmailError {
        _.from_addr = EmailAddress.parse(raw_addr)?;
    }

    /// Appends a `To` recipient after RFC 5322 validation and CRLF injection checks.
    pub fn addTo(*_, raw_addr: string): void ! EmailError {
        const parsed = EmailAddress.parse(raw_addr)?;
        _.to_list.append(_.alloc, parsed) catch return EmailError.OutOfMemory;
    }

    /// Appends a `Cc` recipient after RFC 5322 validation and CRLF injection checks.
    pub fn addCc(*_, raw_addr: string): void ! EmailError {
        const parsed = EmailAddress.parse(raw_addr)?;
        _.cc_list.append(_.alloc, parsed) catch return EmailError.OutOfMemory;
    }

    /// Sets the `Subject` header, rejecting any CR/LF/NUL header injection. Non-ASCII is encoded via RFC 2047 on `build`.
    pub fn setSubject(*_, subject: string): void ! EmailError {
        if (containsHeaderInjection(subject)) {
            return EmailError.HeaderInjection;
        }
        _.subject = subject;
    }

    /// Sets the plain-text body (`text/plain; charset=UTF-8`).
    pub fn setTextBody(*_, body: string) {
        _.text_body = body;
    }

    /// Sets the HTML body (`text/html; charset=UTF-8`).
    pub fn setHtmlBody(*_, body: string) {
        _.html_body = body;
    }

    /// Appends an attachment after checking `filename` and `content_type` for header injection.
    pub fn addAttachment(*_, attachment: EmailAttachment): void ! EmailError {
        if (containsHeaderInjection(attachment.filename) || containsHeaderInjection(attachment.content_type)) {
            return EmailError.HeaderInjection;
        }
        if (attachment.filename.len == 0 || attachment.content_type.len == 0) {
            return EmailError.InvalidMime;
        }
        _.attachments.append(_.alloc, attachment) catch return EmailError.OutOfMemory;
    }

    /// Adds a custom RFC 5322 header after verifying both `name` and `value` against CRLF/NUL injection.
    pub fn addHeader(*_, name: string, value: string): void ! EmailError {
        if (containsHeaderInjection(name) || containsHeaderInjection(value)) {
            return EmailError.HeaderInjection;
        }
        if (name.len == 0) {
            return EmailError.InvalidMime;
        }
        _.custom_headers.append(_.alloc, EmailHeader{ name: name, value: value }) catch return EmailError.OutOfMemory;
    }

    fn appendAddressHeader(buf: *std.ArrayListUnmanaged(u8), alloc: std.mem.Allocator, header_name: string, list: []const EmailAddress): void ! EmailError {
        if (list.len > 0) {
            buf.appendSlice(alloc, header_name) catch return EmailError.OutOfMemory;
            buf.appendSlice(alloc, ": ") catch return EmailError.OutOfMemory;
            let idx: usize = 0;
            while (idx < list.len) : (idx += 1) {
                if (idx > 0) {
                    buf.appendSlice(alloc, ", ") catch return EmailError.OutOfMemory;
                }
                const formatted = list[idx].format(alloc)?;
                defer alloc.free(formatted);
                buf.appendSlice(alloc, formatted) catch return EmailError.OutOfMemory;
            }
            buf.appendSlice(alloc, "\r\n") catch return EmailError.OutOfMemory;
        }
    }

    fn appendTextLeafPart(buf: *std.ArrayListUnmanaged(u8), alloc: std.mem.Allocator, media_type: string, body: string, trailing_crlf: bool): void ! EmailError {
        buf.appendSlice(alloc, "Content-Type: ") catch return EmailError.OutOfMemory;
        buf.appendSlice(alloc, media_type) catch return EmailError.OutOfMemory;
        buf.appendSlice(alloc, "; charset=UTF-8\r\n") catch return EmailError.OutOfMemory;
        buf.appendSlice(alloc, "Content-Transfer-Encoding: quoted-printable\r\n\r\n") catch return EmailError.OutOfMemory;

        const encoded = QuotedPrintable.encode(alloc, body)?;
        defer alloc.free(encoded);
        buf.appendSlice(alloc, encoded) catch return EmailError.OutOfMemory;
        if (trailing_crlf) {
            buf.appendSlice(alloc, "\r\n") catch return EmailError.OutOfMemory;
        }
    }

    fn appendAlternativeSection(buf: *std.ArrayListUnmanaged(u8), alloc: std.mem.Allocator, boundary: string, text_body: string, html_body: string): void ! EmailError {
        buf.appendSlice(alloc, "Content-Type: multipart/alternative; boundary=\"") catch return EmailError.OutOfMemory;
        buf.appendSlice(alloc, boundary) catch return EmailError.OutOfMemory;
        buf.appendSlice(alloc, "\"\r\n\r\n") catch return EmailError.OutOfMemory;

        buf.appendSlice(alloc, "--") catch return EmailError.OutOfMemory;
        buf.appendSlice(alloc, boundary) catch return EmailError.OutOfMemory;
        buf.appendSlice(alloc, "\r\n") catch return EmailError.OutOfMemory;
        appendTextLeafPart(buf, alloc, "text/plain", text_body, true)?;

        buf.appendSlice(alloc, "--") catch return EmailError.OutOfMemory;
        buf.appendSlice(alloc, boundary) catch return EmailError.OutOfMemory;
        buf.appendSlice(alloc, "\r\n") catch return EmailError.OutOfMemory;
        appendTextLeafPart(buf, alloc, "text/html", html_body, true)?;

        buf.appendSlice(alloc, "--") catch return EmailError.OutOfMemory;
        buf.appendSlice(alloc, boundary) catch return EmailError.OutOfMemory;
        buf.appendSlice(alloc, "--\r\n") catch return EmailError.OutOfMemory;
    }

    fn appendQuotedParamValue(buf: *std.ArrayListUnmanaged(u8), alloc: std.mem.Allocator, s: string): void ! EmailError {
        let i: usize = 0;
        while (i < s.len) : (i += 1) {
            const ch = s[i];
            if (ch == '"' || ch == '\\') {
                buf.append(alloc, '\\') catch return EmailError.OutOfMemory;
            }
            buf.append(alloc, ch) catch return EmailError.OutOfMemory;
        }
    }

    /// Serializes the complete RFC 5322 / MIME message into an owned byte slice allocated from `alloc`.
    pub fn build(alloc: std.mem.Allocator): ([]u8) ! EmailError {
        if (_.from_addr == null) {
            return EmailError.InvalidAddress;
        }
        const from_val = _.from_addr.?;

        let out: std.ArrayListUnmanaged(u8) = .empty;
        errdefer out.deinit(alloc);

        // 1. Standard RFC 5322 Headers
        const from_fmt = from_val.format(alloc)?;
        defer alloc.free(from_fmt);
        out.appendSlice(alloc, "From: ") catch return EmailError.OutOfMemory;
        out.appendSlice(alloc, from_fmt) catch return EmailError.OutOfMemory;
        out.appendSlice(alloc, "\r\n") catch return EmailError.OutOfMemory;

        appendAddressHeader(&out, alloc, "To", _.to_list.items)?;
        appendAddressHeader(&out, alloc, "Cc", _.cc_list.items)?;

        if (_.subject.len > 0) {
            out.appendSlice(alloc, "Subject: ") catch return EmailError.OutOfMemory;
            appendRfc2047HeaderValue(&out, alloc, _.subject)?;
            out.appendSlice(alloc, "\r\n") catch return EmailError.OutOfMemory;
        }

        for (h of _.custom_headers.items) {
            out.appendSlice(alloc, h.name) catch return EmailError.OutOfMemory;
            out.appendSlice(alloc, ": ") catch return EmailError.OutOfMemory;
            appendRfc2047HeaderValue(&out, alloc, h.value)?;
            out.appendSlice(alloc, "\r\n") catch return EmailError.OutOfMemory;
        }

        out.appendSlice(alloc, "MIME-Version: 1.0\r\n") catch return EmailError.OutOfMemory;

        // 2. Adaptive MIME Body Layout
        const has_att = _.attachments.items.len > 0;
        const has_text = _.text_body != null;
        const has_html = _.html_body != null;

        if (has_att) {
            out.appendSlice(alloc, "Content-Type: multipart/mixed; boundary=\"") catch return EmailError.OutOfMemory;
            out.appendSlice(alloc, _.boundary_mix) catch return EmailError.OutOfMemory;
            out.appendSlice(alloc, "\"\r\n\r\n") catch return EmailError.OutOfMemory;

            if (has_text || has_html) {
                out.appendSlice(alloc, "--") catch return EmailError.OutOfMemory;
                out.appendSlice(alloc, _.boundary_mix) catch return EmailError.OutOfMemory;
                out.appendSlice(alloc, "\r\n") catch return EmailError.OutOfMemory;

                if (has_text && has_html) {
                    appendAlternativeSection(&out, alloc, _.boundary_alt, _.text_body.?, _.html_body.?)?;
                } else if (has_text) {
                    appendTextLeafPart(&out, alloc, "text/plain", _.text_body.?, true)?;
                } else {
                    appendTextLeafPart(&out, alloc, "text/html", _.html_body.?, true)?;
                }
            }

            for (att of _.attachments.items) {
                out.appendSlice(alloc, "--") catch return EmailError.OutOfMemory;
                out.appendSlice(alloc, _.boundary_mix) catch return EmailError.OutOfMemory;
                out.appendSlice(alloc, "\r\n") catch return EmailError.OutOfMemory;

                out.appendSlice(alloc, "Content-Type: ") catch return EmailError.OutOfMemory;
                out.appendSlice(alloc, att.content_type) catch return EmailError.OutOfMemory;
                out.appendSlice(alloc, "; name=\"") catch return EmailError.OutOfMemory;
                appendQuotedParamValue(&out, alloc, att.filename)?;
                out.appendSlice(alloc, "\"\r\n") catch return EmailError.OutOfMemory;

                out.appendSlice(alloc, "Content-Transfer-Encoding: base64\r\n") catch return EmailError.OutOfMemory;
                out.appendSlice(alloc, "Content-Disposition: attachment; filename=\"") catch return EmailError.OutOfMemory;
                appendQuotedParamValue(&out, alloc, att.filename)?;
                out.appendSlice(alloc, "\"\r\n\r\n") catch return EmailError.OutOfMemory;

                appendBase64Wrapped(&out, alloc, att.data)?;
            }

            out.appendSlice(alloc, "--") catch return EmailError.OutOfMemory;
            out.appendSlice(alloc, _.boundary_mix) catch return EmailError.OutOfMemory;
            out.appendSlice(alloc, "--\r\n") catch return EmailError.OutOfMemory;
        } else if (has_text && has_html) {
            appendAlternativeSection(&out, alloc, _.boundary_alt, _.text_body.?, _.html_body.?)?;
        } else if (has_html) {
            appendTextLeafPart(&out, alloc, "text/html", _.html_body.?, false)?;
        } else {
            appendTextLeafPart(&out, alloc, "text/plain", _.text_body ?? "", false)?;
        }

        return out.toOwnedSlice(alloc) catch return EmailError.OutOfMemory;
    }
}

struct HeaderBodySep {
    header_end: usize = 0,
    body_start: usize = 0,
}

fn findHeaderBodySeparator(raw: []const u8): HeaderBodySep? {
    let i: usize = 0;
    while (i < raw.len) : (i += 1) {
        if (i + 3 < raw.len && raw[i] == '\r' && raw[i + 1] == '\n' && raw[i + 2] == '\r' && raw[i + 3] == '\n') {
            return HeaderBodySep{ header_end: i, body_start: i + 4 };
        }
        if (i + 1 < raw.len && raw[i] == '\n' && raw[i + 1] == '\n') {
            return HeaderBodySep{ header_end: i, body_start: i + 2 };
        }
    }
    return null;
}

fn trimLineEnding(line: []const u8): []const u8 {
    if (line.len > 0 && line[line.len - 1] == '\r') {
        return line[0 .. line.len - 1];
    }
    return line;
}

fn parseHeaderBlock(alloc: std.mem.Allocator, header_block: []const u8, parsed: *ParsedEmail): ([]EmailHeader) ! EmailError {
    let unfolded_lines: std.ArrayListUnmanaged([]u8) = .empty;
    defer unfolded_lines.deinit(alloc);

    let line_iter = std.mem.splitScalar(u8, header_block, '\n');
    while (raw_line := line_iter.next()) {
        const line = trimLineEnding(raw_line);
        if (line.len == 0) continue;

        if (line[0] == ' ' || line[0] == '\t') {
            if (unfolded_lines.items.len == 0) {
                return EmailError.InvalidMime;
            }
            const last_idx = unfolded_lines.items.len - 1;
            const prev = unfolded_lines.items[last_idx];
            const cont = std.mem.trim(u8, line, " \t");
            let merged = alloc.alloc(u8, prev.len + 1 + cont.len) catch return EmailError.OutOfMemory;
            @memcpy(merged[0..prev.len], prev);
            merged[prev.len] = ' ';
            @memcpy(merged[prev.len + 1 .. merged.len], cont);
            // Replace previous owned slice in tracked list or track new one
            unfolded_lines.items[last_idx] = merged;
            parsed.owned_slices.append(alloc, merged) catch return EmailError.OutOfMemory;
        } else {
            const owned_line = alloc.dupe(u8, line) catch return EmailError.OutOfMemory;
            parsed.owned_slices.append(alloc, owned_line) catch return EmailError.OutOfMemory;
            unfolded_lines.append(alloc, owned_line) catch return EmailError.OutOfMemory;
        }
    }

    let headers = alloc.alloc(EmailHeader, unfolded_lines.items.len) catch return EmailError.OutOfMemory;
    parsed.owned_headers.append(alloc, headers) catch return EmailError.OutOfMemory;

    let idx: usize = 0;
    while (idx < unfolded_lines.items.len) : (idx += 1) {
        const line = unfolded_lines.items[idx];
        const colon_opt = std.mem.indexOfScalar(u8, line, ':');
        if (colon_opt == null) return EmailError.InvalidMime;
        const colon = colon_opt.?;
        const name = std.mem.trim(u8, line[0..colon], " \t");
        if (name.len == 0) return EmailError.InvalidMime;
        const val = std.mem.trim(u8, line[colon + 1 ..], " \t");
        headers[idx] = EmailHeader{ name: name, value: val };
    }

    return headers;
}

fn findHeaderInSlice(headers: []const EmailHeader, name: string): string? {
    for (h of headers) {
        if (std.ascii.eqlIgnoreCase(h.name, name)) {
            return h.value;
        }
    }
    return null;
}

fn extractMediaType(ct_value: string): string {
    const semi = std.mem.indexOfScalar(u8, ct_value, ';') ?? ct_value.len;
    return std.mem.trim(u8, ct_value[0..semi], " \t");
}

fn extractHeaderParam(header_val: string, param_name: string): string? {
    let iter = std.mem.splitScalar(u8, header_val, ';');
    * = iter.next(); // skip main token
    while (raw_p := iter.next()) {
        const p = std.mem.trim(u8, raw_p, " \t");
        if (p.len == 0) continue;
        if (eq := std.mem.indexOfScalar(u8, p, '=')) {
            const key = std.mem.trim(u8, p[0..eq], " \t");
            if (std.ascii.eqlIgnoreCase(key, param_name)) {
                let val = std.mem.trim(u8, p[eq + 1 ..], " \t");
                if (val.len >= 2 && val[0] == '"' && val[val.len - 1] == '"') {
                    val = val[1 .. val.len - 1];
                }
                return val;
            }
        }
    }
    return null;
}

fn startsWithIgnoreCase(haystack: string, prefix: string): bool {
    if (haystack.len < prefix.len) return false;
    return std.ascii.eqlIgnoreCase(haystack[0..prefix.len], prefix);
}

fn decodeBodyTransferEncoding(alloc: std.mem.Allocator, cte: string, raw_body: []const u8): ([]u8) ! EmailError {
    if (std.ascii.eqlIgnoreCase(cte, "base64")) {
        return decodeBase64CleanAlloc(alloc, raw_body);
    }
    if (std.ascii.eqlIgnoreCase(cte, "quoted-printable")) {
        return QuotedPrintable.decode(alloc, raw_body);
    }
    return alloc.dupe(u8, raw_body) catch return EmailError.OutOfMemory;
}

fn parseMimeEntity(
    alloc: std.mem.Allocator,
    headers: []const EmailHeader,
    body: []const u8,
    parsed: *ParsedEmail,
    parts_list: *std.ArrayListUnmanaged(ParsedPart),
    att_list: *std.ArrayListUnmanaged(EmailAttachment),
    depth: usize,
): void ! EmailError {
    if (depth > 16) return EmailError.InvalidMime;

    const ct_full = findHeaderInSlice(headers, "Content-Type") ?? "text/plain; charset=UTF-8";
    const media_type = extractMediaType(ct_full);
    const cte = std.mem.trim(u8, findHeaderInSlice(headers, "Content-Transfer-Encoding") ?? "7bit", " \t");
    const disp_full = findHeaderInSlice(headers, "Content-Disposition") ?? "";

    if (startsWithIgnoreCase(media_type, "multipart/")) {
        const boundary_opt = extractHeaderParam(ct_full, "boundary");
        if (boundary_opt == null) return EmailError.InvalidMime;
        const boundary = boundary_opt.?;
        if (boundary.len == 0) return EmailError.InvalidMime;

        const delim = std.fmt.allocPrint(alloc, "--{s}", .{ boundary }) catch return EmailError.OutOfMemory;
        defer alloc.free(delim);

        let search_pos: usize = 0;
        while (search_pos < body.len) {
            const first_opt = std.mem.indexOfPos(u8, body, search_pos, delim);
            if (first_opt == null) break;
            const first = first_opt.?;

            // Must be at start of body or preceded by '\n'
            if (first > 0 && body[first - 1] != '\n') {
                search_pos = first + delim.len;
                continue;
            }

            let after_delim = first + delim.len;
            if (after_delim + 1 < body.len && body[after_delim] == '-' && body[after_delim + 1] == '-') {
                break;
            }
            if (after_delim + 1 < body.len && body[after_delim] == '\r' && body[after_delim + 1] == '\n') {
                after_delim += 2;
            } else if (after_delim < body.len && body[after_delim] == '\n') {
                after_delim += 1;
            } else {
                search_pos = after_delim;
                continue;
            }

            // Find next boundary occurrence
            let next_search = after_delim;
            let next_boundary: usize? = null;
            while (next_search < body.len) {
                const cand = std.mem.indexOfPos(u8, body, next_search, delim) ?? break;
                if (cand > 0 && body[cand - 1] == '\n') {
                    next_boundary = cand;
                    break;
                }
                next_search = cand + delim.len;
            }

            const part_end_raw = next_boundary ?? body.len;
            let part_slice = body[after_delim..part_end_raw];
            if (part_slice.len >= 2 && part_slice[part_slice.len - 2] == '\r' && part_slice[part_slice.len - 1] == '\n') {
                part_slice = part_slice[0 .. part_slice.len - 2];
            } else if (part_slice.len >= 1 && part_slice[part_slice.len - 1] == '\n') {
                part_slice = part_slice[0 .. part_slice.len - 1];
            }

            if (sep := findHeaderBodySeparator(part_slice)) {
                const part_headers = parseHeaderBlock(alloc, part_slice[0..sep.header_end], parsed)?;
                const part_body = part_slice[sep.body_start..];
                parseMimeEntity(alloc, part_headers, part_body, parsed, parts_list, att_list, depth + 1)?;
            } else {
                const empty_headers: []const EmailHeader = [];
                parseMimeEntity(alloc, empty_headers, part_slice, parsed, parts_list, att_list, depth + 1)?;
            }

            if (next_boundary == null) break;
            search_pos = next_boundary.?;
        }
    } else {
        const decoded_body = decodeBodyTransferEncoding(alloc, cte, body)?;
        parsed.owned_slices.append(alloc, decoded_body) catch return EmailError.OutOfMemory;

        let fname: string? = extractHeaderParam(disp_full, "filename");
        if (fname == null) {
            fname = extractHeaderParam(ct_full, "name");
        }
        const is_attachment = startsWithIgnoreCase(std.mem.trim(u8, disp_full, " \t"), "attachment") || (fname != null && !startsWithIgnoreCase(media_type, "text/"));

        parts_list.append(alloc, ParsedPart{
            content_type: media_type,
            content_transfer_encoding: cte,
            content_disposition: disp_full,
            filename: fname,
            headers: headers,
            body: decoded_body,
        }) catch return EmailError.OutOfMemory;

        if (is_attachment) {
            att_list.append(alloc, EmailAttachment{
                filename: fname ?? "attachment.bin",
                content_type: media_type,
                data: decoded_body,
            }) catch return EmailError.OutOfMemory;
        } else if (std.ascii.eqlIgnoreCase(media_type, "text/plain")) {
            if (parsed.text_body == null) {
                parsed.text_body = decoded_body;
            }
        } else if (std.ascii.eqlIgnoreCase(media_type, "text/html")) {
            if (parsed.html_body == null) {
                parsed.html_body = decoded_body;
            }
        }
    }
}

/// RFC 5322 & RFC 2045 MIME parser with header unfolding, RFC 2047 UTF-8 decoding, and multipart extraction.
pub static struct MimeParser {
    /// Parses a raw RFC 5322 / MIME byte stream into a `ParsedEmail`. Caller must call `.deinit()` on the result.
    pub static fn parse(alloc: std.mem.Allocator, raw_rfc5322: []const u8): ParsedEmail ! EmailError {
        let parsed = ParsedEmail{
            alloc: alloc,
        };
        errdefer parsed.deinit();

        let header_slice: []const u8 = raw_rfc5322;
        let body_slice: []const u8 = "";
        if (sep := findHeaderBodySeparator(raw_rfc5322)) {
            header_slice = raw_rfc5322[0..sep.header_end];
            body_slice = raw_rfc5322[sep.body_start..];
        }

        const headers = parseHeaderBlock(alloc, header_slice, &parsed)?;
        parsed.headers = headers;
        parsed.raw_body = body_slice;

        if (v := findHeaderInSlice(headers, "From")) parsed.from = v;
        if (v := findHeaderInSlice(headers, "To")) parsed.to = v;
        if (v := findHeaderInSlice(headers, "Cc")) parsed.cc = v;
        if (v := findHeaderInSlice(headers, "Content-Type")) parsed.content_type = v;

        if (raw_subj := findHeaderInSlice(headers, "Subject")) {
            parsed.raw_subject = raw_subj;
            const decoded_subj = decodeRfc2047Header(alloc, raw_subj)?;
            parsed.owned_slices.append(alloc, decoded_subj) catch return EmailError.OutOfMemory;
            parsed.subject = decoded_subj;
        }

        let parts_list: std.ArrayListUnmanaged(ParsedPart) = .empty;
        errdefer parts_list.deinit(alloc);

        let att_list: std.ArrayListUnmanaged(EmailAttachment) = .empty;
        errdefer att_list.deinit(alloc);

        parseMimeEntity(alloc, headers, body_slice, &parsed, &parts_list, &att_list, 0)?;

        parsed.parts = parts_list.toOwnedSlice(alloc) catch return EmailError.OutOfMemory;
        parsed.attachments = att_list.toOwnedSlice(alloc) catch return EmailError.OutOfMemory;

        return parsed;
    }
}
