// Type definitions for CP

export class Utf8Error {
    valid_up_to: usize,
    has_error_len: bool,
    error_len: usize,
}

export class Utf8CompatResult {
    valid: bool,
    valid_up_to: usize,
    has_error_len: bool,
    error_len: usize,
}

export fn sutfIsCont(b: u8): bool;
export fn isAscii(buf: []u8): bool;
export fn validateUtf8Compat(buf: []u8): Utf8CompatResult;
export fn basicIsValidUtf8(buf: []u8): bool;
export fn countCodepoints(buf: []u8): usize?;
