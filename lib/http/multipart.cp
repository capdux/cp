// Source: cp/lib/http/multipart.cp (RFC 7578 multipart/form-data Parser & Builder)
// lib/http/multipart.cp
// RFC 7578 Binary-Safe Multipart FormData Parser, Boundary Extractor & Builder (cap:http)

const std = @import("std");

pub const MAX_MULTIPART_PARTS: usize = 4096;

pub struct MultipartPart {
    pub name: []const u8 = "",
    pub filename: ([]const u8)? = null,
    pub content_type: ([]const u8)? = null,
    pub body: []const u8 = "",
}

fn multipartTrimWs(s: []const u8): []const u8 {
    return std.mem.trim(u8, s, " \t\r\n");
}

fn multipartHasCtl(s: []const u8): bool {
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        if (s[i] == '\r' || s[i] == '\n' || s[i] == 0) return true;
    }
    return false;
}

priv struct MultipartByteBuf {
    data: []u8 = [],
    len: usize = 0,

    fn deinit(*_, allocator: std.mem.Allocator) {
        if (_.data.len > 0) {
            allocator.free(_.data);
        }
        _.data = [];
        _.len = 0;
    }

    fn appendBytes(*_, allocator: std.mem.Allocator, src: []const u8): ! {
        if (src.len == 0) return;
        const needed = _.len + src.len;
        if (needed > _.data.len) {
            let new_cap = if (_.data.len == 0) 512 else _.data.len * 2;
            while (new_cap < needed) {
                new_cap *= 2;
            }
            if (_.data.len == 0) {
                _.data = try allocator.alloc(u8, new_cap);
            } else {
                _.data = try allocator.realloc(_.data, new_cap);
            }
        }
        @memcpy(_.data[_.len .. _.len + src.len], src);
        _.len += src.len;
    }

    fn appendQuotedParam(*_, allocator: std.mem.Allocator, s: []const u8): ! {
        let i: usize = 0;
        while (i < s.len) : (i += 1) {
            const ch = s[i];
            if (ch == '"') {
                try _.appendBytes(allocator, "%22");
            } else {
                const arr: [1]u8 = [ch];
                try _.appendBytes(allocator, &arr);
            }
        }
    }
}

pub struct MultipartBoundary {
    pub static fn isValid(boundary_str: []const u8): bool {
        if (boundary_str.len == 0 || boundary_str.len > 70) return false;
        if (multipartHasCtl(boundary_str)) return false;
        return true;
    }

    /// Extracts the `boundary` parameter from a `Content-Type: multipart/...; boundary=...` header.
    /// Supports both quoted and unquoted values and rejects non-multipart or empty boundaries.
    pub static fn extractFromContentType(content_type: []const u8): ([]const u8)? {
        let iter = std.mem.splitScalar(u8, content_type, ';');
        const media_raw = iter.next() ?? return null;
        const media = multipartTrimWs(media_raw);
        if (media.len < 10) return null;
        if (!std.ascii.eqlIgnoreCase(media[0..10], "multipart/")) return null;

        while (param_raw := iter.next()) {
            const param = multipartTrimWs(param_raw);
            if (param.len == 0) continue;
            if (eq := std.mem.indexOfScalar(u8, param, '=')) {
                const key = multipartTrimWs(param[0..eq]);
                if (std.ascii.eqlIgnoreCase(key, "boundary")) {
                    let val = multipartTrimWs(param[eq + 1 ..]);
                    if (val.len >= 2 && val[0] == '"') {
                        if (end_q := std.mem.indexOfScalarPos(u8, val, 1, '"')) {
                            val = val[1..end_q];
                        } else {
                            return null;
                        }
                    }
                    if (!MultipartBoundary.isValid(val)) return null;
                    return val;
                }
            }
        }
        return null;
    }
}

pub struct MultipartBuilder {
    pub allocator: std.mem.Allocator = undefined,
    priv boundary_buf: [72]u8 = undefined,
    priv boundary_len: usize = 0,
    priv ct_buf: [128]u8 = undefined,
    priv ct_len: usize = 0,
    priv body_buf: MultipartByteBuf = MultipartByteBuf{},

    pub static fn init(allocator: std.mem.Allocator): MultipartBuilder {
        const default_boundary = "----CPMultipartBoundary7MA4YWxkTrZu0gW";
        return MultipartBuilder.initWithBoundary(allocator, default_boundary) catch unreachable;
    }

    pub static fn initWithBoundary(allocator: std.mem.Allocator, bnd_in: []const u8): MultipartBuilder! {
        if (!MultipartBoundary.isValid(bnd_in)) return error.InvalidBoundary;
        let b = MultipartBuilder{
            allocator: allocator,
            boundary_buf: undefined,
            boundary_len: bnd_in.len,
            ct_buf: undefined,
            ct_len: 0,
            body_buf: MultipartByteBuf{},
        };
        @memcpy(b.boundary_buf[0..bnd_in.len], bnd_in);
        const prefix = "multipart/form-data; boundary=";
        @memcpy(b.ct_buf[0..prefix.len], prefix);
        @memcpy(b.ct_buf[prefix.len .. prefix.len + bnd_in.len], bnd_in);
        b.ct_len = prefix.len + bnd_in.len;
        return b;
    }

    pub fn deinit(*_) {
        _.body_buf.deinit(_.allocator);
    }

    pub fn boundary(*_): []const u8 {
        return _.boundary_buf[0.._.boundary_len];
    }

    pub fn contentTypeHeader(*_): []const u8 {
        return _.ct_buf[0.._.ct_len];
    }

    pub fn addField(*_, name: []const u8, value: []const u8): ! {
        if (name.len == 0 || multipartHasCtl(name)) return error.InvalidFieldName;
        const bnd = _.boundary_buf[0.._.boundary_len];
        try _.body_buf.appendBytes(_.allocator, "--");
        try _.body_buf.appendBytes(_.allocator, bnd);
        try _.body_buf.appendBytes(_.allocator, "\r\nContent-Disposition: form-data; name=\"");
        try _.body_buf.appendQuotedParam(_.allocator, name);
        try _.body_buf.appendBytes(_.allocator, "\"\r\n\r\n");
        try _.body_buf.appendBytes(_.allocator, value);
        try _.body_buf.appendBytes(_.allocator, "\r\n");
    }

    pub fn addFile(*_, name: []const u8, filename: []const u8, content_type: []const u8, data: []const u8): ! {
        if (name.len == 0 || multipartHasCtl(name) || multipartHasCtl(filename) || multipartHasCtl(content_type)) {
            return error.InvalidFieldName;
        }
        const bnd = _.boundary_buf[0.._.boundary_len];
        try _.body_buf.appendBytes(_.allocator, "--");
        try _.body_buf.appendBytes(_.allocator, bnd);
        try _.body_buf.appendBytes(_.allocator, "\r\nContent-Disposition: form-data; name=\"");
        try _.body_buf.appendQuotedParam(_.allocator, name);
        try _.body_buf.appendBytes(_.allocator, "\"; filename=\"");
        try _.body_buf.appendQuotedParam(_.allocator, filename);
        try _.body_buf.appendBytes(_.allocator, "\"\r\n");
        if (content_type.len > 0) {
            try _.body_buf.appendBytes(_.allocator, "Content-Type: ");
            try _.body_buf.appendBytes(_.allocator, content_type);
            try _.body_buf.appendBytes(_.allocator, "\r\n");
        }
        try _.body_buf.appendBytes(_.allocator, "\r\n");
        try _.body_buf.appendBytes(_.allocator, data);
        try _.body_buf.appendBytes(_.allocator, "\r\n");
    }

    /// Builds the complete `multipart/form-data` payload including closing `--boundary--\r\n`.
    /// Caller owns the returned slice and must free it with `allocator.free(body)`.
    pub fn build(*_): ([]const u8)! {
        const bnd = _.boundary_buf[0.._.boundary_len];
        const total = _.body_buf.len + 2 + bnd.len + 4;
        let out = try _.allocator.alloc(u8, total);
        if (_.body_buf.len > 0) {
            @memcpy(out[0.._.body_buf.len], _.body_buf.data[0.._.body_buf.len]);
        }
        let pos = _.body_buf.len;
        out[pos] = '-';
        out[pos + 1] = '-';
        pos += 2;
        @memcpy(out[pos .. pos + bnd.len], bnd);
        pos += bnd.len;
        @memcpy(out[pos .. pos + 4], "--\r\n");
        return out;
    }
}

priv struct MultipartDispositionInfo {
    name: []const u8 = "",
    filename: ([]const u8)? = null,
}

fn multipartParseDisposition(header_val: []const u8): MultipartDispositionInfo {
    let info = MultipartDispositionInfo{};
    let iter = std.mem.splitScalar(u8, header_val, ';');
    * = iter.next(); // skip "form-data"
    while (param_raw := iter.next()) {
        const param = multipartTrimWs(param_raw);
        if (param.len == 0) continue;
        if (eq := std.mem.indexOfScalar(u8, param, '=')) {
            const k = multipartTrimWs(param[0..eq]);
            let v = multipartTrimWs(param[eq + 1 ..]);
            if (v.len >= 2 && v[0] == '"' && v[v.len - 1] == '"') {
                v = v[1 .. v.len - 1];
            }
            if (std.ascii.eqlIgnoreCase(k, "name")) {
                info.name = v;
            } else if (std.ascii.eqlIgnoreCase(k, "filename")) {
                info.filename = v;
            }
        }
    }
    return info;
}

priv struct MultipartBoundaryMatch {
    is_closing: bool = false;
    next_pos: usize = 0;
}

fn multipartCheckLineEndAfterBoundary(body: []const u8, after_dash_bnd: usize): MultipartBoundaryMatch? {
    let p = after_dash_bnd;
    if (p + 2 <= body.len && body[p] == '-' && body[p + 1] == '-') {
        return MultipartBoundaryMatch{ is_closing: true, next_pos: p + 2 };
    }
    while (p < body.len && (body[p] == ' ' || body[p] == '\t')) : (p += 1) {}
    if (p + 2 <= body.len && body[p] == '\r' && body[p + 1] == '\n') {
        return MultipartBoundaryMatch{ is_closing: false, next_pos: p + 2 };
    }
    if (p < body.len && body[p] == '\n') {
        return MultipartBoundaryMatch{ is_closing: false, next_pos: p + 1 };
    }
    return null;
}

pub struct MultipartParser {
    pub static fn freeParts(alloc: std.mem.Allocator, parts: []MultipartPart) {
        if (parts.len > 0) {
            alloc.free(parts);
        }
    }

    /// Parses a binary-safe `multipart/form-data` body using `boundary`.
    /// Skips preamble and epilogue. Returned `MultipartPart` slices borrow `body`; free slice with `freeParts`.
    pub static fn parse(alloc: std.mem.Allocator, boundary: []const u8, body: []const u8): ([]MultipartPart)! {
        if (!MultipartBoundary.isValid(boundary)) return error.InvalidBoundary;

        let dash_buf: [74]u8 = undefined;
        dash_buf[0] = '-';
        dash_buf[1] = '-';
        @memcpy(dash_buf[2 .. 2 + boundary.len], boundary);
        const dash_bnd = dash_buf[0 .. 2 + boundary.len];

        let pos: usize = 0;
        let found_first = false;
        let is_closing = false;

        if (std.mem.startsWith(u8, body, dash_bnd)) {
            if (info := multipartCheckLineEndAfterBoundary(body, dash_bnd.len)) {
                found_first = true;
                is_closing = info.is_closing;
                pos = info.next_pos;
            }
        }

        if (!found_first) {
            let search_pos: usize = 0;
            while (search_pos < body.len) {
                const idx = std.mem.indexOfPos(u8, body, search_pos, dash_bnd) ?? break;
                const preceded_by_nl = (idx == 0) || (body[idx - 1] == '\n');
                if (preceded_by_nl) {
                    if (info := multipartCheckLineEndAfterBoundary(body, idx + dash_bnd.len)) {
                        found_first = true;
                        is_closing = info.is_closing;
                        pos = info.next_pos;
                        break;
                    }
                }
                search_pos = idx + 1;
            }
        }

        if (!found_first) return error.BoundaryNotFound;
        if (is_closing) {
            return [];
        }

        let parts: []MultipartPart = [];
        let parts_len: usize = 0;
        let parts_cap: usize = 0;
        errdefer if (parts_cap > 0) alloc.free(parts);

        while (true) {
            if (parts_len >= MAX_MULTIPART_PARTS) return error.TooManyMultipartParts;

            let hdr_end: usize = pos;
            let body_start: usize = pos;

            if (pos + 2 <= body.len && body[pos] == '\r' && body[pos + 1] == '\n') {
                hdr_end = pos;
                body_start = pos + 2;
            } else if (pos < body.len && body[pos] == '\n') {
                hdr_end = pos;
                body_start = pos + 1;
            } else {
                const crlfcrlf = std.mem.indexOfPos(u8, body, pos, "\r\n\r\n");
                const lflf = std.mem.indexOfPos(u8, body, pos, "\n\n");
                if (crlfcrlf != null && (lflf == null || crlfcrlf.? <= lflf.?)) {
                    hdr_end = crlfcrlf.?;
                    body_start = hdr_end + 4;
                } else if (lflf != null) {
                    hdr_end = lflf.?;
                    body_start = hdr_end + 2;
                } else {
                    return error.MalformedMultipartHeaders;
                }
            }

            let part = MultipartPart{};
            const hdr_block = body[pos..hdr_end];
            let hpos: usize = 0;
            while (hpos < hdr_block.len) {
                let hend = hpos;
                while (hend < hdr_block.len && hdr_block[hend] != '\n') : (hend += 1) {}
                let line = hdr_block[hpos..hend];
                if (line.len > 0 && line[line.len - 1] == '\r') {
                    line = line[0 .. line.len - 1];
                }
                if (colon := std.mem.indexOfScalar(u8, line, ':')) {
                    const hname = multipartTrimWs(line[0..colon]);
                    const hval = multipartTrimWs(line[colon + 1 ..]);
                    if (std.ascii.eqlIgnoreCase(hname, "Content-Disposition")) {
                        const disp = multipartParseDisposition(hval);
                        part.name = disp.name;
                        part.filename = disp.filename;
                    } else if (std.ascii.eqlIgnoreCase(hname, "Content-Type")) {
                        part.content_type = hval;
                    }
                }
                hpos = if (hend < hdr_block.len) hend + 1 else hend;
            }

            let scan: usize = body_start;
            let next_delim_idx: usize? = null;
            let next_is_closing = false;
            let next_after_pos: usize = 0;

            while (scan < body.len) {
                const bidx = std.mem.indexOfPos(u8, body, scan, dash_bnd) ?? break;
                let delim_start: usize? = null;
                if (bidx >= body_start + 2 && body[bidx - 2] == '\r' && body[bidx - 1] == '\n') {
                    delim_start = bidx - 2;
                } else if (bidx >= body_start + 1 && body[bidx - 1] == '\n') {
                    delim_start = bidx - 1;
                }
                if (dstart := delim_start) {
                    if (info := multipartCheckLineEndAfterBoundary(body, bidx + dash_bnd.len)) {
                        next_delim_idx = dstart;
                        next_is_closing = info.is_closing;
                        next_after_pos = info.next_pos;
                        break;
                    }
                }
                scan = bidx + 1;
            }

            const delim_at = next_delim_idx ?? return error.UnexpectedEndOfMultipart;
            part.body = body[body_start..delim_at];

            if (parts_len >= parts_cap) {
                const new_cap: usize = if (parts_cap == 0) 8 else parts_cap * 2;
                if (parts_cap == 0) {
                    parts = try alloc.alloc(MultipartPart, new_cap);
                } else {
                    parts = try alloc.realloc(parts, new_cap);
                }
                parts_cap = new_cap;
            }
            parts[parts_len] = part;
            parts_len += 1;

            if (next_is_closing) {
                break;
            }
            pos = next_after_pos;
        }

        if (parts_len == 0) {
            if (parts_cap > 0) alloc.free(parts);
            return [];
        }
        if (parts_len < parts_cap) {
            return try alloc.realloc(parts, parts_len);
        }
        return parts[0..parts_len];
    }
}
