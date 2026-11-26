// Type definitions for CP

export fn utPopcount64(x: u64): u64;
export fn utAllOnes64(): u64;
export class UcdTrie {
    max_cp: u32,
    chunk_count: usize,
    fn new(): UcdTrie;
    fn init(_: auto, level1: []u16, chunks: []u64): bool;
    fn insertCodepoint(_: auto, level1: []u16, chunks: []u64, cp: u32): bool;
    fn insertRange(_: auto, level1: []u16, chunks: []u64, lo: u32, hi: u32): bool;
    fn contains(level1: []u16, chunks: []u64, cp: u32): bool;
    fn countMembers(level1: []u16, chunks: []u64): u64;
    fn dedupChunkCount(): usize;
}

