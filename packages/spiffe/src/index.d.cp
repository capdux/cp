// Type definitions for CP

export class SpiffeId {
    trust_domain: string,
    path: string,
    valid: bool,
}

export fn sidEqBytes(a: &[]u8, b: &[]u8): bool;
export fn sidIsTrustDomainChar(c: u8): bool;
export fn sidIsPathSegmentChar(c: u8): bool;
export fn isValidTrustDomain(td: string): bool;
export fn isValidPath(path: string): bool;
export fn parseSpiffeId(uri: string): SpiffeId;
export fn belongsToTrustDomain(id: SpiffeId, trust_domain: string): bool;
export fn formatSpiffeId(trust_domain: string, path: string, out: []u8): usize;
