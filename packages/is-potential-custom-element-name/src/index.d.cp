// Type definitions for CP

export const RESERVED_CUSTOM_ELEMENT_NAMES_COUNT: usize;

export enum PcenError {
    Ok,
    Empty,
    TooShort,
    InvalidStartChar,
    MissingHyphen,
    ReservedName,
    InvalidUtf8,
    InvalidChar,
}

export class PcenValidationResult {
    valid: bool,
    err: PcenError,
    error_offset: usize,
}

export fn isValidCustomElementStart(b: u8): bool;
export fn isPcenChar(cp: u32): bool;
export fn reservedCustomElementNameAt(idx: usize): string?;
export fn isReservedCustomElementName(name: string): bool;
export fn validateCustomElementName(name: string): PcenValidationResult;
export fn isPotentialCustomElementName(name: string): bool;
export fn sanitizeCustomElementName(input: string, out: []u8): usize;
