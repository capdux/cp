// Type definitions for CP

export class NthRule {
    a: i32,
    b: i32,
    valid: bool,
    fn matches(index: usize): bool;
}

export fn ntcIsSpace(b: u8): bool;
export fn ntcToLower(b: u8): u8;
export fn ntcTrim(s: string): string;
export fn ntcEqLower(s: string, target: string): bool;
export fn ntcIsDigit(b: u8): bool;
export fn parseNth(formula: string): NthRule;
export fn nthCheck(formula: string, index: usize): bool;
