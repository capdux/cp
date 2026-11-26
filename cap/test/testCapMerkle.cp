// cap/test/testCapMerkle.cp
// Comprehensive unit tests for @cap/merkle

import { test, expect_eq } from "cap:test";
import {
    Hash32,
    MerkleProof,
    MerkleTree,
    sha256,
    hashLeaf,
    hashNode,
    verifyProof,
} from "../../packages/@cap/merkle/src/index.cp";

fn main() {
    test("1. @cap/merkle — Built-in FIPS 180-4 SHA-256 & hex encoding vectors", () => {
        let h1: [64]u8 = undefined;
        expect_eq(sha256("").toHex(h1[0..64]), "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");

        let h2: [64]u8 = undefined;
        expect_eq(sha256("abc").toHex(h2[0..64]), "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
    });

    test("2. @cap/merkle — Empty & single-leaf tree behavior", () => {
        let tree = MerkleTree.init();
        expect_eq(tree.leafCount(), 0 as usize);
        expect_eq(tree.root() == null, true);
        expect_eq(tree.generateProof(0 as usize) == null, true);

        expect_eq(tree.addLeaf("genesis"), true);
        expect_eq(tree.leafCount(), 1 as usize);
        const rOpt = tree.root();
        expect_eq(rOpt != null, true);
        const root = rOpt ?? sha256("");
        expect_eq(root.equals(hashLeaf("genesis")), true);

        const pOpt = tree.generateProof(0 as usize);
        expect_eq(pOpt != null, true);
        const proof = pOpt ?? MerkleProof{ siblings: undefined, is_right: undefined, depth: 0 as usize };
        expect_eq(proof.depth, 0 as usize);
        expect_eq(verifyProof("genesis", proof, root), true);
        expect_eq(verifyProof("tampered", proof, root), false);
    });

    test("3. @cap/merkle — Power-of-two (4 leaves) full inclusion proofs", () => {
        let tree = MerkleTree.init();
        const items: [4]string = ["tx-0", "tx-1", "tx-2", "tx-3"];
        let i: usize = 0;
        while (i < 4) {
            expect_eq(tree.addLeaf(items[i]), true);
            i += 1;
        }
        const root = tree.root() ?? sha256("");

        let j: usize = 0;
        while (j < 4) {
            const p = tree.generateProof(j) ?? MerkleProof{ siblings: undefined, is_right: undefined, depth: 0 as usize };
            expect_eq(p.depth, 2 as usize);
            expect_eq(verifyProof(items[j], p, root), true);
            expect_eq(verifyProof("wrong-tx", p, root), false);
            j += 1;
        }
        expect_eq(tree.generateProof(4 as usize) == null, true);
    });

    test("4. @cap/merkle — Odd-sized trees (5 and 7 leaves) verify every leaf", () => {
        let t5 = MerkleTree.init();
        const data5: [5]string = ["a", "b", "c", "d", "e"];
        let i: usize = 0;
        while (i < 5) {
            * = t5.addLeaf(data5[i]);
            i += 1;
        }
        const root5 = t5.root() ?? sha256("");
        let j: usize = 0;
        while (j < 5) {
            const p5 = t5.generateProof(j) ?? MerkleProof{ siblings: undefined, is_right: undefined, depth: 0 as usize };
            expect_eq(verifyProof(data5[j], p5, root5), true);
            j += 1;
        }

        let t7 = MerkleTree.init();
        const data7: [7]string = ["k0", "k1", "k2", "k3", "k4", "k5", "k6"];
        let a: usize = 0;
        while (a < 7) {
            * = t7.addLeaf(data7[a]);
            a += 1;
        }
        const root7 = t7.root() ?? sha256("");
        let b: usize = 0;
        while (b < 7) {
            const p7 = t7.generateProof(b) ?? MerkleProof{ siblings: undefined, is_right: undefined, depth: 0 as usize };
            expect_eq(verifyProof(data7[b], p7, root7), true);
            b += 1;
        }
    });

    test("5. @cap/merkle — Tampered data, wrong leaf index & forged root rejection", () => {
        let tree = MerkleTree.init();
        * = tree.addLeaf("record-0");
        * = tree.addLeaf("record-1");
        * = tree.addLeaf("record-2");
        const root = tree.root() ?? sha256("");
        const p0 = tree.generateProof(0 as usize) ?? MerkleProof{ siblings: undefined, is_right: undefined, depth: 0 as usize };

        expect_eq(verifyProof("record-0", p0, root), true);
        // Wrong leaf for index 0's proof
        expect_eq(verifyProof("record-1", p0, root), false);
        // Single-bit flip in root
        let badRoot = root;
        badRoot.bytes[0] = badRoot.bytes[0] ^ (1 as u8);
        expect_eq(verifyProof("record-0", p0, badRoot), false);
    });

    test("6. @cap/merkle — RFC 6962 domain separation defeats second-preimage forgery", () => {
        const left = hashLeaf("L");
        const right = hashLeaf("R");
        const internal = hashNode(left, right);

        // Construct raw 64-byte concatenation left || right and hash as a leaf
        let concat: [64]u8 = undefined;
        let i: usize = 0;
        while (i < 32) {
            concat[i] = left.bytes[i];
            concat[32 + i] = right.bytes[i];
            i += 1;
        }
        const forgedLeaf = hashLeaf(concat[0..64]);
        // Must NOT equal internal node hash due to 0x00 vs 0x01 domain separation
        expect_eq(forgedLeaf.equals(internal), false);
    });
}
