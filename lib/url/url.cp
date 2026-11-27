// Reference-hardened against servo/rust-url (MIT/Apache-2.0)
// Source: Absorbed from /data/dev/bun/src/url/url.cp | Synced: 2026-11-22T12:49:17Z
// ============================================================================
// lib/url/url.cp — WHATWG URL Parser, Strict Security Validators & Path Normalizer
// ============================================================================
import { Simd } from "cap:simd";
import { URLSearchParams, QueryParam } from "./searchParams";
import { PercentEncoding, isValidPercentEncoding } from "./percent";
import {
    findSubstr,
    urlIndexOfChar,
    urlLastIndexOfChar,
    isIPHostname,
    isLocalhostName,
    parsePortNumber,
} from "./url_";
const std = @import("std");

/// Hard upper bound for URL length (64 KiB) to prevent memory/CPU exhaustion.
pub const URL_MAX_LEN: usize = 65536;

/// Maximum hostname length per DNS / WHATWG limits.
pub const URL_MAX_HOST_LEN: usize = 255;

/// Maximum path segments tracked during stack-only dot-segment normalization.
const URL_MAX_PATH_SEGMENTS: usize = 256;

fn asciiLower(c: u8): u8 {
    if (c >= (65 as u8) and c <= (90 as u8)) {
        return c + (32 as u8);
    }
    return c;
}

fn asciiEqlCI(a: string, b: string): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (asciiLower(a[i]) != asciiLower(b[i])) return false;
        i += 1;
    }
    return true;
}

fn stripTrailingColon(scheme: string): string {
    if (scheme.len > 0 and scheme[scheme.len - 1] == (58 as u8)) {
        return scheme[0 .. scheme.len - 1];
    }
    return scheme;
}

fn isAsciiAlpha(c: u8): bool {
    return (c >= (65 as u8) and c <= (90 as u8)) or (c >= (97 as u8) and c <= (122 as u8));
}

fn isAsciiDigit(c: u8): bool {
    return c >= (48 as u8) and c <= (57 as u8);
}

fn isSchemeChar(c: u8): bool {
    return isAsciiAlpha(c) or isAsciiDigit(c) or c == (43 as u8) or c == (45 as u8) or c == (46 as u8);
}

/// WHATWG forbidden host code points (RFC / servo/rust-url `host.rs`).
fn isForbiddenHostByte(c: u8): bool {
    if (c <= (32 as u8) or c == (127 as u8)) return true; // C0 controls, space, DEL
    if (c == (35 as u8) or c == (47 as u8) or c == (58 as u8) or c == (60 as u8) or
        c == (62 as u8) or c == (63 as u8) or c == (64 as u8) or c == (91 as u8) or
        c == (92 as u8) or c == (93 as u8) or c == (94 as u8) or c == (124 as u8))
    {
        return true; // '#', '/', ':', '<', '>', '?', '@', '[', '\\', ']', '^', '|'
    }
    return false;
}

/// 16-lane SIMD scan for any primary URL delimiter (`'/'`, `'?'`, `'#'`, `':'`).
pub fn simdFindUrlDelimiter(input: []const u8): usize? {
    return Simd.indexOfAny4(input, 47 as u8, 63 as u8, 35 as u8, 58 as u8);
}

/// Scalar reference for `simdFindUrlDelimiter` (for differential verification).
pub fn simdFindUrlDelimiterScalar(input: []const u8): usize? {
    let i: usize = 0;
    while (i < input.len) {
        const c = input[i];
        if (c == (47 as u8) or c == (63 as u8) or c == (35 as u8) or c == (58 as u8)) {
            return i;
        }
        i += 1;
    }
    return null;
}

/// SIMD-accelerated path separator scan (`'/'`, plus `'\\'` for WHATWG special schemes).
pub fn simdFindPathDelimiter(input: []const u8, is_special: bool): usize? {
    if (is_special) {
        return Simd.indexOfAny2(input, 47 as u8, 92 as u8);
    }
    return Simd.indexOfByte(input, 47 as u8);
}

/// Returns `true` if `scheme` (with or without trailing `':'`) is a WHATWG special scheme
/// (`http`, `https`, `ws`, `wss`, `ftp`, `file`).
pub fn isSpecialScheme(scheme: string): bool {
    const s = stripTrailingColon(scheme);
    return asciiEqlCI(s, "http") or
        asciiEqlCI(s, "https") or
        asciiEqlCI(s, "ws") or
        asciiEqlCI(s, "wss") or
        asciiEqlCI(s, "ftp") or
        asciiEqlCI(s, "file");
}

/// Returns the WHATWG default port for `scheme` (`http:80`, `https:443`, `ws:80`, `wss:443`, `ftp:21`),
/// or `null` for schemes without a default port.
pub fn getDefaultPortForScheme(scheme: string): u16? {
    const s = stripTrailingColon(scheme);
    if (asciiEqlCI(s, "http") or asciiEqlCI(s, "ws")) return 80 as u16;
    if (asciiEqlCI(s, "https") or asciiEqlCI(s, "wss")) return 443 as u16;
    if (asciiEqlCI(s, "ftp")) return 21 as u16;
    return null;
}

/// Normalizes explicit default ports to `""` (e.g., `("https", "443")` -> `""`),
/// preserving non-default ports as-is.
pub fn normalizeDefaultPort(scheme: string, port_str: string): string {
    if (port_str.len == 0) return "";
    if (p := parsePortNumber(port_str)) {
        if (def_p := getDefaultPortForScheme(scheme)) {
            if (p == def_p) return "";
        }
    }
    return port_str;
}

/// Strict port validator: empty string is valid (omitted port); otherwise 1..5 ASCII digits in `0..65535`.
pub fn isValidPortStrict(port_str: string): bool {
    if (port_str.len == 0) return true;
    if (port_str.len > 5) return false;
    return parsePortNumber(port_str) != null;
}

/// Strict canonical dotted-quad IPv4 validator (`0.0.0.0` .. `255.255.255.255`).
/// Rejects leading zeros (octal ambiguity), empty octets, trailing dots, and out-of-range values.
pub fn isValidIPv4Strict(host: string): bool {
    if (host.len < 7 or host.len > 15) return false;
    let octets: usize = 0;
    let i: usize = 0;
    while (i < host.len) {
        if (octets >= 4) return false;
        const start = i;
        let val: u32 = 0;
        while (i < host.len and host[i] != (46 as u8)) {
            const c = host[i];
            if (!isAsciiDigit(c)) return false;
            val = val * 10 + ((c - (48 as u8)) as u32);
            if (val > 255) return false;
            i += 1;
        }
        const digit_len = i - start;
        if (digit_len == 0 or digit_len > 3) return false;
        if (digit_len > 1 and host[start] == (48 as u8)) return false; // reject leading zero
        octets += 1;
        if (i < host.len) {
            // must be '.' and not trailing
            if (host[i] != (46 as u8) or i + 1 == host.len) return false;
            i += 1;
        }
    }
    return octets == 4;
}

/// Strict WHATWG / RFC 4291 IPv6 literal validator (ported from `servo/rust-url/url/src/host.rs::parse_ipv6addr`).
/// Requires enclosing `[` and `]` brackets; supports `::` compression and embedded IPv4 tail.
pub fn isValidIPv6Strict(host: string): bool {
    if (host.len < 4 or host[0] != (91 as u8) or host[host.len - 1] != (93 as u8)) {
        return false;
    }
    const inner = host[1 .. host.len - 1];
    const len = inner.len;
    if (len < 2 or len > 45) return false;

    let piece_pointer: usize = 0;
    let has_compress = false;
    let is_ipv4 = false;
    let i: usize = 0;

    if (inner[0] == (58 as u8)) {
        if (inner[1] != (58 as u8)) return false;
        i = 2;
        piece_pointer = 1;
        has_compress = true;
    }

    while (i < len) {
        if (piece_pointer == 8) return false;
        if (inner[i] == (58 as u8)) {
            if (has_compress) return false;
            i += 1;
            piece_pointer += 1;
            has_compress = true;
            continue;
        }
        const start = i;
        const max_end = if (start + 4 < len) (start + 4) else len;
        while (i < max_end) {
            if (PercentEncoding.hexValue(inner[i]) == 0xFF) break;
            i += 1;
        }
        if (i < len) {
            const sep = inner[i];
            if (sep == (46 as u8)) { // '.' -> embedded IPv4
                if (i == start or piece_pointer > 6) return false;
                i = start;
                is_ipv4 = true;
            } else if (sep == (58 as u8)) { // ':'
                if (i == start) return false;
                i += 1;
                if (i == len) return false; // trailing single ':'
            } else {
                return false;
            }
        }
        if (is_ipv4) break;
        if (i == start) return false;
        piece_pointer += 1;
    }

    if (is_ipv4) {
        if (piece_pointer > 6) return false;
        let numbers_seen: usize = 0;
        while (i < len) {
            if (numbers_seen > 0) {
                if (numbers_seen < 4 and inner[i] == (46 as u8)) {
                    i += 1;
                } else {
                    return false;
                }
            }
            const d_start = i;
            let val: u32 = 0;
            while (i < len and isAsciiDigit(inner[i])) {
                if (i > d_start and inner[d_start] == (48 as u8)) return false; // no leading zero
                val = val * 10 + ((inner[i] - (48 as u8)) as u32);
                if (val > 255) return false;
                i += 1;
            }
            if (i == d_start) return false;
            numbers_seen += 1;
            if (numbers_seen == 2 or numbers_seen == 4) {
                piece_pointer += 1;
            }
        }
        if (numbers_seen != 4) return false;
    }

    if (i < len) return false;
    if (has_compress) {
        return piece_pointer <= 7;
    }
    return piece_pointer == 8;
}

/// Strict hostname validator against SSRF / host confusion:
/// - Rejects empty or > 255 byte hosts;
/// - Enforces strict IPv6 bracket & syntax rules for `[...]` hosts;
/// - Rejects control chars, whitespace, and WHATWG forbidden host code points (`# / : < > ? @ [ \ ] ^ |`);
/// - Validates percent-encoding and rejects decoded forbidden host bytes;
/// - Enforces strict IPv4 validation for all-numeric dotted hosts.
pub fn isValidHostStrict(host: string): bool {
    if (host.len == 0 or host.len > URL_MAX_HOST_LEN) return false;
    if (host[0] == (91 as u8)) {
        return isValidIPv6Strict(host);
    }

    let all_digits_or_dots = true;
    let label_len: usize = 0;
    let i: usize = 0;
    while (i < host.len) {
        const c = host[i];
        if (isForbiddenHostByte(c)) return false;
        if (c == (37 as u8)) { // '%'
            if (i + 2 >= host.len) return false;
            const hi = PercentEncoding.hexValue(host[i + 1]);
            const lo = PercentEncoding.hexValue(host[i + 2]);
            if (hi == 0xFF or lo == 0xFF) return false;
            const decoded: u8 = (hi << 4) | lo;
            if (isForbiddenHostByte(decoded)) return false;
            all_digits_or_dots = false;
            label_len += 1;
            i += 3;
            continue;
        }
        if (c == (46 as u8)) { // '.'
            if (label_len == 0) return false; // leading '.' or empty label ".."
            if (label_len > 63) return false;
            label_len = 0;
        } else {
            if (!isAsciiDigit(c)) {
                all_digits_or_dots = false;
            }
            label_len += 1;
            if (label_len > 63) return false;
        }
        i += 1;
    }

    if (all_digits_or_dots) {
        return isValidIPv4Strict(host);
    }
    return true;
}

fn isEncodedDot(seg: string, pos: usize): bool {
    return pos + 2 < seg.len and
        seg[pos] == (37 as u8) and
        seg[pos + 1] == (50 as u8) and
        (seg[pos + 2] == (101 as u8) or seg[pos + 2] == (69 as u8)); // %2e or %2E
}

fn isSingleDotSegment(seg: string): bool {
    if (seg.len == 1 and seg[0] == (46 as u8)) return true;
    if (seg.len == 3 and isEncodedDot(seg, 0)) return true;
    return false;
}

fn isDoubleDotSegment(seg: string): bool {
    if (seg.len == 2 and seg[0] == (46 as u8) and seg[1] == (46 as u8)) return true;
    if (seg.len == 4) {
        if (seg[0] == (46 as u8) and isEncodedDot(seg, 1)) return true;
        if (isEncodedDot(seg, 0) and seg[3] == (46 as u8)) return true;
    }
    if (seg.len == 6 and isEncodedDot(seg, 0) and isEncodedDot(seg, 3)) return true;
    return false;
}

/// WHATWG URL path normalizer (zero-allocation):
/// - Normalizes `\` to `/` when `is_special` is `true` (http/https/ws/wss/ftp/file);
/// - Collapses `.`, `%2e`, `%2E`, `..`, `%2e.`, `.%2e`, `%2e%2e` dot-segments against traversal;
/// - Enforces `URL_MAX_LEN` and output buffer bounds (returns `null` on overflow).
fn normalizeUrlPathImpl(path: string, is_special: bool, out: []u8): ([]const u8)? {
    if (out.len == 0 or path.len > URL_MAX_LEN) return null;
    if (path.len == 0) {
        out[0] = 47 as u8;
        return out[0..1];
    }

    let seg_starts: [256]usize = undefined;
    let seg_lens: [256]usize = undefined;
    let seg_count: usize = 0;

    const last_byte = path[path.len - 1];
    let trailing_slash = last_byte == (47 as u8) or (is_special and last_byte == (92 as u8));
    let last_was_dot = false;

    let i: usize = 0;
    while (i <= path.len) {
        const start = i;
        if (i < path.len) {
            const sep_rel = simdFindPathDelimiter(path[i..], is_special);
            if (off := sep_rel) {
                i = start + off;
            } else {
                i = path.len;
            }
        }
        const seg = path[start..i];
        if (seg.len == 0) {
            last_was_dot = false;
        } else if (isSingleDotSegment(seg)) {
            last_was_dot = true;
        } else if (isDoubleDotSegment(seg)) {
            if (seg_count > 0) {
                seg_count -= 1;
            }
            last_was_dot = true;
        } else {
            if (seg_count >= URL_MAX_PATH_SEGMENTS) return null;
            seg_starts[seg_count] = start;
            seg_lens[seg_count] = seg.len;
            seg_count += 1;
            last_was_dot = false;
        }
        if (i < path.len) {
            i += 1; // skip '/' or '\\'
        } else {
            break;
        }
    }

    if (last_was_dot) {
        trailing_slash = true;
    }

    if (seg_count == 0) {
        out[0] = 47 as u8;
        return out[0..1];
    }

    let pos: usize = 0;
    let s: usize = 0;
    while (s < seg_count) {
        if (pos + 1 > out.len) return null;
        out[pos] = 47 as u8;
        pos += 1;
        const slen = seg_lens[s];
        const sstart = seg_starts[s];
        if (pos + slen > out.len) return null;
        @memcpy(out[pos .. pos + slen], path[sstart .. sstart + slen]);
        pos += slen;
        s += 1;
    }
    if (trailing_slash and out[pos - 1] != (47 as u8)) {
        if (pos + 1 > out.len) return null;
        out[pos] = 47 as u8;
        pos += 1;
    }
    return out[0..pos];
}

/// WHATWG URL path normalizer (zero-allocation):
/// - Normalizes `\` to `/` when `is_special` is `true` (http/https/ws/wss/ftp/file);
/// - Collapses `.`, `%2e`, `%2E`, `..`, `%2e.`, `.%2e`, `%2e%2e` dot-segments against traversal;
/// - Enforces `URL_MAX_LEN` and output buffer bounds (returns `null` on overflow).
pub fn normalizeUrlPath(path: string, is_special: bool, out: []u8): ([]const u8)? {
    return normalizeUrlPathImpl(path, is_special, out);
}

/// Zero-allocation strict WHATWG URL security validator:
/// - Rejects empty input, length > `URL_MAX_LEN`, control chars (`0x00..0x1F`, `0x7F`), and raw spaces;
/// - Enforces RFC/WHATWG scheme syntax (`ALPHA *( ALPHA / DIGIT / "+" / "-" / "." ) ":"`);
/// - Enforces strict IPv4 / IPv6 `[...]` / domain host validation and `0..65535` port bounds;
/// - Rejects malformed or truncated `%XX` sequences across the URL.
pub fn isValidUrlStrict(input: string): bool {
    if (input.len == 0 or input.len > URL_MAX_LEN) return false;

    // 1. Reject ASCII control chars (<= 0x20 or 0x7F); use SIMD ASCII check when possible.
    if (Simd.isAscii(input)) {
        let k: usize = 0;
        while (k < input.len) {
            const c = input[k];
            if (c <= (32 as u8) or c == (127 as u8)) return false;
            k += 1;
        }
    } else {
        let k: usize = 0;
        while (k < input.len) {
            const c = input[k];
            if (c <= (32 as u8) or c == (127 as u8)) return false;
            k += 1;
        }
    }

    // 2. Scheme validation via SIMD delimiter search
    const first_delim = simdFindUrlDelimiter(input);
    const colon_idx = first_delim orelse return false;
    if (colon_idx == 0 or input[colon_idx] != (58 as u8)) return false;
    if (!isAsciiAlpha(input[0])) return false;
    let si: usize = 1;
    while (si < colon_idx) {
        if (!isSchemeChar(input[si])) return false;
        si += 1;
    }

    const scheme = input[0..colon_idx];
    const special = isSpecialScheme(scheme);
    const is_file = asciiEqlCI(scheme, "file");
    let rem = input[colon_idx + 1 ..];

    // 3. Validate all percent-escapes in the remainder
    if (!isValidPercentEncoding(rem)) return false;

    // 4. Strip fragment '#' and query '?'
    if (h_idx := Simd.indexOfByte(rem, 35 as u8)) {
        rem = rem[0..h_idx];
    }
    if (q_idx := Simd.indexOfByte(rem, 63 as u8)) {
        rem = rem[0..q_idx];
    }

    // 5. Authority check
    const has_slashes = rem.len >= 2 and
        (rem[0] == (47 as u8) or (special and rem[0] == (92 as u8))) and
        (rem[1] == (47 as u8) or (special and rem[1] == (92 as u8)));

    if (special and !has_slashes) return false;
    if (!has_slashes) {
        // Non-special opaque URL (e.g. "mailto:a@b", "data:text/plain,hi", "urn:isbn:0451450523")
        return rem.len > 0;
    }

    rem = rem[2..];
    let authority = rem;
    if (p_idx := simdFindPathDelimiter(rem, special)) {
        authority = rem[0..p_idx];
    }

    if (is_file) {
        if (authority.len == 0) return true;
        return isValidHostStrict(authority);
    }

    if (authority.len == 0) return false;

    // 6. Userinfo: split at the LAST '@' to prevent host-confusion spoofing
    let hostport = authority;
    if (at_idx := urlLastIndexOfChar(authority, 64 as u8)) {
        hostport = authority[at_idx + 1 ..];
        if (hostport.len == 0) return false;
    }

    // 7. Host & Port validation
    if (hostport[0] == (91 as u8)) { // '[' IPv6
        const close_idx = Simd.indexOfByte(hostport, 93 as u8) orelse return false;
        const ipv6_host = hostport[0 .. close_idx + 1];
        if (!isValidIPv6Strict(ipv6_host)) return false;
        const after = hostport[close_idx + 1 ..];
        if (after.len == 0) return true;
        if (after[0] != (58 as u8)) return false;
        return isValidPortStrict(after[1..]);
    } else {
        if (port_colon := urlLastIndexOfChar(hostport, 58 as u8)) {
            const host_part = hostport[0..port_colon];
            const port_part = hostport[port_colon + 1 ..];
            // Reject multiple colons outside IPv6 brackets
            if (Simd.indexOfByte(host_part, 58 as u8) != null) return false;
            if (!isValidHostStrict(host_part)) return false;
            return isValidPortStrict(port_part);
        }
        return isValidHostStrict(hostport);
    }
}

pub struct URL {
    pub href: string = "",
    pub protocol: string = "",
    pub scheme: string = "",
    pub username: string = "",
    pub password: string = "",
    pub host: string = "",
    pub hostname: string = "",
    pub port: string = "",
    pub pathname: string = "/",
    pub path: string = "/",
    pub search: string = "",
    pub hash: string = "",
    pub origin: string = "",
    pub searchParams: URLSearchParams = URLSearchParams{},
    pub allocated_origin: string? = null,
    pub allocator: std.mem.Allocator? = null,

    pub fn isFile(): bool {
        return asciiEqlCI(stripTrailingColon(_.protocol), "file");
    }

    pub fn isBlob(): bool {
        return asciiEqlCI(stripTrailingColon(_.protocol), "blob");
    }

    pub fn isHTTPS(): bool {
        return asciiEqlCI(stripTrailingColon(_.protocol), "https");
    }

    pub fn isHTTP(): bool {
        return asciiEqlCI(stripTrailingColon(_.protocol), "http");
    }

    pub fn isWS(): bool {
        return asciiEqlCI(stripTrailingColon(_.protocol), "ws");
    }

    pub fn isWSS(): bool {
        return asciiEqlCI(stripTrailingColon(_.protocol), "wss");
    }

    pub fn isFTP(): bool {
        return asciiEqlCI(stripTrailingColon(_.protocol), "ftp");
    }

    pub fn isSpecial(): bool {
        return isSpecialScheme(_.protocol);
    }

    pub fn isS3(): bool {
        return asciiEqlCI(stripTrailingColon(_.protocol), "s3");
    }

    pub fn isUnix(): bool {
        return _.protocol.len >= 4 and _.protocol[0] == (117 as u8) and _.protocol[1] == (110 as u8) and
            _.protocol[2] == (105 as u8) and _.protocol[3] == (120 as u8); // "unix"
    }

    pub fn hasHTTPLikeProtocol(): bool {
        return _.isHTTP() or _.isHTTPS();
    }

    pub fn isLocalhost(): bool {
        return isLocalhostName(_.hostname);
    }

    pub fn isIPAddress(): bool {
        return isIPHostname(_.hostname);
    }

    pub fn hasValidPort(): bool {
        if (p := _.getPort()) {
            return p > 0;
        }
        return false;
    }

    pub fn isEmpty(): bool {
        return _.href.len == 0;
    }

    pub fn isAbsolute(): bool {
        return _.hostname.len > 0 and _.pathname.len > 0;
    }

    pub fn displayProtocol(): string {
        if (_.protocol.len > 0) {
            if (_.protocol[_.protocol.len - 1] == (58 as u8)) { // ':'
                return _.protocol[0 .. _.protocol.len - 1];
            }
            return _.protocol;
        }
        if (p := _.getPort()) {
            if (p == 443) return "https";
        }
        return "http";
    }

    pub fn getScheme(): string {
        if (_.scheme.len > 0) return _.scheme;
        return _.displayProtocol();
    }

    pub fn displayHostname(): string {
        if (_.hostname.len > 0) return _.hostname;
        return "localhost";
    }

    pub fn displayHost(): string {
        if (_.host.len > 0) return _.host;
        return _.displayHostname();
    }

    pub fn getPort(): u16? {
        return parsePortNumber(_.port);
    }

    pub fn getPortAuto(): u16 {
        if (p := _.getPort()) {
            return p;
        }
        return _.getDefaultPort();
    }

    pub fn getDefaultPort(): u16 {
        if (def_p := getDefaultPortForScheme(_.protocol)) {
            return def_p;
        }
        if (_.protocol.len == 0) return 80 as u16;
        return 0 as u16;
    }

    /// Returns `host + pathname` without trailing slash, protocol, search, or hash.
    pub fn hostWithPath(): string {
        if (_.host.len == 0) return "";
        if (_.pathname.len <= 1) return _.host;
        if (host_idx := findSubstr(_.href, _.host)) {
            let end_idx = host_idx + _.host.len + _.pathname.len;
            if (end_idx <= _.href.len) {
                if (_.href[end_idx - 1] == (47 as u8)) { // '/'
                    end_idx -= 1;
                }
                return _.href[host_idx..end_idx];
            }
        }
        return _.host;
    }

    pub fn s3Path(): string {
        let h = _.href;
        if (_.protocol.len > 0 and h.len > _.protocol.len + 2) {
            h = h[_.protocol.len + 2 ..];
        }
        let end = h.len;
        if (_.search.len > 0) {
            end -= _.search.len;
        }
        if (_.hash.len > 0) {
            end -= _.hash.len;
        }
        return h[0..end];
    }

    pub static fn joinNormalize(
        out: []u8,
        prefix: string,
        dirname: string,
        basename: string,
        extname: string
    ): string {
        let pos: usize = 0;
        if (prefix.len > 0 and pos + prefix.len <= out.len) {
            if (prefix[0] != (47 as u8)) {
                out[pos] = 47 as u8;
                pos += 1;
            }
            @memcpy(out[pos .. pos + prefix.len], prefix);
            pos += prefix.len;
        }
        if (dirname.len > 0) {
            if (pos > 0 and out[pos - 1] != (47 as u8)) {
                out[pos] = 47 as u8;
                pos += 1;
            }
            let d = dirname;
            if (d.len > 0 and d[0] == (47 as u8)) d = d[1..];
            if (pos + d.len <= out.len) {
                @memcpy(out[pos .. pos + d.len], d);
                pos += d.len;
            }
        }
        if (basename.len > 0) {
            if (pos > 0 and out[pos - 1] != (47 as u8)) {
                out[pos] = 47 as u8;
                pos += 1;
            }
            let b = basename;
            if (b.len > 0 and b[0] == (47 as u8)) b = b[1..];
            if (pos + b.len <= out.len) {
                @memcpy(out[pos .. pos + b.len], b);
                pos += b.len;
            }
        }
        if (extname.len > 0 and pos + extname.len <= out.len) {
            @memcpy(out[pos .. pos + extname.len], extname);
            pos += extname.len;
        }
        if (pos == 0) {
            out[0] = 47 as u8;
            return out[0..1];
        }
        return out[0..pos];
    }

    /// Zero-allocation strict validation wrapper.
    pub static fn isValidStrict(input: string): bool {
        return isValidUrlStrict(input);
    }

    /// Zero-allocation WHATWG path normalizer wrapper.
    pub static fn normalizeUrlPath(path: string, is_special: bool, out: []u8): ([]const u8)? {
        return normalizeUrlPathImpl(path, is_special, out);
    }

    /// Zero-allocation slice parser (equivalent to `URL.parse(input, null)`).
    pub static fn parseSlice(input: string): URL {
        return URL.parse(input, null) catch URL{};
    }

    /// Full WHATWG URL parsing engine (SIMD-accelerated delimiter scanning + `URL_MAX_LEN` guard).
    pub static fn parse(input: string, alloc: std.mem.Allocator?): !URL {
        if (input.len > URL_MAX_LEN) return err.UrlTooLong;

        let url = URL{
            .href = input,
            .searchParams = URLSearchParams.init(alloc),
            .allocator = alloc,
        };

        if (input.len == 0) return url;

        let rem = input;

        // 1. Protocol scheme via 16-lane SIMD delimiter scan ('/', '?', '#', ':')
        const delim_opt = simdFindUrlDelimiter(rem);
        if (c_idx := delim_opt) {
            if (rem[c_idx] == (58 as u8) and c_idx > 0) {
                let is_scheme = true;
                let k: usize = 0;
                while (k < c_idx) {
                    if (rem[k] == (64 as u8)) { // '@'
                        is_scheme = false;
                        break;
                    }
                    k += 1;
                }
                if (is_scheme) {
                    url.protocol = input[0 .. c_idx + 1];
                    url.scheme = input[0..c_idx];
                    const after_colon = rem[c_idx + 1 ..];
                    const special = isSpecialScheme(url.scheme);
                    if (after_colon.len >= 2 and
                        (after_colon[0] == (47 as u8) or (special and after_colon[0] == (92 as u8))) and
                        (after_colon[1] == (47 as u8) or (special and after_colon[1] == (92 as u8))))
                    {
                        rem = after_colon[2..]; // skip "://"
                    } else {
                        rem = after_colon; // e.g. "blob:", "mailto:"
                    }
                }
            }
        }

        // 2. Hash fragment ("#...") via SIMD byte scan
        if (h_idx := Simd.indexOfByte(rem, 35 as u8)) { // '#'
            url.hash = rem[h_idx..];
            rem = rem[0..h_idx];
        }

        // 3. Search query string ("?...") via SIMD byte scan
        if (q_idx := Simd.indexOfByte(rem, 63 as u8)) { // '?'
            url.search = rem[q_idx..];
            url.searchParams = URLSearchParams.parse(url.search, alloc);
            rem = rem[0..q_idx];
        }

        // 4. Pathname separation ("/" or "\\" for special schemes)
        const special_scheme = isSpecialScheme(url.scheme);
        const path_start = simdFindPathDelimiter(rem, special_scheme);
        let authority = rem;
        if (p_start := path_start) {
            url.pathname = rem[p_start..];
            authority = rem[0..p_start];
        } else {
            url.pathname = "/";
            authority = rem;
        }
        url.path = url.pathname;

        // 5. Userinfo ("user:pass@host") — split at LAST '@' per WHATWG to prevent host confusion
        if (at_idx := urlLastIndexOfChar(authority, 64 as u8)) { // '@'
            const userinfo = authority[0..at_idx];
            authority = authority[at_idx + 1 ..];

            if (colon_idx := Simd.indexOfByte(userinfo, 58 as u8)) { // ':'
                url.username = userinfo[0..colon_idx];
                url.password = userinfo[colon_idx + 1 ..];
            } else {
                url.username = userinfo;
                url.password = "";
            }
        }

        // 6. Host & Port (handles IPv6 "[::1]:8080" and IPv4/domain "host:port")
        url.host = authority;
        if (authority.len > 0 and authority[0] == (91 as u8)) { // '[' for IPv6
            if (close_idx := Simd.indexOfByte(authority, 93 as u8)) { // ']'
                url.hostname = authority[0 .. close_idx + 1];
                const after_bracket = authority[close_idx + 1 ..];
                if (after_bracket.len > 1 and after_bracket[0] == (58 as u8)) { // ':'
                    url.port = after_bracket[1..];
                } else {
                    url.port = "";
                }
            } else {
                url.hostname = authority;
                url.port = "";
            }
        } else {
            if (port_idx := urlLastIndexOfChar(authority, 58 as u8)) { // ':'
                url.hostname = authority[0..port_idx];
                url.port = authority[port_idx + 1 ..];
            } else {
                url.hostname = authority;
                url.port = "";
            }
        }

        // 7. Origin formatting
        if (url.isFile()) {
            url.origin = "null";
        } else if (url.protocol.len > 0 and url.host.len > 0) {
            if (url.username.len == 0 and url.password.len == 0) {
                // Slice directly from input: "protocol//host"
                const proto_host_end = url.protocol.len + 2 + url.host.len;
                if (proto_host_end <= input.len and std.mem.eql(u8, input[url.protocol.len + 2 .. proto_host_end], url.host)) {
                    url.origin = input[0..proto_host_end];
                } else if (a := alloc) {
                    url.origin = std.fmt.allocPrint(a, "{s}//{s}", .{ url.protocol, url.host })?;
                    url.allocated_origin = url.origin;
                }
            } else if (a := alloc) {
                url.origin = std.fmt.allocPrint(a, "{s}//{s}", .{ url.protocol, url.host })?;
                url.allocated_origin = url.origin;
            }
        }

        return url;
    }

    pub fn deinit(*_) {
        _.searchParams.deinit();
        if (orig := _.allocated_origin) {
            if (alloc := _.allocator) {
                alloc.free(orig);
            }
            _.allocated_origin = null;
        }
    }

    /// Formats the URL origin (`scheme://host` or `"null"` for file://) into `out` without heap allocation.
    pub fn formatOrigin(out: []u8): string {
        if (_.isFile()) {
            const null_str = "null";
            if (out.len < null_str.len) return "";
            @memcpy(out[0..null_str.len], null_str);
            return out[0..null_str.len];
        }
        if (_.protocol.len == 0 or _.host.len == 0) return "";
        const needed = _.protocol.len + 2 + _.host.len;
        if (out.len < needed) return "";
        @memcpy(out[0.._.protocol.len], _.protocol);
        out[_.protocol.len] = 47 as u8;
        out[_.protocol.len + 1] = 47 as u8;
        @memcpy(out[_.protocol.len + 2 .. needed], _.host);
        return out[0..needed];
    }

    /// Serializes the full URL back into `out` without heap allocation.
    pub fn format(out: []u8): string {
        let pos: usize = 0;
        if (_.protocol.len > 0) {
            if (pos + _.protocol.len > out.len) return out[0..pos];
            @memcpy(out[pos .. pos + _.protocol.len], _.protocol);
            pos += _.protocol.len;
            if (_.host.len > 0 or _.isFile()) {
                if (pos + 2 > out.len) return out[0..pos];
                out[pos] = 47 as u8;
                out[pos + 1] = 47 as u8;
                pos += 2;
            }
        }
        if (_.username.len > 0) {
            if (pos + _.username.len > out.len) return out[0..pos];
            @memcpy(out[pos .. pos + _.username.len], _.username);
            pos += _.username.len;
            if (_.password.len > 0) {
                if (pos + 1 + _.password.len > out.len) return out[0..pos];
                out[pos] = 58 as u8; // ':'
                pos += 1;
                @memcpy(out[pos .. pos + _.password.len], _.password);
                pos += _.password.len;
            }
            if (pos + 1 > out.len) return out[0..pos];
            out[pos] = 64 as u8; // '@'
            pos += 1;
        }
        if (_.host.len > 0) {
            if (pos + _.host.len > out.len) return out[0..pos];
            @memcpy(out[pos .. pos + _.host.len], _.host);
            pos += _.host.len;
        }
        if (_.pathname.len > 0) {
            if (pos + _.pathname.len > out.len) return out[0..pos];
            @memcpy(out[pos .. pos + _.pathname.len], _.pathname);
            pos += _.pathname.len;
        }
        if (_.search.len > 0) {
            if (pos + _.search.len > out.len) return out[0..pos];
            @memcpy(out[pos .. pos + _.search.len], _.search);
            pos += _.search.len;
        }
        if (_.hash.len > 0) {
            if (pos + _.hash.len > out.len) return out[0..pos];
            @memcpy(out[pos .. pos + _.hash.len], _.hash);
            pos += _.hash.len;
        }
        return out[0..pos];
    }

    /// RFC 3986 Section 5.2.4 dot-segment normalization (resolves `.`, `..`, and `%2e` variants in `path`).
    pub static fn normalizePath(path: string, out: []u8): string {
        if (res := normalizeUrlPathImpl(path, false, out)) {
            return res;
        }
        if (out.len == 0) return "";
        out[0] = 47 as u8;
        return out[0..1];
    }

    /// Resolves a relative or absolute URL reference `relative_str` against `base_str` into `out` (RFC 3986 §5.2).
    pub static fn resolve(base_str: string, relative_str: string, out: []u8): string? {
        if (relative_str.len == 0) {
            if (base_str.len > out.len) return null;
            @memcpy(out[0..base_str.len], base_str);
            return out[0..base_str.len];
        }

        // Check if relative_str already has a scheme (e.g. "https://...")
        if (c_idx := simdFindUrlDelimiter(relative_str)) {
            if (relative_str[c_idx] == (58 as u8) and c_idx > 0) {
                let is_scheme = true;
                let k: usize = 0;
                while (k < c_idx) {
                    if (relative_str[k] == (64 as u8)) {
                        is_scheme = false;
                        break;
                    }
                    k += 1;
                }
                if (is_scheme) {
                    if (relative_str.len > out.len) return null;
                    @memcpy(out[0..relative_str.len], relative_str);
                    return out[0..relative_str.len];
                }
            }
        }

        const base = URL.parseSlice(base_str);
        if (base.protocol.len == 0) return null;

        // Protocol-relative "//host/path..."
        if (relative_str.len >= 2 and relative_str[0] == (47 as u8) and relative_str[1] == (47 as u8)) {
            const needed = base.protocol.len + relative_str.len;
            if (needed > out.len) return null;
            @memcpy(out[0..base.protocol.len], base.protocol);
            @memcpy(out[base.protocol.len .. needed], relative_str);
            return out[0..needed];
        }

        // Split relative_str into path, ?search, #hash
        let rel = relative_str;
        let rel_hash: string = "";
        if (h_idx := Simd.indexOfByte(rel, 35 as u8)) {
            rel_hash = rel[h_idx..];
            rel = rel[0..h_idx];
        }
        let rel_search: string = "";
        if (q_idx := Simd.indexOfByte(rel, 63 as u8)) {
            rel_search = rel[q_idx..];
            rel = rel[0..q_idx];
        }

        let norm_path_buf: [1024]u8 = undefined;
        let final_path: string = base.pathname;
        let final_search: string = rel_search;
        const special = isSpecialScheme(base.scheme);

        if (rel.len == 0) {
            if (rel_search.len == 0) {
                final_search = base.search;
            }
        } else if (rel[0] == (47 as u8) or (special and rel[0] == (92 as u8))) {
            final_path = normalizeUrlPathImpl(rel, special, &norm_path_buf) orelse URL.normalizePath(rel, &norm_path_buf);
        } else {
            // Merge relative path with base directory
            let merged_buf: [1024]u8 = undefined;
            let m_pos: usize = 0;
            if (last_sl := urlLastIndexOfChar(base.pathname, 47 as u8)) {
                const dir_len = last_sl + 1;
                if (dir_len <= merged_buf.len) {
                    @memcpy(merged_buf[0..dir_len], base.pathname[0..dir_len]);
                    m_pos = dir_len;
                }
            } else {
                merged_buf[0] = 47 as u8;
                m_pos = 1;
            }
            if (m_pos + rel.len > merged_buf.len) return null;
            @memcpy(merged_buf[m_pos .. m_pos + rel.len], rel);
            m_pos += rel.len;
            final_path = normalizeUrlPathImpl(merged_buf[0..m_pos], special, &norm_path_buf) orelse URL.normalizePath(merged_buf[0..m_pos], &norm_path_buf);
        }

        let resolved = URL{
            .protocol = base.protocol,
            .scheme = base.scheme,
            .username = base.username,
            .password = base.password,
            .host = base.host,
            .hostname = base.hostname,
            .port = base.port,
            .pathname = final_path,
            .path = final_path,
            .search = final_search,
            .hash = rel_hash,
        };
        return resolved.format(out);
    }

    /// Resolves `input` against `base_str` into `out_buf` and parses the resulting `URL`.
    pub static fn parseWithBase(input: string, base_str: string, alloc: std.mem.Allocator?, out_buf: []u8): !URL {
        const resolved = URL.resolve(base_str, input, out_buf) orelse return err.InvalidUrl;
        return URL.parse(resolved, alloc);
    }
}

pub const Url = URL;
