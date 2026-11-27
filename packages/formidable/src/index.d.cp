// Type definitions for CP

import { isAlnum, strEq, eqIgnoreCase, startsWith, findByte } from "cap:ascii";
import { ParsedMediaType } from "../../mediatype/src/index.cp";
import { ContentDisposition, parseContentDisposition } from "../../content-disposition/src/index.cp";
export fn frmIsOws(b: u8): bool;
export fn frmIsTokenChar(b: u8): bool;
export fn frmTrimOws(s: string): string;
export fn frmSkipOws(s: string, start: usize): usize;
export fn frmIsBoundaryTerminator(body: string, after_pos: usize): bool;
export fn frmFindFirstBoundary(body: string, boundary: string): usize;
export fn frmFindNextBoundary(body: string, start: usize, boundary: string): usize;
export fn frmExtractDispositionName(header: string): string;
export fn frmHasFilenameParam(header: string): bool;
export class FormidableOptions {
    max_files: usize,
    max_fields: usize,
    max_file_size: u64,
    allow_empty_files: bool,
    fn defaults(): FormidableOptions;
}

export class FormField {
    name: string,
    value: string,
}

export class FormFile {
    field_name: string,
    original_filename: string,
    mime_type: string,
    body_start: usize,
    body_len: usize,
}

export class ParsedForm {
    fields: [16]FormField,
    field_count: usize,
    files: [16]FormFile,
    file_count: usize,
    valid: bool,
    err_code: u8,
    fn empty(): ParsedForm;
    fn fail(code: u8): ParsedForm;
    fn getField(name: string): string?;
    fn getFile(field_name: string): FormFile?;
}

export fn parseMultipartForm(content_type: string, body: string, opts: FormidableOptions): ParsedForm;
