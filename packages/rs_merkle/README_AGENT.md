# rs_merkle@1.0.0 (Agent Quickref)

> Zero-allocation RFC 6962 domain-separated SHA-256 Merkle tree and inclusion proof verifier (Rust rs_merkle / merkletreejs parity) for CP | License: MIT
> Import: `import { ... } from "rs_merkle";` | Full types: `src/index.d.cp`

## Functions
- `export fn sha256(data: string): Hash32;`
- `export fn hashLeaf(data: string): Hash32;`
- `export fn hashNode(left: Hash32, right: Hash32): Hash32;`
- `export fn verifyProof(leaf_data: string, proof: MerkleProof, expected_root: Hash32): bool;`
