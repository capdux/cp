// Type definitions for CP

export class ErrorField {
    key: string,
    value: string,
    line_mode: bool,
}

export class StructuredError {
    name: string,
    code: i32,
    message: string,
}

export fn erexWriteByte(out: []u8, pos: usize, b: u8): usize;
export fn erexWriteStr(out: []u8, pos: usize, s: string): usize;
export fn erexWriteI32(out: []u8, pos: usize, val: i32): usize;
export fn formatStructuredError(e: StructuredError, fields: &[]ErrorField, out: []u8): usize;
export fn erexMatchesPlaceholder(tmpl: string, start: usize, key: string): bool;
export fn formatErrorTemplate(tmpl: string, key: string, val: string, out: []u8): usize;
