// Type definitions for CP

export fn isSpace(b: u8): bool;
export fn toLowerAscii(b: u8): u8;
export fn isB64TokenCore(b: u8): bool;
export fn isValidTokenChars(token: string): bool;
export fn extractBearer(header: string): string;
export fn formatBearer(token: string, out: []u8): usize;
export fn isBase64UrlChar(b: u8): bool;
export fn isJwtLike(token: string): bool;
export fn constantTimeEq(a: string, b: string): bool;
export fn verifyBearer(header: string, expected_token: string): bool;
export fn pushSlice(dst: []u8, pos: usize, src: string): i64;
export fn pushQuotedParam(dst: []u8, pos: usize, first: bool, key: string, val: string): i64;
export fn formatChallenge(realm: string, err_code: string, err_desc: string, out: []u8): usize;
