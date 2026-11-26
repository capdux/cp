// Type definitions for CP

export enum PreKind {
    None,
    Alpha,
    Beta,
    Rc
}

export class Pep440Version {
    valid: bool,
    epoch: u32,
    release: [4]u32,
    release_len: u8,
    pre_kind: PreKind,
    pre_num: u32,
    has_post: bool,
    post_num: u32,
    has_dev: bool,
    dev_num: u32,
}

export fn pepInvalid(): Pep440Version;
export fn pepIsDigit(c: u8): bool;
export fn pepIsSpace(c: u8): bool;
export fn pepToLower(c: u8): u8;
export fn pepIsSep(c: u8): bool;
export fn pepMatchWord(s: string, pos: usize, end: usize, word: string): bool;
export class PepNumRes {
    ok: bool,
    val: u32,
    next: usize,
}

export fn pepReadU32(s: string, pos: usize, end: usize): PepNumRes;
export fn parsePep440(s: string): Pep440Version;
export fn pepCmpU32(a: u32, b: u32): i32;
export fn pepPreRank(k: PreKind): i32;
export fn pepStageRank(v: Pep440Version): i32;
export fn comparePep440(a: Pep440Version, b: Pep440Version): i32;
export fn pepPrefixMatch(v: Pep440Version, target: Pep440Version, prefix_len: u8): bool;
export fn pepMatchSingle(v: Pep440Version, raw: string): bool;
export fn matchesSpecifier(ver: string, spec: string): bool;
