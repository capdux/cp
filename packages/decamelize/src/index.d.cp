// Type definitions for CP

export fn dcIsUpper(c: u8): bool;
export fn dcIsLower(c: u8): bool;
export fn dcIsDigit(c: u8): bool;
export fn dcToLower(c: u8): u8;
export fn dcPushSep(sep: string, out: []u8, pos: usize): usize;
export fn decamelizeWithOptions(input: string, separator: string, preserve_consecutive_uppercase: bool, out: []u8): usize;
export fn decamelize(input: string, separator: string, out: []u8): usize;
export fn toSnakeCase(input: string, out: []u8): usize;
export fn toKebabCase(input: string, out: []u8): usize;
