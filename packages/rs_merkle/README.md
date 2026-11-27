[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)

# rs_merkle

Zero-allocation RFC 6962 domain-separated SHA-256 Merkle tree and inclusion proof verifier (Rust rs_merkle / merkletreejs parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:rs_merkle` ([https://github.com/antouhou/rs-merkle](https://github.com/antouhou/rs-merkle))（原库名 `rs_merkle` → CP 包名 `rs_merkle`）

| Metadata | Value |
| --- | --- |
| Package | `rs_merkle` |
| Version | `1.0.0` |
| License | `MIT` |
| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |

## Installation & Import

```bash
cap add rs_merkle
```

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

## Core API

### Data Structures

| Type | Kind | Fields & Methods |
| --- | --- | --- |
| `Hash32` | `class` | Fields: `bytes: [32]u8` · Methods: `equals, toHex` |
| `Sha256State` | `class` | Fields: `h: [8]u32, buf: [64]u8, buf_len: usize, total_len: u64` |
| `MerkleProof` | `class` | Fields: `siblings: [MAX_PROOF_DEPTH]Hash32, is_right: [MAX_PROOF_DEPTH]bool, depth: usize` |
| `MerkleTree` | `class` | Fields: `leaves: [MAX_LEAVES]Hash32, count: usize` · Methods: `init, addLeaf, leafCount, root, generateProof` |

### Functions

| Function | Signature | Description |
| --- | --- | --- |
| `hexLower` | `(nibble: u8): u8` | Invokes `hexLower` and returns `u8`. |
| `zeroHash` | `(): Hash32` | Invokes `zeroHash` and returns `Hash32`. |
| `add32` | `(a: u32, b: u32): u32` | Mutates or appends state via `add32`. |
| `pow2_64` | `(n: u32): u64` | Invokes `pow2_64` and returns `u64`. |
| `rotr32` | `(x: u32, n: u32): u32` | Invokes `rotr32` and returns `u32`. |
| `shr32` | `(x: u32, n: u32): u32` | Invokes `shr32` and returns `u32`. |
| `bitNot32` | `(x: u32): u32` | Invokes `bitNot32` and returns `u32`. |
| `sha256K` | `(i: usize): u32` | Invokes `sha256K` and returns `u32`. |
| `sha256Init` | `(): Sha256State` | Invokes `sha256Init` and returns `Sha256State`. |
| `sha256Compress` | `(st: &mut Sha256State)` | Executes `sha256Compress` in-place. |
| `sha256UpdateByte` | `(st: &mut Sha256State, byte: u8)` | Executes `sha256UpdateByte` in-place. |
| `sha256UpdateSlice` | `(st: &mut Sha256State, data: string)` | Executes `sha256UpdateSlice` in-place. |
| `sha256Finalize` | `(st: &mut Sha256State): Hash32` | Invokes `sha256Finalize` and returns `Hash32`. |
| `sha256` | `(data: string): Hash32` | Invokes `sha256` and returns `Hash32`. |
| `hashLeaf` | `(data: string): Hash32` | Checks predicate `hashLeaf` and returns `bool`. |
| `hashNode` | `(left: Hash32, right: Hash32): Hash32` | Checks predicate `hashNode` and returns `bool`. |
| `emptyProof` | `(): MerkleProof` | Invokes `emptyProof` and returns `MerkleProof`. |
| `verifyProof` | `(leaf_data: string, proof: MerkleProof, expected_root: Hash32): bool` | Validates input with `verifyProof` returning `bool`. |

## Minimal Example

```cp
import { hashLeaf } from "rs_merkle";

pub fn main() {
    let result = hashLeaf("example");
    _ = result;
}
```

## CP Zero-Heap & Borrow-Safety Highlights

- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection.
- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies.
- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default).
