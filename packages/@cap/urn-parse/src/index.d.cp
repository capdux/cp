// Type definitions for CP

export class Urn {
    nid: string,
    nss: string,
    r_component: string,
    q_component: string,
    f_component: string,
    valid: bool,
}

export fn invalidUrn(): Urn;
export fn isSpace(b: u8): bool;
export fn isAlphaNum(b: u8): bool;
export fn toLowerAscii(b: u8): u8;
export fn eqIgnoreCase(a: string, b: string): bool;
export fn strEq(a: string, b: string): bool;
export fn isValidNid(nid: string): bool;
export fn parseUrn(input: string): Urn;
export fn urnEquivalent(a: Urn, b: Urn): bool;
export fn pushSlice(dst: []u8, pos: usize, src: string): i64;
export fn pushLowerSlice(dst: []u8, pos: usize, src: string): i64;
export fn formatUrn(u: Urn, out: []u8): usize;
