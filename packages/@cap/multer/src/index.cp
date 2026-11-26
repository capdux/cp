// @cap/multer — Pure CP zero-allocation multipart/form-data upload validator.
// Clean-room MIT implementation inspired by npm `multer` v2.
// Reuses `@cap/mediatype` for Content-Type/boundary parsing and `@cap/mime-types` for MIME lookup.

import { ParsedMediaType } from "../../mediatype/src/index.cp";
import { lookup } from "../../mime-types/src/index.cp";

pub const MULTER_MAX_FILES: usize = 16;
pub const MULTER_MAX_ALLOWED_FIELDS: usize = 16;

pub const MULTER_OK: u8 = 0;
pub const ERR_LIMIT_FILE_SIZE: u8 = 1;
pub const ERR_LIMIT_FILE_COUNT: u8 = 2;
pub const ERR_LIMIT_FIELD_COUNT: u8 = 3;
pub const ERR_LIMIT_FIELD_VALUE: u8 = 4;
pub const ERR_UNEXPECTED_FILE: u8 = 5;
pub const ERR_INVALID_CONTENT_TYPE: u8 = 6;

fn mltToLower(b: u8): u8 {
    if (b >= ('A' as u8) && b <= ('Z' as u8)) {
        return b + (32 as u8);
    }
    return b;
}

fn mltStrEq(a: string, b: string): bool {
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

fn mltEqIgnoreCase(a: string, b: string): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (mltToLower(a[i]) != mltToLower(b[i])) {
            return false;
        }
        i = i + 1;
    }
    return true;
}

/// Resource limits enforced on multipart uploads.
pub struct MulterLimits {
    pub file_size: u64,
    pub files: usize,
    pub fields: usize,
    pub field_size: u64,

    pub static fn defaults(): MulterLimits {
        return MulterLimits {
            file_size: 10485760 as u64,
            files: MULTER_MAX_FILES,
            fields: 64,
            field_size: 1048576 as u64
        };
    }
}

/// Metadata recorded for each accepted uploaded file.
pub struct UploadedFile {
    pub field_name: string,
    pub original_name: string,
    pub mime_type: string,
    pub size: u64,
}

/// Validates a `Content-Type` header as `multipart/form-data` via `@cap/mediatype`
/// and returns its non-empty `boundary` parameter, or `null` if invalid.
pub fn extractMultipartBoundary(content_type: string): string? {
    let parsed = ParsedMediaType.parse(content_type);
    if (!parsed.valid) {
        return null;
    }
    if (!mltEqIgnoreCase(parsed.top_level, "multipart")
        || !mltEqIgnoreCase(parsed.sub_level, "form-data"))
    {
        return null;
    }
    let boundary_opt = parsed.getParam("boundary");
    if (boundary_opt == null) {
        return null;
    }
    let boundary = boundary_opt.?;
    if (boundary.len == 0) {
        return null;
    }
    return boundary;
}

/// Zero-heap multipart upload policy validator (up to 16 files and 16 whitelisted field names).
pub struct MulterValidator {
    pub limits: MulterLimits,
    pub files: [16]UploadedFile,
    pub file_count: usize,
    pub field_count: usize,
    pub allowed_fields: [16]string,
    pub allowed_count: usize,

    pub static fn new(limits: MulterLimits): MulterValidator {
        let empty_file = UploadedFile {
            field_name: "",
            original_name: "",
            mime_type: "",
            size: 0 as u64
        };
        return MulterValidator {
            limits: limits,
            files: [empty_file; 16],
            file_count: 0,
            field_count: 0,
            allowed_fields: [""; 16],
            allowed_count: 0
        };
    }

    /// Registers an allowed file upload field name (up to 16 entries).
    pub fn allowField(*_, field_name: string): bool {
        if (field_name.len == 0 || _.allowed_count >= MULTER_MAX_ALLOWED_FIELDS) {
            return false;
        }
        _.allowed_fields[_.allowed_count] = field_name;
        _.allowed_count = _.allowed_count + 1;
        return true;
    }

    /// Returns `true` if `field_name` is permitted (always `true` when no whitelist is configured).
    pub fn isFieldAllowed(field_name: string): bool {
        if (_.allowed_count == 0) {
            return true;
        }
        let i: usize = 0;
        while (i < _.allowed_count) {
            if (mltStrEq(_.allowed_fields[i], field_name)) {
                return true;
            }
            i = i + 1;
        }
        return false;
    }

    /// Validates that `content_type` is `multipart/form-data` with a valid boundary.
    pub fn validateContentType(content_type: string): u8 {
        if (extractMultipartBoundary(content_type) == null) {
            return ERR_INVALID_CONTENT_TYPE;
        }
        return MULTER_OK;
    }

    /// Validates and records an incoming file upload.
    /// When `content_type` is empty, derives MIME type from `filename` via `@cap/mime-types`.
    pub fn acceptFile(
        *_,
        field_name: string,
        filename: string,
        content_type: string,
        size: u64
    ): u8 {
        if (!_.isFieldAllowed(field_name)) {
            return ERR_UNEXPECTED_FILE;
        }
        if (_.file_count >= _.limits.files || _.file_count >= MULTER_MAX_FILES) {
            return ERR_LIMIT_FILE_COUNT;
        }
        if (size > _.limits.file_size) {
            return ERR_LIMIT_FILE_SIZE;
        }

        let resolved_mime: string = content_type;
        if (resolved_mime.len == 0) {
            let inferred = lookup(filename);
            resolved_mime = if (inferred != null) { inferred.? } else { "application/octet-stream" };
        }

        _.files[_.file_count] = UploadedFile {
            field_name: field_name,
            original_name: filename,
            mime_type: resolved_mime,
            size: size
        };
        _.file_count = _.file_count + 1;
        return MULTER_OK;
    }

    /// Validates and counts a non-file text form field.
    pub fn acceptField(*_, value_len: u64): u8 {
        if (_.field_count >= _.limits.fields) {
            return ERR_LIMIT_FIELD_COUNT;
        }
        if (value_len > _.limits.field_size) {
            return ERR_LIMIT_FIELD_VALUE;
        }
        _.field_count = _.field_count + 1;
        return MULTER_OK;
    }

    /// Returns the number of accepted files.
    pub fn fileCount(): usize {
        return _.file_count;
    }

    /// Returns the number of accepted text fields.
    pub fn fieldCount(): usize {
        return _.field_count;
    }
}

export {
    MULTER_MAX_FILES,
    MULTER_MAX_ALLOWED_FIELDS,
    MULTER_OK,
    ERR_LIMIT_FILE_SIZE,
    ERR_LIMIT_FILE_COUNT,
    ERR_LIMIT_FIELD_COUNT,
    ERR_LIMIT_FIELD_VALUE,
    ERR_UNEXPECTED_FILE,
    ERR_INVALID_CONTENT_TYPE,
    MulterLimits,
    UploadedFile,
    extractMultipartBoundary,
    MulterValidator
};
