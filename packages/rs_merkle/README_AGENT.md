# rs_merkle — AI Agent Quick Ref

Zero-allocation RFC 6962 domain-separated SHA-256 Merkle tree and inclusion proof verifier (Rust rs_merkle / merkletreejs parity) for CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    Hash32,
    Sha256State,
    MerkleProof,
    MerkleTree,
    hexLower,
    zeroHash,
    add32,
    pow2_64,
} from "rs_merkle";
```

## Core Types & Constants

```cp
class Hash32 { bytes: [32]u8 } // methods: equals(...) +1
class Sha256State { h: [8]u32, buf: [64]u8, buf_len: usize, total_len: u64 }
class MerkleProof { siblings: [MAX_PROOF_DEPTH]Hash32, is_right: [MAX_PROOF_DEPTH]bool, depth: usize }
class MerkleTree { leaves: [MAX_LEAVES]Hash32, count: usize } // methods: init(...) +4
```

## API Signatures

```cp
fn hexLower(nibble: u8): u8;
fn zeroHash(): Hash32;
fn add32(a: u32, b: u32): u32;
fn pow2_64(n: u32): u64;
fn rotr32(x: u32, n: u32): u32;
fn shr32(x: u32, n: u32): u32;
fn bitNot32(x: u32): u32;
fn sha256K(i: usize): u32;
fn sha256Init(): Sha256State;
fn sha256Compress(st: &mut Sha256State);
fn sha256UpdateByte(st: &mut Sha256State, byte: u8);
fn sha256UpdateSlice(st: &mut Sha256State, data: string);
fn sha256Finalize(st: &mut Sha256State): Hash32;
fn sha256(data: string): Hash32;
fn hashLeaf(data: string): Hash32;
fn hashNode(left: Hash32, right: Hash32): Hash32;
fn emptyProof(): MerkleProof;
fn verifyProof(leaf_data: string, proof: MerkleProof, expected_root: Hash32): bool;
```

## Minimal Usage

```cp
import { hashLeaf } from "rs_merkle";

pub fn main() {
    let result = hashLeaf("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"rs_merkle"`; install via `cap add rs_merkle`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).
