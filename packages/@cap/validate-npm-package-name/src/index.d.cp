// Type definitions for CP

export class PackageNameValidation {
    valid_for_new: bool,
    valid_for_old: bool,
    error_code: u8,
    warning_code: u8,
}

export fn vnpStrEq(a: string, b: string): bool;
export fn vnpEqIgnoreCase(a: string, b: string): bool;
export fn vnpIsUrlSafeChar(c: u8): bool;
export fn vnpIsSpecialWarnChar(c: u8): bool;
export fn isBlacklistedName(name: string): bool;
export fn isCoreModuleName(name: string): bool;
export fn vnpCheckSegment(seg: string): u8;
export fn validatePackageName(name: string): PackageNameValidation;
export fn isValidNewPackageName(name: string): bool;
