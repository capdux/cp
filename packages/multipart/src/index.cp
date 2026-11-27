// @cap/multipart — Zero-heap RFC 7578 multipart/form-data codec.
// Clean-room MIT implementation modeled on crates.io `multipart` / `multer`,
// reusing `@cap/mediatype` for boundary extraction and `@cap/unicase` for case-insensitive headers.

import { trim } from "cap:ascii";
import { writeStr } from "cap:fmt";
import { ParsedMediaType } from "../../mediatype/src/index.cp";
import { eqAscii } from "../../unicase/src/index.cp";

/// A single decoded part from a `multipart/form-data` payload (zero-copy slices into body).
pub struct MultipartPart {
    pub name: string,
    pub filename: string,
    pub content_type: string,
    pub data: string,
    pub is_file: bool,
}

fn mpartIsSpace(c: u8): bool {
    return c == 32 || c == 9;
}

fn mpartMatchAt(haystack: string, pos: usize, needle: string): bool {
    if (pos + needle.len > haystack.len) { return false; }
    let i: usize = 0;
    while (i < needle.len) {
        if (haystack[pos + i] != needle[i]) { return false; }
        i = i + 1;
    }
    return true;
}

fn mpartIsBoundaryAt(body: string, pos: usize, boundary: string): bool {
    if (pos + 2 + boundary.len > body.len) { return false; }
    if (body[pos] != 45 || body[pos + 1] != 45) { return false; }
    if (!mpartMatchAt(body, pos + 2, boundary)) { return false; }
    let after: usize = pos + 2 + boundary.len;
    if (after >= body.len) { return true; }
    let c: u8 = body[after];
    if (c == 45 && after + 1 < body.len && body[after + 1] == 45) { return true; }
    if (c == 13 || c == 10 || c == 32 || c == 9) { return true; }
    return false;
}

fn mpartFindFirstBoundary(body: string, boundary: string): usize {
    if (mpartIsBoundaryAt(body, 0, boundary)) { return 0; }
    let i: usize = 0;
    while (i < body.len) {
        if (body[i] == 10) {
            let next: usize = i + 1;
            if (mpartIsBoundaryAt(body, next, boundary)) {
                return next;
            }
        }
        i = i + 1;
    }
    return body.len;
}

fn mpartFindNextBoundary(body: string, start: usize, boundary: string, data_end_out: *usize, next_bnd_out: *usize): bool {
    let i: usize = start;
    while (i < body.len) {
        if (body[i] == 13 && i + 1 < body.len && body[i + 1] == 10) {
            if (mpartIsBoundaryAt(body, i + 2, boundary)) {
                data_end_out.* = i;
                next_bnd_out.* = i + 2;
                return true;
            }
            i = i + 2;
        } else if (body[i] == 10) {
            if (mpartIsBoundaryAt(body, i + 1, boundary)) {
                data_end_out.* = i;
                next_bnd_out.* = i + 1;
                return true;
            }
            i = i + 1;
        } else {
            i = i + 1;
        }
    }
    return false;
}

fn mpartSkipLineEnd(body: string, pos: *usize): bool {
    let p: usize = pos.*;
    while (p < body.len && mpartIsSpace(body[p])) {
        p = p + 1;
    }
    if (p + 1 < body.len && body[p] == 13 && body[p + 1] == 10) {
        pos.* = p + 2;
        return true;
    }
    if (p < body.len && body[p] == 10) {
        pos.* = p + 1;
        return true;
    }
    return false;
}

fn mpartParseDisposition(val: string, part: *MultipartPart) {
    let pos: usize = 0;
    // Skip disposition type (e.g. "form-data") up to first ';'
    while (pos < val.len && val[pos] != 59) {
        pos = pos + 1;
    }
    while (pos < val.len) {
        if (val[pos] == 59) { pos = pos + 1; }
        let seg_start: usize = pos;
        let in_quotes: bool = false;
        while (pos < val.len) {
            let c: u8 = val[pos];
            if (c == 34) {
                in_quotes = !in_quotes;
            } else if (c == 59 && !in_quotes) {
                break;
            }
            pos = pos + 1;
        }
        let seg: string = trim(val[seg_start..pos]);
        if (seg.len == 0) { continue; }
        let eq_pos: usize = 0;
        let found_eq: bool = false;
        while (eq_pos < seg.len) {
            if (seg[eq_pos] == 61) {
                found_eq = true;
                break;
            }
            eq_pos = eq_pos + 1;
        }
        if (!found_eq || eq_pos == 0) { continue; }
        let pk: string = trim(seg[0..eq_pos]);
        let pv: string = trim(seg[(eq_pos + 1)..seg.len]);
        if (pv.len >= 2 && pv[0] == 34 && pv[pv.len - 1] == 34) {
            pv = pv[1..(pv.len - 1)];
        }
        if (eqAscii(pk, "name")) {
            part.*.name = pv;
        } else if (eqAscii(pk, "filename")) {
            part.*.filename = pv;
            part.*.is_file = true;
        }
    }
}

/// Extracts the `boundary` parameter from a `multipart/*` Content-Type header using `@cap/mediatype`.
/// Returns `null` if the media type is invalid, not `multipart/*`, or has an empty/oversized boundary.
pub fn extractBoundary(content_type: string): string? {
    let mt: ParsedMediaType = ParsedMediaType.parse(content_type);
    if (!mt.valid) { return null; }
    if (!eqAscii(mt.top_level, "multipart")) { return null; }
    let maybe = mt.getParam("boundary");
    if (maybe == null) { return null; }
    let b: string = maybe ?? "";
    if (b.len == 0 || b.len > 70) { return null; }
    return b;
}

/// Formats `multipart/form-data; boundary=<boundary>` into `out`, returning bytes written.
pub fn formatContentTypeHeader(boundary: string, out: []u8): usize {
    if (boundary.len == 0) { return 0; }
    let p: usize = writeStr("multipart/form-data; boundary=", out, 0);
    p = writeStr(boundary, out, p);
    return p;
}

/// Zero-heap streaming cursor reader for RFC 7578 `multipart/form-data` payloads.
pub struct MultipartReader {
    pub boundary: string,
    pub body: string,
    pub cursor: usize,
    pub finished: bool,

    /// Creates a new reader over `body` using the given `boundary`.
    pub static fn new(boundary: string, body: string): MultipartReader {
        return MultipartReader {
            boundary: boundary,
            body: body,
            cursor: 0,
            finished: boundary.len == 0 || body.len == 0,
        };
    }

    /// Advances the cursor and returns the next `MultipartPart`, or `null` at end / malformed input.
    pub fn nextPart(*_): MultipartPart? {
        if (_.finished) { return null; }

        let pos: usize = _.cursor;
        if (pos == 0) {
            pos = mpartFindFirstBoundary(_.body, _.boundary);
            if (pos >= _.body.len) {
                _.finished = true;
                return null;
            }
        } else {
            if (!mpartIsBoundaryAt(_.body, pos, _.boundary)) {
                _.finished = true;
                return null;
            }
        }

        // Move past "--<boundary>"
        pos = pos + 2 + _.boundary.len;
        // Check closing delimiter "--"
        if (pos + 1 < _.body.len && _.body[pos] == 45 && _.body[pos + 1] == 45) {
            _.finished = true;
            _.cursor = pos + 2;
            return null;
        }
        if (!mpartSkipLineEnd(_.body, &pos)) {
            _.finished = true;
            return null;
        }

        let part: MultipartPart = MultipartPart {
            name: "",
            filename: "",
            content_type: "",
            data: "",
            is_file: false,
        };

        // Parse part headers until blank line
        let headers_done: bool = false;
        while (!headers_done) {
            if (pos >= _.body.len) {
                _.finished = true;
                return null;
            }
            if (pos + 1 < _.body.len && _.body[pos] == 13 && _.body[pos + 1] == 10) {
                pos = pos + 2;
                headers_done = true;
            } else if (_.body[pos] == 10) {
                pos = pos + 1;
                headers_done = true;
            } else {
                let line_start: usize = pos;
                while (pos < _.body.len && _.body[pos] != 10 && _.body[pos] != 13) {
                    pos = pos + 1;
                }
                let line: string = _.body[line_start..pos];
                if (!mpartSkipLineEnd(_.body, &pos)) {
                    _.finished = true;
                    return null;
                }
                let colon: usize = 0;
                let has_colon: bool = false;
                while (colon < line.len) {
                    if (line[colon] == 58) {
                        has_colon = true;
                        break;
                    }
                    colon = colon + 1;
                }
                if (has_colon) {
                    let hname: string = trim(line[0..colon]);
                    let hval: string = trim(line[(colon + 1)..line.len]);
                    if (eqAscii(hname, "Content-Disposition")) {
                        mpartParseDisposition(hval, &part);
                    } else if (eqAscii(hname, "Content-Type")) {
                        part.content_type = hval;
                    }
                }
            }
        }

        let data_start: usize = pos;
        let data_end: usize = data_start;
        let next_bnd: usize = _.body.len;
        if (!mpartFindNextBoundary(_.body, data_start, _.boundary, &data_end, &next_bnd)) {
            _.finished = true;
            return null;
        }

        _.cursor = next_bnd;
        part.data = _.body[data_start..data_end];
        return part;
    }

    /// Counts remaining parts without mutating this reader's cursor.
    pub fn countParts(): usize {
        let clone: MultipartReader = MultipartReader {
            boundary: _.boundary,
            body: _.body,
            cursor: _.cursor,
            finished: _.finished,
        };
        let total: usize = 0;
        while (clone.nextPart() != null) {
            total = total + 1;
        }
        return total;
    }
}

/// Zero-heap RFC 7578 `multipart/form-data` payload writer into a caller-provided `[]u8` buffer.
pub struct MultipartWriter {
    pub boundary: string,

    /// Creates a new writer with the given `boundary`.
    pub static fn new(boundary: string): MultipartWriter {
        return MultipartWriter { boundary: boundary };
    }

    /// Appends a form text field part at `pos` in `out`, returning the updated byte offset.
    pub fn writeField(name: string, value: string, out: []u8, pos: usize): usize {
        let p: usize = writeStr("--", out, pos);
        p = writeStr(_.boundary, out, p);
        p = writeStr("\r\nContent-Disposition: form-data; name=\"", out, p);
        p = writeStr(name, out, p);
        p = writeStr("\"\r\n\r\n", out, p);
        p = writeStr(value, out, p);
        p = writeStr("\r\n", out, p);
        return p;
    }

    /// Appends a file upload part at `pos` in `out`, returning the updated byte offset.
    pub fn writeFile(name: string, filename: string, content_type: string, data: string, out: []u8, pos: usize): usize {
        let ctype: string = if (content_type.len > 0) { content_type } else { "application/octet-stream" };
        let p: usize = writeStr("--", out, pos);
        p = writeStr(_.boundary, out, p);
        p = writeStr("\r\nContent-Disposition: form-data; name=\"", out, p);
        p = writeStr(name, out, p);
        p = writeStr("\"; filename=\"", out, p);
        p = writeStr(filename, out, p);
        p = writeStr("\"\r\nContent-Type: ", out, p);
        p = writeStr(ctype, out, p);
        p = writeStr("\r\n\r\n", out, p);
        p = writeStr(data, out, p);
        p = writeStr("\r\n", out, p);
        return p;
    }

    /// Appends the closing `--<boundary>--\r\n` delimiter at `pos`, returning the final length.
    pub fn finish(out: []u8, pos: usize): usize {
        let p: usize = writeStr("--", out, pos);
        p = writeStr(_.boundary, out, p);
        p = writeStr("--\r\n", out, p);
        return p;
    }
}
