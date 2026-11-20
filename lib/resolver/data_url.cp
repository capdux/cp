// Source: /data/dev/bun/src/resolver/data_url.zig
// ============================================================================
// lib/resolver/data_url.cp — RFC 2397 Data URL Parser & Decoder (cap:resolver)
// ============================================================================

const std = @import("std");
import { Base64 } from "cap:encoding";
import { PercentEncoding } from "cap:url";

pub err DataUrlError {
    InvalidDataURL,
    Base64DecodeError,
    OutOfMemory,
}

fn startsWithAsciiIgnoreCase(haystack: []const u8, prefix: []const u8): bool {
    if (haystack.len < prefix.len) return false;
    return std.ascii.eqlIgnoreCase(haystack[0..prefix.len], prefix);
}

fn endsWithAsciiIgnoreCase(haystack: []const u8, suffix: []const u8): bool {
    if (haystack.len < suffix.len) return false;
    return std.ascii.eqlIgnoreCase(haystack[haystack.len - suffix.len ..], suffix);
}

fn trimAsciiWhitespace(s: []const u8): []const u8 {
    return std.mem.trim(u8, s, " \t\r\n");
}

/// RFC 2397 `data:[<mediatype>][;charset=...][;base64],<data>` Parser & Decoder
pub struct DataURL {
    raw_url: []const u8 = "",
    mediatype: []const u8 = "",
    mime_type: []const u8 = "text/plain",
    charset: []const u8 = "",
    is_base64: bool = false,
    raw_data: []const u8 = "",

    /// Parses a `data:` URL; returns `null` if `url` does not begin with `data:`.
    pub static fn parse(url: []const u8): DataURL?! {
        if (!startsWithAsciiIgnoreCase(url, "data:")) {
            return null;
        }
        return parseWithoutCheck(url)?;
    }

    /// Parses a `data:` URL assuming the caller already verified or expects `data:` prefix.
    pub static fn parseWithoutCheck(url: []const u8): DataURL! {
        if (!startsWithAsciiIgnoreCase(url, "data:")) {
            return error.InvalidDataURL;
        }
        const comma_opt = std.mem.indexOfScalar(u8, url, ',');
        if (comma_opt == null) {
            return error.InvalidDataURL;
        }
        const comma = comma_opt.?;

        let meta = url[5..comma];
        let is_base64 = false;
        if (endsWithAsciiIgnoreCase(meta, ";base64")) {
            meta = meta[0 .. meta.len - 7];
            is_base64 = true;
        }

        let mime_type: []const u8 = "text/plain";
        let charset: []const u8 = "";

        if (meta.len > 0) {
            // Split parameters by ';'
            const first_semi = std.mem.indexOfScalar(u8, meta, ';');
            if (semi_idx := first_semi) {
                const head = trimAsciiWhitespace(meta[0..semi_idx]);
                if (head.len > 0) {
                    mime_type = head;
                }
                let rest = meta[semi_idx + 1 ..];
                while (rest.len > 0) {
                    const next_semi = std.mem.indexOfScalar(u8, rest, ';');
                    let param_raw = rest;
                    if (idx := next_semi) {
                        param_raw = rest[0..idx];
                    }
                    const param = trimAsciiWhitespace(param_raw);
                    if (startsWithAsciiIgnoreCase(param, "charset=")) {
                        let cs = trimAsciiWhitespace(param[8..]);
                        if (cs.len >= 2 && ((cs[0] == '"' && cs[cs.len - 1] == '"') || (cs[0] == '\'' && cs[cs.len - 1] == '\''))) {
                            cs = cs[1 .. cs.len - 1];
                        }
                        charset = cs;
                    } else if (std.ascii.eqlIgnoreCase(param, "base64")) {
                        is_base64 = true;
                    }
                    if (idx := next_semi) {
                        rest = rest[idx + 1 ..];
                    } else {
                        break;
                    }
                }
            } else {
                const trimmed = trimAsciiWhitespace(meta);
                if (trimmed.len > 0) {
                    mime_type = trimmed;
                }
            }
        }

        return DataURL{
            raw_url: url,
            mediatype: meta,
            mime_type: mime_type,
            charset: charset,
            is_base64: is_base64,
            raw_data: url[comma + 1 ..],
        };
    }

    /// Returns the MIME type (defaults to `"text/plain"` per RFC 2397).
    pub fn decodeMimeType(): []const u8 {
        return _.mime_type;
    }

    /// Returns explicit charset or `"US-ASCII"` default per RFC 2397.
    pub fn effectiveCharset(): []const u8 {
        if (_.charset.len > 0) return _.charset;
        return "US-ASCII";
    }

    /// Decodes the payload into an owned slice using Base64 (via `cap:encoding`)
    /// or RFC 3986 Percent-Decoding (via `cap:url`).
    pub fn decodeAlloc(allocator: std.mem.Allocator): ([]u8)! {
        const percent_buf = allocator.alloc(u8, _.raw_data.len)?;
        let freed_percent = false;
        errdefer {
            if (!freed_percent) allocator.free(percent_buf);
        }
        const pct_len = PercentEncoding.decodeToBuf(_.raw_data, false, percent_buf);
        const percent_decoded = percent_buf[0..pct_len];

        if (_.is_base64) {
            const decoded = Base64.decodeAlloc(allocator, percent_decoded) catch {
                allocator.free(percent_buf);
                freed_percent = true;
                return error.Base64DecodeError;
            };
            allocator.free(percent_buf);
            freed_percent = true;
            return decoded;
        }

        const out = allocator.realloc(percent_buf, pct_len) catch {
            freed_percent = true;
            return percent_buf[0..pct_len];
        };
        freed_percent = true;
        return out;
    }

    /// Encodes `mime_type` and raw `payload` bytes into a `data:<mime>;base64,<data>` URL.
    pub static fn encodeBase64Alloc(allocator: std.mem.Allocator, mime_type: []const u8, payload: []const u8): ([]u8)! {
        const b64 = Base64.encodeAlloc(allocator, payload)?;
        defer allocator.free(b64);
        return std.fmt.allocPrint(allocator, "data:{s};base64,{s}", .{ mime_type, b64 });
    }
}
