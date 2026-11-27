// Type definitions for CP

export fn idempIsOws(c: u8): bool;
export fn idempTrimOws(s: &[]u8): &[]u8;
export fn idempIsVisibleAscii(c: u8): bool;
export fn idempIsHex(c: u8): bool;
export fn isValidKey(key: string): bool;
export fn parseKeyHeader(header: string, out: []u8): usize;
export fn formatKeyHeader(key: string, out: []u8): usize;
export fn isUuidV4Like(key: string): bool;
export fn idempMulWrap64(a: u64, b: u64): u64;
export fn idempFnvUpdate(h: u64, data: &[]u8): u64;
export fn idempFnvByte(h: u64, b: u8): u64;
export fn fingerprintRequest(method: string, path: string, body: &[]u8): u64;
export fn evaluateState(exists: bool, in_flight: bool, stored_fp: u64, req_fp: u64): i32;
