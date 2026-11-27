// Type definitions for CP

export fn sstIsSpace(b: u8): bool;
export fn sstTrim(s: string): string;
export fn sstStrEq(a: string, b: string): bool;
export fn parseSpaceTokens(input: string, out: []string): usize;
export fn hasSpaceToken(input: string, target: string): bool;
export fn stringifySpaceTokens(tokens: &[]string, out: []u8): usize;
export fn parseCommaTokens(input: string, out: []string): usize;
export fn stringifyCommaTokens(tokens: &[]string, out: []u8): usize;
