// Type definitions for CP

export class PhcParam {
    key: string,
    value: string,
}

export class PhcHash {
    algorithm: string,
    version: u32,
    has_version: bool,
    params: [8]PhcParam,
    param_count: usize,
    salt: string,
    hash: string,
    valid: bool,
}

export fn phchEmptyHash(): PhcHash;
export fn phchIsLowerAlnumHyphen(c: u8): bool;
export fn phchIsParamValueChar(c: u8): bool;
export fn phchIsB64Char(c: u8): bool;
export fn phchIsValidAlgo(s: string): bool;
export fn phchIsValidParamKey(s: string): bool;
export fn phchIsValidParamValue(s: string): bool;
export fn phchIsValidSalt(s: string): bool;
export fn phchIsValidHash(s: string): bool;
export fn phchContainsByte(s: string, target: u8): bool;
export fn phchParseU32(s: string): i64;
export fn phchStrEq(a: string, b: string): bool;
export fn phchAppendBytes(out: []u8, pos: usize, src: string): usize;
export fn phchAppendU32(out: []u8, pos: usize, val: u32): usize;
export fn parsePhc(phc_str: string): PhcHash;
export fn formatPhc(h: PhcHash, out: []u8): usize;
export fn getPhcParam(h: PhcHash, key: string): string?;
export fn ctEqual(a: string, b: string): bool;
