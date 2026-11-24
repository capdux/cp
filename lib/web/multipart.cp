// Reference-hardened against rwf2/multer (MIT) & RFC 7578/6570
// Source: cp/lib/multipart/multipart.cp (Pure CP RFC 7578 multipart/form-data Parser, Boundary Extractor & Builder)
// Zero-C-dependency RFC 7578 multipart/form-data boundary extractor, binary-safe parser, and builder
// with 16-lane @Vector(16, u8) SIMD (`MpUriVec16u8`) boundary/CRLF scanning and header/part DoS bounds.

const std = @import("std");

/// Module-local 16-lane byte vector alias for multipart boundary & CRLF scanning hot paths.
type MpUriVec16u8 = @Vector(16, u8);

pub const DEFAULT_MAX_PARTS: usize = 1024;
pub const DEFAULT_MAX_PART_BYTES: usize = 64 * 1024 * 1024;
pub const DEFAULT_MAX_HEADERS_PER_PART: usize = 64;

/// Error set returned by `cap:multipart` extraction, parsing, and building operations.
pub err MultipartError {
    InvalidBoundary,
    InvalidContentType,
    MalformedPartHeader,
    MissingEndBoundary,
    TooManyParts,
    PartTooLarge,
    InvalidHeaderValue,
    OutOfMemory,
}

// ============================================================================
// 16-Lane SIMD Boundary / CRLF Scanner (0-Diff with Scalar Reference)
// ============================================================================

/// Scalar reference: finds the first index of `'-'`, `'\r'`, or `'\n'` in `s`, or `null`.
pub fn scalarMpFindDashOrCrlf(s: []const u8): usize? {
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        const c = s[i];
        if (c == '-' || c == '\r' || c == '\n') return i;
    }
    return null;
}

/// 16-lane SIMD accelerated scanner for `'-'`, `'\r'`, or `'\n'` (wired into multipart boundary search).
pub fn simdMpFindDashOrCrlf(s: []const u8): usize? {
    const v_dash: MpUriVec16u8 = @splat('-');
    const v_cr: MpUriVec16u8 = @splat('\r');
    const v_lf: MpUriVec16u8 = @splat('\n');
    let i: usize = 0;
    while (i + 16 <= s.len) {
        const v: MpUriVec16u8 = s[i .. i + 16][0..16].*;
        const hit = (v == v_dash) | (v == v_cr) | (v == v_lf);
        const mask: u16 = @bitCast(hit);
        if (mask != 0) {
            return i + (@ctz(mask) as usize);
        }
        i += 16;
    }
    while (i < s.len) : (i += 1) {
        const c = s[i];
        if (c == '-' || c == '\r' || c == '\n') return i;
    }
    return null;
}

/// A single parsed `multipart/form-data` part.
pub struct MultipartPart {
    pub name: []const u8 = "",
    pub filename: ([]const u8)? = null,
    pub content_type: []const u8 = "text/plain",
    pub data: []const u8 = "",
}

/// Parsed `multipart/form-data` document owning all extracted part buffers.
pub struct MultipartDocument {
    pub parts: []MultipartPart = [],

    /// Returns the first part matching `name`, or `null` if not present.
    pub fn findField(name: []const u8): MultipartPart? {
        let i: usize = 0;
        while (i < _.parts.len) : (i += 1) {
            if (std.mem.eql(u8, _.parts[i].name, name)) {
                return _.parts[i];
            }
        }
        return null;
    }

    /// Returns a newly allocated slice of all parts matching `name`.
    /// Caller owns the returned slice and must free it with `allocator.free(...)`.
    pub fn findAll(allocator: std.mem.Allocator, name: []const u8): ([]MultipartPart) ! MultipartError {
        let count: usize = 0;
        let i: usize = 0;
        while (i < _.parts.len) : (i += 1) {
            if (std.mem.eql(u8, _.parts[i].name, name)) {
                count += 1;
            }
        }
        let out = allocator.alloc(MultipartPart, count) catch return MultipartError.OutOfMemory;
        let idx: usize = 0;
        i = 0;
        while (i < _.parts.len) : (i += 1) {
            if (std.mem.eql(u8, _.parts[i].name, name)) {
                out[idx] = _.parts[i];
                idx += 1;
            }
        }
        return out;
    }

    /// Frees all memory owned by this document and its parts.
    pub fn deinit(*_, allocator: std.mem.Allocator) {
        let i: usize = 0;
        while (i < _.parts.len) : (i += 1) {
            allocator.free(_.parts[i].name);
            if (fname := _.parts[i].filename) {
                allocator.free(fname);
            }
            allocator.free(_.parts[i].content_type);
            allocator.free(_.parts[i].data);
        }
        if (_.parts.len > 0) {
            allocator.free(_.parts);
        }
        _.parts = [];
    }
}

fn mpTrimWs(s: []const u8): []const u8 {
    return std.mem.trim(u8, s, " \t\r\n");
}

fn mpHasCtl(s: []const u8): bool {
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        const ch = s[i];
        if (ch < 0x20 || ch == 0x7F) return true;
    }
    return false;
}

fn mpIsBoundaryCharNoSpace(ch: u8): bool {
    if (ch >= '0' && ch <= '9') return true;
    if (ch >= 'A' && ch <= 'Z') return true;
    if (ch >= 'a' && ch <= 'z') return true;
    return ch == '\'' || ch == '(' || ch == ')' || ch == '+' || ch == '_' ||
        ch == ',' || ch == '-' || ch == '.' || ch == '/' || ch == ':' ||
        ch == '=' || ch == '?';
}

/// Validates an RFC 2046 §5.1.1 / RFC 7578 multipart boundary (1..=70 bytes, legal `bchars`,
/// no trailing space, no CR/LF/control bytes).
pub fn isValidMultipartBoundary(boundary: []const u8): bool {
    if (boundary.len == 0 || boundary.len > 70) return false;
    let i: usize = 0;
    while (i < boundary.len) : (i += 1) {
        const ch = boundary[i];
        if (ch == ' ') {
            if (i == boundary.len - 1) return false;
        } else if (!mpIsBoundaryCharNoSpace(ch)) {
            return false;
        }
    }
    return true;
}

/// Extracts and validates the `boundary` parameter from a `Content-Type: multipart/form-data; ...` header.
/// Supports case-insensitive media type/parameter names, semicolon-separated parameters, and quoted values.
pub fn extractBoundary(content_type_header: []const u8): ([]const u8) ! MultipartError {
    if (mpHasCtl(content_type_header)) return MultipartError.InvalidContentType;
    const trimmed = mpTrimWs(content_type_header);
    if (trimmed.len == 0) return MultipartError.InvalidContentType;

    const semi_idx = std.mem.indexOfScalar(u8, trimmed, ';') ?? trimmed.len;
    const media_type = mpTrimWs(trimmed[0..semi_idx]);
    if (!std.ascii.eqlIgnoreCase(media_type, "multipart/form-data")) {
        return MultipartError.InvalidContentType;
    }

    if (semi_idx >= trimmed.len) {
        return MultipartError.InvalidBoundary;
    }

    let pos: usize = semi_idx + 1;
    while (pos < trimmed.len) {
        while (pos < trimmed.len && (trimmed[pos] == ' ' || trimmed[pos] == '\t' || trimmed[pos] == ';')) {
            pos += 1;
        }
        if (pos >= trimmed.len) break;

        const param_start = pos;
        let eq_pos: usize? = null;
        while (pos < trimmed.len && trimmed[pos] != ';') {
            if (trimmed[pos] == '=' && eq_pos == null) {
                eq_pos = pos;
            } else if (trimmed[pos] == '"') {
                pos += 1;
                while (pos < trimmed.len && trimmed[pos] != '"') {
                    if (trimmed[pos] == '\\' && pos + 1 < trimmed.len) {
                        pos += 2;
                    } else {
                        pos += 1;
                    }
                }
                if (pos >= trimmed.len) return MultipartError.InvalidBoundary;
            }
            pos += 1;
        }

        if (eq := eq_pos) {
            const key = mpTrimWs(trimmed[param_start..eq]);
            let val = mpTrimWs(trimmed[eq + 1 .. pos]);
            if (std.ascii.eqlIgnoreCase(key, "boundary")) {
                if (val.len >= 2 && val[0] == '"') {
                    if (val[val.len - 1] != '"') return MultipartError.InvalidBoundary;
                    val = val[1 .. val.len - 1];
                } else if (std.mem.indexOfScalar(u8, val, '"') != null) {
                    return MultipartError.InvalidBoundary;
                }
                if (!isValidMultipartBoundary(val)) return MultipartError.InvalidBoundary;
                return val;
            }
        }
    }

    return MultipartError.InvalidBoundary;
}

priv struct MpBoundaryMatch {
    is_closing: bool = false,
    next_pos: usize = 0,
}

fn mpMatchBoundaryAt(body: []const u8, pos: usize, dash_bnd: []const u8): MpBoundaryMatch? {
    if (pos + dash_bnd.len > body.len) return null;
    if (!std.mem.eql(u8, body[pos .. pos + dash_bnd.len], dash_bnd)) return null;

    const after = pos + dash_bnd.len;
    if (after + 2 <= body.len && body[after] == '-' && body[after + 1] == '-') {
        const end_after = after + 2;
        if (end_after == body.len || body[end_after] == '\r' || body[end_after] == '\n' ||
            body[end_after] == ' ' || body[end_after] == '\t')
        {
            return MpBoundaryMatch{ is_closing: true, next_pos: end_after };
        }
        return null;
    }

    if (after + 2 <= body.len && body[after] == '\r' && body[after + 1] == '\n') {
        return MpBoundaryMatch{ is_closing: false, next_pos: after + 2 };
    }
    if (after < body.len && body[after] == '\n') {
        return MpBoundaryMatch{ is_closing: false, next_pos: after + 1 };
    }
    return null;
}

/// 16-lane SIMD accelerated search for the first boundary delimiter (`--boundary`).
fn mpFindFirstBoundary(body: []const u8, dash_bnd: []const u8): MpBoundaryMatch? {
    let search_pos: usize = 0;
    while (search_pos < body.len) {
        const rel = simdMpFindDashOrCrlf(body[search_pos..]) ?? return null;
        const idx = search_pos + rel;
        if (body[idx] == '-' && (idx == 0 || body[idx - 1] == '\n')) {
            if (m := mpMatchBoundaryAt(body, idx, dash_bnd)) {
                return m;
            }
        }
        search_pos = idx + 1;
    }
    return null;
}

priv struct MpNextBoundaryResult {
    data_end: usize = 0,
    match_info: MpBoundaryMatch = MpBoundaryMatch{},
}

/// 16-lane SIMD accelerated search for the next boundary delimiter after `data_start`.
/// Strictly rejects fake prefixes such as `--boundary_extra`.
fn mpFindNextBoundary(body: []const u8, data_start: usize, dash_bnd: []const u8): MpNextBoundaryResult? {
    let search_pos = data_start;
    while (search_pos < body.len) {
        const rel = simdMpFindDashOrCrlf(body[search_pos..]) ?? return null;
        const idx = search_pos + rel;
        if (body[idx] == '-') {
            let data_end: usize = idx;
            let valid_line_start = false;
            if (idx >= 2 && body[idx - 2] == '\r' && body[idx - 1] == '\n') {
                data_end = idx - 2;
                valid_line_start = true;
            } else if (idx >= 1 && body[idx - 1] == '\n') {
                data_end = idx - 1;
                valid_line_start = true;
            }
            if (valid_line_start && data_end >= data_start) {
                if (m := mpMatchBoundaryAt(body, idx, dash_bnd)) {
                    return MpNextBoundaryResult{ data_end: data_end, match_info: m };
                }
            }
        }
        search_pos = idx + 1;
    }
    return null;
}

fn mpUnescapeQuotedAlloc(allocator: std.mem.Allocator, raw: []const u8): ([]u8) ! MultipartError {
    let out_len: usize = 0;
    let i: usize = 0;
    while (i < raw.len) : (i += 1) {
        if (raw[i] == '\\' && i + 1 < raw.len) {
            i += 1;
        }
        out_len += 1;
    }
    let out = allocator.alloc(u8, out_len) catch return MultipartError.OutOfMemory;
    let w: usize = 0;
    i = 0;
    while (i < raw.len) : (i += 1) {
        if (raw[i] == '\\' && i + 1 < raw.len) {
            i += 1;
        }
        out[w] = raw[i];
        w += 1;
    }
    return out;
}

priv struct MpDispositionParsed {
    name: ([]u8)? = null,
    filename: ([]u8)? = null,

    fn deinit(*_, allocator: std.mem.Allocator) {
        if (n := _.name) allocator.free(n);
        if (f := _.filename) allocator.free(f);
        _.name = null;
        _.filename = null;
    }
}

fn mpParseContentDisposition(allocator: std.mem.Allocator, header_val: []const u8): MpDispositionParsed ! MultipartError {
    if (mpHasCtl(header_val)) return MultipartError.MalformedPartHeader;
    let res = MpDispositionParsed{};
    let pos: usize = 0;

    // Skip disposition-type (e.g., "form-data") up to first ';'
    const first_semi = std.mem.indexOfScalar(u8, header_val, ';') ?? header_val.len;
    const disp_type = mpTrimWs(header_val[0..first_semi]);
    if (!std.ascii.eqlIgnoreCase(disp_type, "form-data")) {
        return MultipartError.MalformedPartHeader;
    }
    pos = first_semi;

    while (pos < header_val.len) {
        while (pos < header_val.len && (header_val[pos] == ';' || header_val[pos] == ' ' || header_val[pos] == '\t')) {
            pos += 1;
        }
        if (pos >= header_val.len) break;

        const key_start = pos;
        while (pos < header_val.len && header_val[pos] != '=' && header_val[pos] != ';') {
            pos += 1;
        }
        if (pos >= header_val.len || header_val[pos] == ';') {
            continue;
        }
        const key = mpTrimWs(header_val[key_start..pos]);
        pos += 1; // skip '='

        while (pos < header_val.len && (header_val[pos] == ' ' || header_val[pos] == '\t')) {
            pos += 1;
        }

        let raw_val: []const u8 = "";
        let is_quoted = false;
        if (pos < header_val.len && header_val[pos] == '"') {
            is_quoted = true;
            pos += 1;
            const val_start = pos;
            while (pos < header_val.len && header_val[pos] != '"') {
                if (header_val[pos] == '\\' && pos + 1 < header_val.len) {
                    pos += 2;
                } else {
                    pos += 1;
                }
            }
            if (pos >= header_val.len) {
                res.deinit(allocator);
                return MultipartError.MalformedPartHeader;
            }
            raw_val = header_val[val_start..pos];
            pos += 1; // skip closing '"'
        } else {
            const val_start = pos;
            while (pos < header_val.len && header_val[pos] != ';') {
                pos += 1;
            }
            raw_val = mpTrimWs(header_val[val_start..pos]);
        }

        if (std.ascii.eqlIgnoreCase(key, "name")) {
            const duped = if (is_quoted)
                (mpUnescapeQuotedAlloc(allocator, raw_val) catch {
                    res.deinit(allocator);
                    return MultipartError.OutOfMemory;
                })
            else
                (allocator.dupe(u8, raw_val) catch {
                    res.deinit(allocator);
                    return MultipartError.OutOfMemory;
                });
            if (mpHasCtl(duped)) {
                allocator.free(duped);
                res.deinit(allocator);
                return MultipartError.InvalidHeaderValue;
            }
            if (old := res.name) allocator.free(old);
            res.name = duped;
        } else if (std.ascii.eqlIgnoreCase(key, "filename")) {
            const duped = if (is_quoted)
                (mpUnescapeQuotedAlloc(allocator, raw_val) catch {
                    res.deinit(allocator);
                    return MultipartError.OutOfMemory;
                })
            else
                (allocator.dupe(u8, raw_val) catch {
                    res.deinit(allocator);
                    return MultipartError.OutOfMemory;
                });
            if (mpHasCtl(duped)) {
                allocator.free(duped);
                res.deinit(allocator);
                return MultipartError.InvalidHeaderValue;
            }
            if (old := res.filename) allocator.free(old);
            res.filename = duped;
        }
    }

    return res;
}

fn mpFreePartsSlice(allocator: std.mem.Allocator, parts: []MultipartPart, count: usize) {
    let i: usize = 0;
    while (i < count) : (i += 1) {
        allocator.free(parts[i].name);
        if (fname := parts[i].filename) allocator.free(fname);
        allocator.free(parts[i].content_type);
        allocator.free(parts[i].data);
    }
    if (parts.len > 0) {
        allocator.free(parts);
    }
}

/// Parses a binary-safe `multipart/form-data` payload with default limits (`DEFAULT_MAX_PARTS`, `DEFAULT_MAX_PART_BYTES`).
pub fn parse(allocator: std.mem.Allocator, boundary: []const u8, body: []const u8): MultipartDocument ! MultipartError {
    return parseWithLimits(allocator, boundary, body, DEFAULT_MAX_PARTS, DEFAULT_MAX_PART_BYTES);
}

/// Parses a binary-safe `multipart/form-data` payload with explicit part count and per-part byte limits.
/// Enforces `DEFAULT_MAX_HEADERS_PER_PART` per part to defend against header-count exhaustion.
pub fn parseWithLimits(
    allocator: std.mem.Allocator,
    boundary: []const u8,
    body: []const u8,
    max_parts: usize,
    max_part_bytes: usize,
): MultipartDocument ! MultipartError {
    if (!isValidMultipartBoundary(boundary)) return MultipartError.InvalidBoundary;

    let dash_bnd_buf: [72]u8 = undefined;
    dash_bnd_buf[0] = '-';
    dash_bnd_buf[1] = '-';
    @memcpy(dash_bnd_buf[2 .. 2 + boundary.len], boundary);
    const dash_bnd = dash_bnd_buf[0 .. 2 + boundary.len];

    const first = mpFindFirstBoundary(body, dash_bnd) orelse return MultipartError.MissingEndBoundary;
    if (first.is_closing) {
        return MultipartDocument{ parts: [] };
    }

    let parts_cap: usize = 0;
    let parts_len: usize = 0;
    let parts_buf: []MultipartPart = [];

    let cursor: usize = first.next_pos;
    while (true) {
        let disp: MpDispositionParsed = MpDispositionParsed{};
        let raw_ct: []const u8 = "text/plain";
        let has_disp = false;
        let data_start: usize = cursor;
        let header_count: usize = 0;

        // Parse part headers until blank line (\r\n or \n)
        while (true) {
            const nl_idx_opt = std.mem.indexOfScalarPos(u8, body, cursor, '\n');
            if (nl_idx_opt == null) {
                disp.deinit(allocator);
                mpFreePartsSlice(allocator, parts_buf, parts_len);
                return MultipartError.MalformedPartHeader;
            }
            const nl_idx = nl_idx_opt.?;
            let line = body[cursor..nl_idx];
            if (line.len > 0 && line[line.len - 1] == '\r') {
                line = line[0 .. line.len - 1];
            }
            cursor = nl_idx + 1;

            if (line.len == 0) {
                data_start = cursor;
                break;
            }

            header_count += 1;
            if (header_count > DEFAULT_MAX_HEADERS_PER_PART) {
                disp.deinit(allocator);
                mpFreePartsSlice(allocator, parts_buf, parts_len);
                return MultipartError.MalformedPartHeader;
            }

            const colon_opt = std.mem.indexOfScalar(u8, line, ':');
            if (colon_opt == null) {
                disp.deinit(allocator);
                mpFreePartsSlice(allocator, parts_buf, parts_len);
                return MultipartError.MalformedPartHeader;
            }
            const colon = colon_opt.?;
            const hname = mpTrimWs(line[0..colon]);
            const hval = mpTrimWs(line[colon + 1 ..]);
            if (hname.len == 0 || mpHasCtl(hname)) {
                disp.deinit(allocator);
                mpFreePartsSlice(allocator, parts_buf, parts_len);
                return MultipartError.MalformedPartHeader;
            }

            if (std.ascii.eqlIgnoreCase(hname, "Content-Disposition")) {
                disp.deinit(allocator);
                disp = mpParseContentDisposition(allocator, hval) catch |e| {
                    mpFreePartsSlice(allocator, parts_buf, parts_len);
                    return e;
                };
                has_disp = true;
            } else if (std.ascii.eqlIgnoreCase(hname, "Content-Type")) {
                if (mpHasCtl(hval)) {
                    disp.deinit(allocator);
                    mpFreePartsSlice(allocator, parts_buf, parts_len);
                    return MultipartError.InvalidHeaderValue;
                }
                if (hval.len > 0) {
                    raw_ct = hval;
                }
            }
        }

        if (!has_disp) {
            disp.deinit(allocator);
            mpFreePartsSlice(allocator, parts_buf, parts_len);
            return MultipartError.MalformedPartHeader;
        }
        const part_name = disp.name orelse {
            disp.deinit(allocator);
            mpFreePartsSlice(allocator, parts_buf, parts_len);
            return MultipartError.MalformedPartHeader;
        };
        if (part_name.len == 0) {
            disp.deinit(allocator);
            mpFreePartsSlice(allocator, parts_buf, parts_len);
            return MultipartError.MalformedPartHeader;
        }

        const next_res_opt = mpFindNextBoundary(body, data_start, dash_bnd);
        if (next_res_opt == null) {
            disp.deinit(allocator);
            mpFreePartsSlice(allocator, parts_buf, parts_len);
            return MultipartError.MissingEndBoundary;
        }
        const next_res = next_res_opt.?;

        if (parts_len >= max_parts) {
            disp.deinit(allocator);
            mpFreePartsSlice(allocator, parts_buf, parts_len);
            return MultipartError.TooManyParts;
        }

        const raw_data = body[data_start..next_res.data_end];
        if (raw_data.len > max_part_bytes) {
            disp.deinit(allocator);
            mpFreePartsSlice(allocator, parts_buf, parts_len);
            return MultipartError.PartTooLarge;
        }

        const ct_duped = allocator.dupe(u8, raw_ct) catch {
            disp.deinit(allocator);
            mpFreePartsSlice(allocator, parts_buf, parts_len);
            return MultipartError.OutOfMemory;
        };
        const data_duped = allocator.dupe(u8, raw_data) catch {
            allocator.free(ct_duped);
            disp.deinit(allocator);
            mpFreePartsSlice(allocator, parts_buf, parts_len);
            return MultipartError.OutOfMemory;
        };

        if (parts_len == parts_cap) {
            const new_cap = if (parts_cap == 0) 8 else parts_cap * 2;
            const new_buf = if (parts_cap == 0)
                (allocator.alloc(MultipartPart, new_cap) catch {
                    allocator.free(data_duped);
                    allocator.free(ct_duped);
                    disp.deinit(allocator);
                    mpFreePartsSlice(allocator, parts_buf, parts_len);
                    return MultipartError.OutOfMemory;
                })
            else
                (allocator.realloc(parts_buf, new_cap) catch {
                    allocator.free(data_duped);
                    allocator.free(ct_duped);
                    disp.deinit(allocator);
                    mpFreePartsSlice(allocator, parts_buf, parts_len);
                    return MultipartError.OutOfMemory;
                });
            parts_buf = new_buf;
            parts_cap = new_cap;
        }

        parts_buf[parts_len] = MultipartPart{
            name: part_name,
            filename: disp.filename,
            content_type: ct_duped,
            data: data_duped,
        };
        parts_len += 1;

        if (next_res.match_info.is_closing) {
            break;
        }
        cursor = next_res.match_info.next_pos;
    }

    if (parts_len == 0) {
        if (parts_cap > 0) allocator.free(parts_buf);
        return MultipartDocument{ parts: [] };
    }
    const final_parts = allocator.realloc(parts_buf, parts_len) catch parts_buf[0..parts_len];
    return MultipartDocument{ parts: final_parts };
}

/// Incremental `multipart/form-data` payload builder with CR/LF/control header injection protection.
pub struct MultipartBuilder {
    allocator: std.mem.Allocator = undefined,
    boundary_buf: [72]u8 = undefined,
    boundary_len: usize = 0,
    buf: []u8 = [],
    len: usize = 0,

    /// Initializes a `MultipartBuilder` with an explicit validated `boundary`.
    pub static fn init(allocator: std.mem.Allocator, boundary: []const u8): MultipartBuilder ! MultipartError {
        if (!isValidMultipartBoundary(boundary)) return MultipartError.InvalidBoundary;
        let b = MultipartBuilder{
            allocator: allocator,
            boundary_buf: undefined,
            boundary_len: boundary.len,
            buf: [],
            len: 0,
        };
        @memcpy(b.boundary_buf[0..boundary.len], boundary);
        return b;
    }

    /// Releases internal builder buffers.
    pub fn deinit(*_) {
        if (_.buf.len > 0) {
            _.allocator.free(_.buf);
        }
        _.buf = [];
        _.len = 0;
    }

    fn appendBytes(*_, src: []const u8): void ! MultipartError {
        if (src.len == 0) return;
        const needed = _.len + src.len;
        if (needed > _.buf.len) {
            let new_cap = if (_.buf.len == 0) 512 else _.buf.len * 2;
            while (new_cap < needed) {
                new_cap *= 2;
            }
            if (_.buf.len == 0) {
                _.buf = _.allocator.alloc(u8, new_cap) catch return MultipartError.OutOfMemory;
            } else {
                _.buf = _.allocator.realloc(_.buf, new_cap) catch return MultipartError.OutOfMemory;
            }
        }
        @memcpy(_.buf[_.len .. _.len + src.len], src);
        _.len += src.len;
    }

    fn appendEscapedParam(*_, src: []const u8): void ! MultipartError {
        let i: usize = 0;
        while (i < src.len) : (i += 1) {
            const ch = src[i];
            if (ch == '"' || ch == '\\') {
                _.appendBytes("\\")?;
            }
            const one: [1]u8 = [ch];
            _.appendBytes(&one)?;
        }
    }

    /// Appends a text form field. Rejects empty or CR/LF/control-containing field names.
    pub fn addField(*_, name: []const u8, value: []const u8): void ! MultipartError {
        if (name.len == 0 || mpHasCtl(name)) return MultipartError.InvalidHeaderValue;
        const bnd = _.boundary_buf[0.._.boundary_len];
        _.appendBytes("--")?;
        _.appendBytes(bnd)?;
        _.appendBytes("\r\nContent-Disposition: form-data; name=\"")?;
        _.appendEscapedParam(name)?;
        _.appendBytes("\"\r\n\r\n")?;
        _.appendBytes(value)?;
        _.appendBytes("\r\n")?;
    }

    /// Appends a file part with filename and Content-Type. Rejects CR/LF/control injection in header fields.
    pub fn addFile(
        *_,
        name: []const u8,
        filename: []const u8,
        content_type: []const u8,
        data: []const u8,
    ): void ! MultipartError {
        if (name.len == 0 || mpHasCtl(name) || mpHasCtl(filename) || mpHasCtl(content_type)) {
            return MultipartError.InvalidHeaderValue;
        }
        const bnd = _.boundary_buf[0.._.boundary_len];
        _.appendBytes("--")?;
        _.appendBytes(bnd)?;
        _.appendBytes("\r\nContent-Disposition: form-data; name=\"")?;
        _.appendEscapedParam(name)?;
        _.appendBytes("\"; filename=\"")?;
        _.appendEscapedParam(filename)?;
        _.appendBytes("\"\r\n")?;
        if (content_type.len > 0) {
            _.appendBytes("Content-Type: ")?;
            _.appendBytes(content_type)?;
            _.appendBytes("\r\n")?;
        }
        _.appendBytes("\r\n")?;
        _.appendBytes(data)?;
        _.appendBytes("\r\n")?;
    }

    /// Allocates and returns `"multipart/form-data; boundary=<boundary>"`.
    /// Caller owns the returned slice and must free it with `allocator.free(...)`.
    pub fn contentTypeHeader(*_, allocator: std.mem.Allocator): ([]u8) ! MultipartError {
        const prefix = "multipart/form-data; boundary=";
        const bnd = _.boundary_buf[0.._.boundary_len];
        let out = allocator.alloc(u8, prefix.len + bnd.len) catch return MultipartError.OutOfMemory;
        @memcpy(out[0..prefix.len], prefix);
        @memcpy(out[prefix.len .. prefix.len + bnd.len], bnd);
        return out;
    }

    /// Builds and returns the complete `multipart/form-data` payload ending with `--<boundary>--\r\n`.
    /// Caller owns the returned slice and must free it with `allocator.free(...)`.
    pub fn finish(*_, allocator: std.mem.Allocator): ([]u8) ! MultipartError {
        const bnd = _.boundary_buf[0.._.boundary_len];
        const total = _.len + 2 + bnd.len + 4;
        let out = allocator.alloc(u8, total) catch return MultipartError.OutOfMemory;
        if (_.len > 0) {
            @memcpy(out[0.._.len], _.buf[0.._.len]);
        }
        let pos = _.len;
        out[pos] = '-';
        out[pos + 1] = '-';
        pos += 2;
        @memcpy(out[pos .. pos + bnd.len], bnd);
        pos += bnd.len;
        @memcpy(out[pos .. pos + 4], "--\r\n");
        return out;
    }
}
