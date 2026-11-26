// Type definitions for CP

export fn splitmix64Next(state: u64): u64;
export fn adjectiveAt(idx: usize): string;
export fn adverbAt(idx: usize): string;
export fn nounAt(idx: usize): string;
export fn appendStr(out: []u8, pos: usize, s: string): usize;
export fn appendHex4(out: []u8, pos: usize, val: u64): usize;
export class Petname {
    state: u64,
    fn init(seed: u64): Petname;
    fn nextU64(_: auto): u64;
    fn generate(_: auto, words: u8, sep: string, out: []u8): string;
    fn generateSlug(_: auto, out: []u8): string;
    fn generateSubdomain(_: auto, out: []u8): string;
}

export fn petnameFromSeed(seed: u64, words: u8, sep: string, out: []u8): string;
export fn subdomainFromSeed(seed: u64, out: []u8): string;
