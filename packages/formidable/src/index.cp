// @cap/formidable — Pure CP zero-allocation multipart/form-data form & file upload parser.
// Clean-room MIT implementation inspired by npm `formidable` v3.
// Reuses `@cap/mediatype` for Content-Type/boundary parsing and `@cap/content-disposition`
// for Content-Disposition header parsing.

import { ParsedMediaType } from "../../mediatype/src/index.cp";
import { ContentDisposition, parseContentDisposition } from "../../content-disposition/src/index.cp";

pub const FORMIDABLE_MAX_FIELDS: usize = 16;
pub const FORMIDABLE_MAX_FILES: usize = 16;

pub const FORM_OK: u8 = 0;
pub const ERR_INVALID_CONTENT_TYPE: u8 = 1;
pub const ERR_MALFORMED_MULTIPART: u8 = 2;
pub const ERR_MAX_FIELDS_EXCEEDED: u8 = 3;
pub const ERR_MAX_FILES_EXCEEDED: u8 = 4;
pub const ERR_MAX_FILE_SIZE_EXCEEDED: u8 = 5;
pub const ERR_EMPTY_FILE_NOT_ALLOWED: u8 = 6;

fn frmToLower(b: u8): u8 {
    if (b >= 65 && b <= 90) {
        return b + 32;
    }
    return b;
}

fn frmIsOws(b: u8): bool {
    return b == 32 || b == 9;
}

fn frmIsTokenChar(b: u8): bool {
    if (b >= 48 && b <= 57) { return true; }
    if (b >= 65 && b <= 90) { return true; }
    if (b >= 97 && b <= 122) { return true; }
    if (b == 33 || b == 35 || b == 36 || b == 37 || b == 38 ||
        b == 39 || b == 42 || b == 43 || b == 45 || b == 46 ||
        b == 94 || b == 95 || b == 96 || b == 124 || b == 126) {
        return true;
    }
    return false;
}

fn frmStrEq(a: string, b: string): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i = i + 1;
    }
    return true;
}

fn frmEqIgnoreCase(a: string, b: string): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (frmToLower(a[i]) != frmToLower(b[i])) {
            return false;
        }
        i = i + 1;
    }
    return true;
}

fn frmTrimOws(s: string): string {
    let start: usize = 0;
    let end: usize = s.len;
    while (start < end && frmIsOws(s[start])) {
        start = start + 1;
    }
    while (end > start && frmIsOws(s[end - 1])) {
        end = end - 1;
    }
    return s[start..end];
}

fn frmSkipOws(s: string, start: usize): usize {
    let i = start;
    while (i < s.len && frmIsOws(s[i])) {
        i = i + 1;
    }
    return i;
}

fn frmMatchesAt(hay: string, pos: usize, needle: string): bool {
    if (pos + needle.len > hay.len) {
        return false;
    }
    let i: usize = 0;
    while (i < needle.len) {
        if (hay[pos + i] != needle[i]) {
            return false;
        }
        i = i + 1;
    }
    return true;
}

fn frmIsBoundaryTerminator(body: string, after_pos: usize): bool {
    if (after_pos >= body.len) {
        return true;
    }
    let b = body[after_pos];
    if (b == 13 || b == 10 || b == 32 || b == 9) {
        return true;
    }
    if (b == 45 && after_pos + 1 < body.len && body[after_pos + 1] == 45) {
        return true;
    }
    return false;
}

// Finds the position of `--<boundary>` starting at line start (pos 0 or after LF).
// Returns index of the first `-`, or `body.len` if not found.
fn frmFindFirstBoundary(body: string, boundary: string): usize {
    let i: usize = 0;
    while (i + 2 + boundary.len <= body.len) {
        let at_line_start = (i == 0) || (body[i - 1] == 10);
        if (at_line_start && body[i] == 45 && body[i + 1] == 45) {
            if (frmMatchesAt(body, i + 2, boundary)) {
                if (frmIsBoundaryTerminator(body, i + 2 + boundary.len)) {
                    return i;
                }
            }
        }
        i = i + 1;
    }
    return body.len;
}

// Finds the next `\r\n--<boundary>` or `\n--<boundary>` starting at or after `start`.
// Returns the index where the preceding CRLF/LF begins (i.e. part content end),
// or `body.len` if not found.
fn frmFindNextBoundary(body: string, start: usize, boundary: string): usize {
    let i = start;
    while (i < body.len) {
        if (body[i] == 13 && i + 1 < body.len && body[i + 1] == 10) {
            let dash = i + 2;
            if (dash + 2 + boundary.len <= body.len && body[dash] == 45 && body[dash + 1] == 45) {
                if (frmMatchesAt(body, dash + 2, boundary)) {
                    if (frmIsBoundaryTerminator(body, dash + 2 + boundary.len)) {
                        return i;
                    }
                }
            }
        } else if (body[i] == 10) {
            let dash = i + 1;
            if (dash + 2 + boundary.len <= body.len && body[dash] == 45 && body[dash + 1] == 45) {
                if (frmMatchesAt(body, dash + 2, boundary)) {
                    if (frmIsBoundaryTerminator(body, dash + 2 + boundary.len)) {
                        return i;
                    }
                }
            }
        }
        i = i + 1;
    }
    return body.len;
}

// Extracts the `name` parameter value from a Content-Disposition header value.
fn frmExtractDispositionName(header: string): string {
    let i = frmSkipOws(header, 0);
    while (i < header.len && frmIsTokenChar(header[i])) {
        i = i + 1;
    }
    i = frmSkipOws(header, i);
    while (i < header.len) {
        if (header[i] != 59) { // ';'
            break;
        }
        i = frmSkipOws(header, i + 1);
        if (i >= header.len) {
            break;
        }
        let key_start = i;
        while (i < header.len && frmIsTokenChar(header[i])) {
            i = i + 1;
        }
        let key = header[key_start..i];
        i = frmSkipOws(header, i);
        if (i >= header.len || header[i] != 61) { // '='
            break;
        }
        i = frmSkipOws(header, i + 1);
        if (i >= header.len) {
            break;
        }
        let val: string = "";
        if (header[i] == 34) { // '"'
            i = i + 1;
            let val_start = i;
            while (i < header.len) {
                let ch = header[i];
                if (ch == 92 && i + 1 < header.len) { // '\'
                    i = i + 2;
                    continue;
                }
                if (ch == 34) {
                    val = header[val_start..i];
                    i = i + 1;
                    break;
                }
                i = i + 1;
            }
        } else {
            let val_start = i;
            while (i < header.len && header[i] != 59 && !frmIsOws(header[i])) {
                i = i + 1;
            }
            val = header[val_start..i];
        }
        if (frmEqIgnoreCase(key, "name")) {
            return val;
        }
        i = frmSkipOws(header, i);
    }
    return "";
}

// Checks whether a `filename` or `filename*` parameter is present (even if empty, e.g. `filename=""`).
fn frmHasFilenameParam(header: string): bool {
    let i = frmSkipOws(header, 0);
    while (i < header.len && frmIsTokenChar(header[i])) {
        i = i + 1;
    }
    i = frmSkipOws(header, i);
    while (i < header.len) {
        if (header[i] != 59) {
            break;
        }
        i = frmSkipOws(header, i + 1);
        if (i >= header.len) {
            break;
        }
        let key_start = i;
        while (i < header.len && frmIsTokenChar(header[i])) {
            i = i + 1;
        }
        let key = header[key_start..i];
        i = frmSkipOws(header, i);
        if (i >= header.len || header[i] != 61) {
            break;
        }
        i = frmSkipOws(header, i + 1);
        if (i >= header.len) {
            break;
        }
        if (header[i] == 34) {
            i = i + 1;
            while (i < header.len) {
                let ch = header[i];
                if (ch == 92 && i + 1 < header.len) {
                    i = i + 2;
                    continue;
                }
                if (ch == 34) {
                    i = i + 1;
                    break;
                }
                i = i + 1;
            }
        } else {
            while (i < header.len && header[i] != 59 && !frmIsOws(header[i])) {
                i = i + 1;
            }
        }
        if (frmEqIgnoreCase(key, "filename") || frmEqIgnoreCase(key, "filename*")) {
            return true;
        }
        i = frmSkipOws(header, i);
    }
    return false;
}

/// Configuration options for multipart form parsing (inspired by `formidable` v3).
pub struct FormidableOptions {
    pub max_files: usize,
    pub max_fields: usize,
    pub max_file_size: u64,
    pub allow_empty_files: bool,

    pub static fn defaults(): FormidableOptions {
        return FormidableOptions {
            max_files: FORMIDABLE_MAX_FILES,
            max_fields: FORMIDABLE_MAX_FIELDS,
            max_file_size: 10485760 as u64,
            allow_empty_files: false
        };
    }
}

/// A parsed non-file form field (zero-copy slice into the request body).
pub struct FormField {
    pub name: string,
    pub value: string,
}

/// A parsed uploaded file part (zero-copy offset and length into the request body).
pub struct FormFile {
    pub field_name: string,
    pub original_filename: string,
    pub mime_type: string,
    pub body_start: usize,
    pub body_len: usize,
}

/// Result of parsing a `multipart/form-data` payload into fixed-capacity zero-heap slots.
pub struct ParsedForm {
    pub fields: [16]FormField,
    pub field_count: usize,
    pub files: [16]FormFile,
    pub file_count: usize,
    pub valid: bool,
    pub err_code: u8,

    pub static fn empty(): ParsedForm {
        let blank_field = FormField { name: "", value: "" };
        let field_arr: [16]FormField = [blank_field; 16];
        let blank_file = FormFile {
            field_name: "",
            original_filename: "",
            mime_type: "",
            body_start: 0,
            body_len: 0
        };
        let file_arr: [16]FormFile = [blank_file; 16];
        return ParsedForm {
            fields: field_arr,
            field_count: 0,
            files: file_arr,
            file_count: 0,
            valid: true,
            err_code: FORM_OK
        };
    }

    pub static fn fail(code: u8): ParsedForm {
        let res = ParsedForm.empty();
        res.valid = false;
        res.err_code = code;
        return res;
    }

    /// Returns the first field value matching `name`, or `null` if not found.
    pub fn getField(name: string): string? {
        let i: usize = 0;
        while (i < _.field_count) {
            if (frmStrEq(_.fields[i].name, name)) {
                return _.fields[i].value;
            }
            i = i + 1;
        }
        return null;
    }

    /// Returns the first uploaded file matching `field_name`, or `null` if not found.
    pub fn getFile(field_name: string): FormFile? {
        let i: usize = 0;
        while (i < _.file_count) {
            if (frmStrEq(_.files[i].field_name, field_name)) {
                return _.files[i];
            }
            i = i + 1;
        }
        return null;
    }
}

/// Parses a `multipart/form-data` request body using the `Content-Type` header and options.
/// Reuses `@cap/mediatype` to validate `multipart/form-data` and extract the boundary,
/// and `@cap/content-disposition` to parse each part's `Content-Disposition` header.
pub fn parseMultipartForm(content_type: string, body: string, opts: FormidableOptions): ParsedForm {
    let mt = ParsedMediaType.parse(content_type);
    if (!mt.valid) {
        return ParsedForm.fail(ERR_INVALID_CONTENT_TYPE);
    }
    if (!frmEqIgnoreCase(mt.top_level, "multipart") || !frmEqIgnoreCase(mt.sub_level, "form-data")) {
        return ParsedForm.fail(ERR_INVALID_CONTENT_TYPE);
    }
    let boundary_opt = mt.getParam("boundary");
    if (boundary_opt == null) {
        return ParsedForm.fail(ERR_INVALID_CONTENT_TYPE);
    }
    let boundary = boundary_opt.?;
    if (boundary.len == 0 || boundary.len > 70) {
        return ParsedForm.fail(ERR_INVALID_CONTENT_TYPE);
    }

    let first_dash = frmFindFirstBoundary(body, boundary);
    if (first_dash >= body.len) {
        return ParsedForm.fail(ERR_MALFORMED_MULTIPART);
    }

    let form = ParsedForm.empty();
    let pos = first_dash + 2 + boundary.len;

    // Immediate closing boundary `--boundary--` represents an empty multipart form.
    if (pos + 2 <= body.len && body[pos] == 45 && body[pos + 1] == 45) {
        return form;
    }

    // Consume optional OWS and line break after opening boundary.
    pos = frmSkipOws(body, pos);
    if (pos + 1 < body.len && body[pos] == 13 && body[pos + 1] == 10) {
        pos = pos + 2;
    } else if (pos < body.len && body[pos] == 10) {
        pos = pos + 1;
    } else {
        return ParsedForm.fail(ERR_MALFORMED_MULTIPART);
    }

    let closed = false;
    while (pos < body.len) {
        let part_end = frmFindNextBoundary(body, pos, boundary);
        if (part_end >= body.len) {
            return ParsedForm.fail(ERR_MALFORMED_MULTIPART);
        }

        // Locate header/body separator (`\r\n\r\n` or `\n\n`) within `pos..part_end`.
        let hdr_end: usize = part_end;
        let part_body_start: usize = part_end;
        let scan = pos;
        let found_sep = false;
        while (scan < part_end) {
            if (body[scan] == 13 && scan + 3 < part_end &&
                body[scan + 1] == 10 && body[scan + 2] == 13 && body[scan + 3] == 10) {
                hdr_end = scan;
                part_body_start = scan + 4;
                found_sep = true;
                break;
            }
            if (body[scan] == 10 && scan + 1 < part_end && body[scan + 1] == 10) {
                hdr_end = scan;
                part_body_start = scan + 2;
                found_sep = true;
                break;
            }
            scan = scan + 1;
        }
        if (!found_sep) {
            return ParsedForm.fail(ERR_MALFORMED_MULTIPART);
        }

        // Parse part headers line by line.
        let has_cd = false;
        let field_name: string = "";
        let is_file = false;
        let orig_filename: string = "";
        let part_mime: string = "";

        let hpos = pos;
        while (hpos < hdr_end) {
            let line_end = hpos;
            while (line_end < hdr_end && body[line_end] != 10 && body[line_end] != 13) {
                line_end = line_end + 1;
            }
            let line = body[hpos..line_end];

            // Advance hpos past line break.
            hpos = line_end;
            if (hpos < hdr_end && body[hpos] == 13) {
                hpos = hpos + 1;
            }
            if (hpos < hdr_end && body[hpos] == 10) {
                hpos = hpos + 1;
            }

            // Split header line on ':'.
            let colon: usize = 0;
            let has_colon = false;
            while (colon < line.len) {
                if (line[colon] == 58) { // ':'
                    has_colon = true;
                    break;
                }
                colon = colon + 1;
            }
            if (!has_colon || colon == 0) {
                return ParsedForm.fail(ERR_MALFORMED_MULTIPART);
            }

            let hname = frmTrimOws(line[0..colon]);
            let hval = frmTrimOws(line[(colon + 1)..line.len]);

            if (frmEqIgnoreCase(hname, "content-disposition")) {
                let cd: ContentDisposition = parseContentDisposition(hval);
                if (!cd.valid || !frmEqIgnoreCase(cd.disp_type, "form-data")) {
                    return ParsedForm.fail(ERR_MALFORMED_MULTIPART);
                }
                has_cd = true;
                field_name = frmExtractDispositionName(hval);
                if (cd.hasFilename() || frmHasFilenameParam(hval)) {
                    is_file = true;
                    if (cd.filename.len > 0) {
                        orig_filename = cd.filename;
                    } else if (cd.filename_star.len > 0) {
                        orig_filename = cd.filename_star;
                    } else {
                        orig_filename = "";
                    }
                }
            } else if (frmEqIgnoreCase(hname, "content-type")) {
                part_mime = hval;
            }
        }

        if (!has_cd || field_name.len == 0) {
            return ParsedForm.fail(ERR_MALFORMED_MULTIPART);
        }

        let part_body_len = part_end - part_body_start;

        if (is_file) {
            if (!opts.allow_empty_files && part_body_len == 0) {
                return ParsedForm.fail(ERR_EMPTY_FILE_NOT_ALLOWED);
            }
            if ((part_body_len as u64) > opts.max_file_size) {
                return ParsedForm.fail(ERR_MAX_FILE_SIZE_EXCEEDED);
            }
            if (form.file_count >= opts.max_files || form.file_count >= FORMIDABLE_MAX_FILES) {
                return ParsedForm.fail(ERR_MAX_FILES_EXCEEDED);
            }
            let effective_mime = part_mime;
            if (effective_mime.len == 0) {
                effective_mime = "application/octet-stream";
            }
            form.files[form.file_count] = FormFile {
                field_name: field_name,
                original_filename: orig_filename,
                mime_type: effective_mime,
                body_start: part_body_start,
                body_len: part_body_len
            };
            form.file_count = form.file_count + 1;
        } else {
            if (form.field_count >= opts.max_fields || form.field_count >= FORMIDABLE_MAX_FIELDS) {
                return ParsedForm.fail(ERR_MAX_FIELDS_EXCEEDED);
            }
            form.fields[form.field_count] = FormField {
                name: field_name,
                value: body[part_body_start..part_end]
            };
            form.field_count = form.field_count + 1;
        }

        // Move past the `\r\n` or `\n` and `--<boundary>`.
        let next_pos = part_end;
        if (next_pos + 1 < body.len && body[next_pos] == 13 && body[next_pos + 1] == 10) {
            next_pos = next_pos + 2;
        } else if (next_pos < body.len && body[next_pos] == 10) {
            next_pos = next_pos + 1;
        }
        next_pos = next_pos + 2 + boundary.len;

        if (next_pos + 2 <= body.len && body[next_pos] == 45 && body[next_pos + 1] == 45) {
            closed = true;
            break;
        }

        next_pos = frmSkipOws(body, next_pos);
        if (next_pos + 1 < body.len && body[next_pos] == 13 && body[next_pos + 1] == 10) {
            pos = next_pos + 2;
        } else if (next_pos < body.len && body[next_pos] == 10) {
            pos = next_pos + 1;
        } else {
            return ParsedForm.fail(ERR_MALFORMED_MULTIPART);
        }
    }

    if (!closed) {
        return ParsedForm.fail(ERR_MALFORMED_MULTIPART);
    }

    return form;
}
