// Type definitions for CP

export fn ynToLower(b: u8): u8;
export fn ynTrim(s: string): string;
export fn ynEqLower(s: string, target: string): bool;
export fn yn(input: string): bool?;
export fn ynLenient(input: string): bool?;
export fn ynDefault(input: string, default_val: bool): bool;
