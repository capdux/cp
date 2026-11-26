// Type definitions for CP

export fn surToLower(b: u8): u8;
export fn surEqLower(a: string, target: string): bool;
export fn surIsHex(b: u8): bool;
export fn surIsSafeUriByte(b: u8): bool;
export fn surHexDigit(n: u8): u8;
export fn isSafeUri(uri: string): bool;
export fn sanitizeUri(uri: string, out: []u8): usize;
