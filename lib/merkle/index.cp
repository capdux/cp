// Source: cp/lib/merkle/index.cp (Unified Exports for cap:merkle)
// Pure CP zero-C-dependency RFC 6962 Merkle Hash Tree engine.

export {
    MerkleError,
    ProofSide,
    ProofStep,
    MerkleProof,
    MerkleTree,
    hashLeaf,
    hashNode,
    verifyProof,
    diffTrees,
} from "./merkle.cp";
