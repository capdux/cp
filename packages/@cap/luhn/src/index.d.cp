// Type definitions for CP

export fn luhnValidate(input: &[]u8): bool;
export fn luhnCheckDigit(payload: &[]u8): i32;
export fn dammCheckDigit(payload: &[]u8): i32;
export fn dammValidate(input: &[]u8): bool;
export fn verhoeffCheckDigit(payload: &[]u8): i32;
export fn verhoeffValidate(input: &[]u8): bool;
