// Type definitions for CP

import { strEq, eqIgnoreCase } from "cap:ascii";
import { ParsedMediaType } from "../../mediatype/src/index.cp";
import { lookup } from "../../mime-types/src/index.cp";
export class MulterLimits {
    file_size: u64,
    files: usize,
    fields: usize,
    field_size: u64,
    fn defaults(): MulterLimits;
}

export class UploadedFile {
    field_name: string,
    original_name: string,
    mime_type: string,
    size: u64,
}

export fn extractMultipartBoundary(content_type: string): string?;
export class MulterValidator {
    limits: MulterLimits,
    files: [16]UploadedFile,
    file_count: usize,
    field_count: usize,
    allowed_fields: [16]string,
    allowed_count: usize,
    fn new(limits: MulterLimits): MulterValidator;
    fn allowField(_: auto, field_name: string): bool;
    fn isFieldAllowed(field_name: string): bool;
    fn validateContentType(content_type: string): u8;
    fn acceptFile(_: auto, field_name: string, filename: string, content_type: string, size: u64): u8;
    fn acceptField(_: auto, value_len: u64): u8;
    fn fileCount(): usize;
    fn fieldCount(): usize;
}

