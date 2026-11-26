// Type definitions for CP

export class Hash32 {
    bytes: [32]u8,
    fn equals(other: Hash32): bool;
    fn toHex(out: []u8): string;
}

export fn hexLower(nibble: u8): u8;
export fn zeroHash(): Hash32;
export fn add32(a: u32, b: u32): u32;
export fn pow2_64(n: u32): u64;
export fn rotr32(x: u32, n: u32): u32;
export fn shr32(x: u32, n: u32): u32;
export fn bitNot32(x: u32): u32;
export fn sha256K(i: usize): u32;
export class Sha256State {
    h: [8]u32,
    buf: [64]u8,
    buf_len: usize,
    total_len: u64,
}

export fn sha256Init(): Sha256State;
export fn sha256Compress(st: &mut Sha256State);
export fn sha256UpdateByte(st: &mut Sha256State, byte: u8);
export fn sha256UpdateSlice(st: &mut Sha256State, data: string);
export fn sha256Finalize(st: &mut Sha256State): Hash32;
export fn sha256(data: string): Hash32;
export fn hashLeaf(data: string): Hash32;
export fn hashNode(left: Hash32, right: Hash32): Hash32;
export class MerkleProof {
    siblings: [MAX_PROOF_DEPTH]Hash32,
    is_right: [MAX_PROOF_DEPTH]bool,
    depth: usize,
}

export fn emptyProof(): MerkleProof;
export class MerkleTree {
    leaves: [MAX_LEAVES]Hash32,
    count: usize,
    fn init(): MerkleTree;
    fn addLeaf(_: auto, data: string): bool;
    fn leafCount(): usize;
    fn root(): Hash32?;
    fn generateProof(leaf_index: usize): MerkleProof?;
}

export fn verifyProof(leaf_data: string, proof: MerkleProof, expected_root: Hash32): bool;
