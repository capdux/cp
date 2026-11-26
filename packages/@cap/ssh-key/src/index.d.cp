// Type definitions for CP

export class SshPublicKey {
    alg: u8,
    key_type: string,
    key_b64: string,
    comment: string,
    valid: bool,
}

export fn isSpace(c: u8): bool;
export fn isLineTrim(c: u8): bool;
export fn strEq(a: string, b: string): bool;
export fn identifyAlgorithm(key_type: string): u8;
export fn isValidBase64(b64: string): bool;
export fn parsePublicKey(line: string): SshPublicKey;
export fn formatPublicKey(key: SshPublicKey, out: []u8): usize;
export fn countAuthorizedKeys(text: string): usize;
export fn isModernAlgorithm(alg: u8): bool;
