// Type definitions for CP

export fn nidStepState(state: u64): u64;
export fn nidBitmask(alphabet_len: usize): u8;
export fn nidContainsByte(alphabet: string, ch: u8): bool;
export fn nanoidFromBytes(bytes: []u8, size: usize, out: []u8): usize;
export fn customAlphabetFromBytes(alphabet: string, bytes: []u8, size: usize, out: []u8): usize;
export class NanoIdGenerator {
    state: u64,
    fn new(seed: u64): NanoIdGenerator;
    fn nextInto(_: auto, size: usize, out: []u8): usize;
    fn nextCustomInto(_: auto, alphabet: string, size: usize, out: []u8): usize;
}

export fn nanoidSeeded(seed: u64, size: usize, out: []u8): usize;
export fn isValidNanoId(id: string, alphabet: string): bool;
