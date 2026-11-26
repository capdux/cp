// Type definitions for CP

export enum SriAlgo {
    Sha256,
    Sha384,
    Sha512,
    Blake3,
    Unknown
}

export class SriHash {
    valid: bool,
    algo: SriAlgo,
    algo_name: string,
    digest: string,
    options: string,
}

export class SriIntegrity {
    hashes: [MAX_HASHES]SriHash,
    count: usize,
    fn get(idx: usize): SriHash?;
    fn isEmpty(): bool;
}

export fn emptyHash(): SriHash;
export fn isSpace(c: u8): bool;
export fn toLowerAscii(c: u8): u8;
export fn strEqIgnoreCase(a: string, b: string): bool;
export fn isAlnum(c: u8): bool;
export fn isValidAlgoName(name: string): bool;
export fn parseAlgo(name: string): SriAlgo;
export fn algoName(algo: SriAlgo): string;
export fn algoStrength(algo: SriAlgo): u8;
export fn isBase64DataChar(c: u8): bool;
export fn isValidBase64(s: string): bool;
export fn isValidOptions(opts: string): bool;
export fn constantTimeEq(a: string, b: string): bool;
export fn parseSingleToken(tok: string): SriHash;
export fn parseIntegrity(raw: string): SriIntegrity;
export fn strongestHash(sri: SriIntegrity): SriHash?;
export fn pickAlgorithm(sri: SriIntegrity, algo: SriAlgo): SriHash?;
export fn matchesIntegrity(expected: SriIntegrity, actual: SriIntegrity): bool;
export fn appendStr(out: []u8, pos: usize, s: string): usize;
export fn appendByte(out: []u8, pos: usize, c: u8): usize;
export fn formatHash(algo: SriAlgo, base64_digest: string, out: []u8): string;
export fn formatIntegrity(sri: SriIntegrity, out: []u8): string;
