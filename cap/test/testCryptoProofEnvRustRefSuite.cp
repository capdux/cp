import { test, expect_eq } from "cap:test";
import {
    MERKLE_HASH_LEN,
    MERKLE_MAX_LEAVES,
    scalarProofCtEqual,
    simdProofCtEqual,
    scalarProofCopyBytes,
    simdProofCopyBytes,
    MerkleHash,
    merkleHashEmpty,
    merkleHashLeaf,
    merkleHashNode,
    Rfc6962MerkleTree,
    verifyMerkleInclusionProofHash,
    verifyMerkleInclusionProof,
} from "../../lib/crypto/merkleTree.cp";
import {
    ENVELOPE_KEK_LEN,
    EnvelopeStatus,
    EnvelopeKeyState,
    EnvelopeKeyring,
    decodeEnvelopeHeader,
    sealEnvelope,
    openEnvelope,
} from "../../lib/crypto/envelope.cp";
import {
    MACAROON_MAX_CAVEATS,
    MacaroonStatus,
    MacaroonVerifyContext,
    Macaroon,
} from "../../lib/crypto/macaroon.cp";

const std = @import("std");

fn expect_true(cond: bool): void! {
    expect_eq(cond, true);
}

fn expect_false(cond: bool): void! {
    expect_eq(cond, false);
}

struct XorShift64 {
    state: u64 = 0x1E3779B97F4A7C15,

    fn next(*_): u64 {
        let x = _.state;
        x ^= x << 13;
        x ^= x >> 7;
        x ^= x << 17;
        _.state = x;
        return x;
    }
}

// ---------------------------------------------------------------------------
// Test 1: Envelope CPE1 Seal/Open, Key Rotation, Revocation & 1-Bit Tampering
// ---------------------------------------------------------------------------

fn runTestEnvelopeStrict(): void! {
    let kr = EnvelopeKeyring.init();
    const zero_kek: [32]u8 = [0; 32];
    const kek1: [32]u8 = [0x11; 32];
    const kek2: [32]u8 = [0x22; 32];

    // Reject zero key material and key_version == 0
    expect_eq(kr.registerKey(1, zero_kek, EnvelopeKeyState.primary, 100), EnvelopeStatus.invalid_key_material);
    expect_eq(kr.registerKey(0, kek1, EnvelopeKeyState.primary, 100), EnvelopeStatus.invalid_key_material);
    expect_eq(kr.registerKey(1, kek1, EnvelopeKeyState.primary, 100), EnvelopeStatus.ok);

    const nonce: [16]u8 = [0xAA; 16];
    const salt: [16]u8 = [0xBB; 16];
    const aad: []const u8 = "tenant=acme;doc=42";
    const pt: []const u8 = "top-secret-payload-for-envelope-encryption-v1!";

    let env_buf: [256]u8 = [0; 256];
    let pt_out: [128]u8 = [0; 128];
    const sres = sealEnvelope(kr, nonce[0..], salt[0..], aad, pt, env_buf[0..]);
    expect_eq(sres.status, EnvelopeStatus.ok);
    expect_eq(sres.key_version, 1 as u32);

    const env_slice = env_buf[0..sres.bytes_written];
    const ores = openEnvelope(kr, env_slice, aad, pt_out[0..]);
    expect_eq(ores.status, EnvelopeStatus.ok);
    expect_false(ores.needs_rewrap)?;
    expect_true(simdProofCtEqual(pt_out[0..ores.bytes_written], pt))?;

    // Single-bit flip in EVERY byte of header + tag + ciphertext MUST fail authentication or header check
    let mut_env: [256]u8 = [0; 256];
    simdProofCopyBytes(mut_env[0..sres.bytes_written], env_slice);
    let idx: usize = 0;
    while (idx < sres.bytes_written) : (idx += 1) {
        mut_env[idx] ^= 0x01;
        const r = openEnvelope(kr, mut_env[0..sres.bytes_written], aad, pt_out[0..]);
        expect_true(r.status != EnvelopeStatus.ok)?;
        mut_env[idx] ^= 0x01;
    }

    // Wrong AAD & trailing garbage byte rejected
    expect_eq(openEnvelope(kr, env_slice, "tenant=evil;doc=42", pt_out[0..]).status, EnvelopeStatus.auth_failed);
    mut_env[sres.bytes_written] = 0x00;
    expect_eq(openEnvelope(kr, mut_env[0 .. sres.bytes_written + 1], aad, pt_out[0..]).status, EnvelopeStatus.auth_failed);
    expect_eq(openEnvelope(kr, env_slice[0 .. env_slice.len - 1], aad, pt_out[0..]).status, EnvelopeStatus.truncated_input);

    // Key rotation: register v2 as primary -> v1 decrypts with needs_rewrap=true; revoke v1 -> key_revoked & scrubbed
    expect_eq(kr.registerKey(2, kek2, EnvelopeKeyState.primary, 200), EnvelopeStatus.ok);
    const ores2 = openEnvelope(kr, env_slice, aad, pt_out[0..]);
    expect_eq(ores2.status, EnvelopeStatus.ok);
    expect_true(ores2.needs_rewrap)?;

    expect_eq(kr.revokeKey(1), EnvelopeStatus.ok);
    expect_eq(openEnvelope(kr, env_slice, aad, pt_out[0..]).status, EnvelopeStatus.key_revoked);
    expect_true(simdProofCtEqual(kr.entries[0].kek[0..], zero_kek[0..]))?;

    kr.zeroize();
    expect_true(simdProofCtEqual(kr.entries[1].kek[0..], zero_kek[0..]))?;
}

// ---------------------------------------------------------------------------
// Test 2: Macaroon Chained Attenuation, Tampering, Reordering & Cap Guards
// ---------------------------------------------------------------------------

fn runTestMacaroonStrict(): void! {
    const root_key: []const u8 = "super-secret-macaroon-root-key-32b";
    let tok = Macaroon.mint(root_key, "https://api.example.com", "tok-001").?;
    expect_eq(tok.addExpCaveat(2000), MacaroonStatus.ok);
    expect_eq(tok.addScopeCaveat("read:*"), MacaroonStatus.ok);
    expect_eq(tok.addMethodCaveat("GET"), MacaroonStatus.ok);
    expect_eq(tok.addPrefixCaveat("/v1/items/"), MacaroonStatus.ok);

    const ctx_ok = MacaroonVerifyContext.init(1500, "read:docs", "GET", "/v1/items/42");
    const v_ok = tok.verify(root_key, ctx_ok);
    expect_true(v_ok.isOk())?;
    expect_eq(v_ok.caveats_checked, 4 as usize);

    // Expired / wrong scope / wrong method / wrong prefix fail closed
    expect_eq(tok.verify(root_key, MacaroonVerifyContext.init(2000, "read:docs", "GET", "/v1/items/42")).status, MacaroonStatus.caveat_expired);
    expect_eq(tok.verify(root_key, MacaroonVerifyContext.init(1500, "write:docs", "GET", "/v1/items/42")).status, MacaroonStatus.caveat_scope_denied);
    expect_eq(tok.verify(root_key, MacaroonVerifyContext.init(1500, "read:docs", "POST", "/v1/items/42")).status, MacaroonStatus.caveat_method_denied);
    expect_eq(tok.verify(root_key, MacaroonVerifyContext.init(1500, "read:docs", "GET", "/v2/admin")).status, MacaroonStatus.caveat_prefix_denied);

    // Binary encode/decode roundtrip + single-bit tamper & truncation rejection
    let wire: [512]u8 = [0; 512];
    const wlen = tok.encodeBinary(wire[0..]).?;
    const decoded = Macaroon.decodeBinary(wire[0..wlen]).?;
    expect_true(decoded.verify(root_key, ctx_ok).isOk())?;

    // Truncated or trailing junk rejected by decodeBinary
    expect_true(Macaroon.decodeBinary(wire[0 .. wlen - 1]) == null)?;
    wire[wlen] = 0x99;
    expect_true(Macaroon.decodeBinary(wire[0 .. wlen + 1]) == null)?;

    // Single-bit flip in signature or caveat bytes fails signature verification
    let f_tok = decoded;
    f_tok.sig[0] ^= 0x01;
    expect_false(f_tok.verifySignature(root_key))?;
    f_tok.sig[0] ^= 0x01;
    f_tok.caveats[0].bytes[0] ^= 0x01;
    expect_false(f_tok.verifySignature(root_key))?;

    // Caveat order swap breaks chained HMAC signature
    let swapped = decoded;
    const tmp_c = swapped.caveats[0];
    swapped.caveats[0] = swapped.caveats[1];
    swapped.caveats[1] = tmp_c;
    expect_false(swapped.verifySignature(root_key))?;

    // Caveat cap enforcement (MACAROON_MAX_CAVEATS = 16)
    let cap_tok = Macaroon.mint(root_key, "loc", "id-cap").?;
    let c: usize = 0;
    while (c < MACAROON_MAX_CAVEATS) : (c += 1) {
        expect_eq(cap_tok.addCaveat("scope = *"), MacaroonStatus.ok);
    }
    expect_eq(cap_tok.addCaveat("scope = *"), MacaroonStatus.caveats_full);

    // zeroize scrubs signature
    const zeros32: [32]u8 = [0; 32];
    cap_tok.zeroize();
    expect_true(simdProofCtEqual(cap_tok.sig[0..], zeros32[0..]))?;
}

// ---------------------------------------------------------------------------
// Test 3: RFC 6962 Merkle Tree Domain Separation, All Shapes & Tamper Reject
// ---------------------------------------------------------------------------

fn runTestMerkleRfc6962Strict(): void! {
    // 1. Empty tree root == SHA256("")
    let empty_tree = Rfc6962MerkleTree.init();
    const empty_root = empty_tree.compute_root();
    expect_true(simdProofCtEqual(empty_root[0..], merkleHashEmpty()[0..]))?;
    expect_true(empty_tree.generate_inclusion_proof(0) == null)?;

    // 2. Domain separation: leaf hash (0x00 || A||B) != node hash (0x01 || A || B)
    const lh_a = merkleHashLeaf("leaf-A");
    const lh_b = merkleHashLeaf("leaf-B");
    let concat_ab: [64]u8 = [0; 64];
    simdProofCopyBytes(concat_ab[0..32], lh_a[0..]);
    simdProofCopyBytes(concat_ab[32..64], lh_b[0..]);
    const leaf_of_concat = merkleHashLeaf(concat_ab[0..]);
    const node_of_ab = merkleHashNode(lh_a, lh_b);
    expect_false(simdProofCtEqual(leaf_of_concat[0..], node_of_ab[0..]))?;

    // 3. Single leaf, odd leaf counts (3, 5, 7, 13), and power-of-two (4, 16)
    const sizes: [6]usize = [1, 3, 4, 7, 13, 16];
    let si: usize = 0;
    while (si < sizes.len) : (si += 1) {
        const n = sizes[si];
        let tree = Rfc6962MerkleTree.init();
        let leaf_buf: [16]u8 = [0; 16];
        let i: usize = 0;
        while (i < n) : (i += 1) {
            leaf_buf[0] = (65 as u8) + ((i % 26) as u8);
            leaf_buf[1] = (48 as u8) + ((i / 10) as u8);
            leaf_buf[2] = (48 as u8) + ((i % 10) as u8);
            expect_true(tree.append_leaf(leaf_buf[0..3]))?;
        }
        const root = tree.compute_root();
        // Out-of-bounds leaf index rejected
        expect_true(tree.generate_inclusion_proof(n) == null)?;

        // Verify every single leaf's inclusion proof & 1-bit tamper rejection
        i = 0;
        while (i < n) : (i += 1) {
            leaf_buf[0] = (65 as u8) + ((i % 26) as u8);
            leaf_buf[1] = (48 as u8) + ((i / 10) as u8);
            leaf_buf[2] = (48 as u8) + ((i % 10) as u8);
            let proof = tree.generate_inclusion_proof(i).?;
            expect_true(verifyMerkleInclusionProof(leaf_buf[0..3], proof, root))?;
            expect_true(verifyMerkleInclusionProofHash(merkleHashLeaf(leaf_buf[0..3]), proof, root))?;

            // Tamper leaf data -> rejected
            leaf_buf[0] ^= 0x01;
            expect_false(verifyMerkleInclusionProof(leaf_buf[0..3], proof, root))?;
            leaf_buf[0] ^= 0x01;

            // Tamper proof sibling hash or direction -> rejected
            if (proof.depth > 0) {
                proof.steps[0].sibling[0] ^= 0x01;
                expect_false(verifyMerkleInclusionProof(leaf_buf[0..3], proof, root))?;
                proof.steps[0].sibling[0] ^= 0x01;
                proof.steps[0].sibling_is_left = !proof.steps[0].sibling_is_left;
                expect_false(verifyMerkleInclusionProof(leaf_buf[0..3], proof, root))?;
            }
        }
    }

    // MerkleHash.eql uses 16-lane SIMD constant-time comparison
    const mh1 = MerkleHash{ bytes: lh_a };
    const mh2 = MerkleHash{ bytes: lh_a };
    const mh3 = MerkleHash{ bytes: lh_b };
    expect_true(mh1.eql(mh2))?;
    expect_false(mh1.eql(mh3))?;
}

// ---------------------------------------------------------------------------
// Test 4: 16-Lane SIMD vs Scalar 0-Diff Verification
// ---------------------------------------------------------------------------

fn runTestSimdZeroDiff(): void! {
    let a: [96]u8 = [0; 96];
    let b: [96]u8 = [0; 96];
    let dst_simd: [96]u8 = [0; 96];
    let dst_scalar: [96]u8 = [0; 96];

    let i: usize = 0;
    while (i < 96) : (i += 1) {
        a[i] = ((i * 53 + 17) & 0xFF) as u8;
        b[i] = a[i];
    }

    let len: usize = 0;
    while (len <= 96) : (len += 1) {
        expect_eq(simdProofCtEqual(a[0..len], b[0..len]), scalarProofCtEqual(a[0..len], b[0..len]));
        expect_true(simdProofCtEqual(a[0..len], b[0..len]))?;

        simdProofCopyBytes(dst_simd[0..len], a[0..len]);
        scalarProofCopyBytes(dst_scalar[0..len], a[0..len]);
        expect_true(scalarProofCtEqual(dst_simd[0..len], dst_scalar[0..len]))?;

        if (len > 0 and (len <= 34 or len == 48 or len == 64 or len == 96)) {
            let p: usize = 0;
            while (p < len) : (p += 1) {
                b[p] ^= 0x40;
                expect_eq(simdProofCtEqual(a[0..len], b[0..len]), scalarProofCtEqual(a[0..len], b[0..len]));
                expect_false(simdProofCtEqual(a[0..len], b[0..len]))?;
                b[p] ^= 0x40;
            }
        }
    }
    expect_false(simdProofCtEqual(a[0..32], b[0..31]))?;
}

// ---------------------------------------------------------------------------
// Test 5: 1,800-Case Deterministic PRNG Fuzz (600 Envelope + 600 Macaroon + 600 Merkle)
// ---------------------------------------------------------------------------

fn runTestProofEnvFuzz1800(): void! {
    let rng = XorShift64 { state: 0x3704E420261124 };

    // Part A: 600 Envelope fuzz cases (300 seal/open/tamper + 300 malformed decode)
    let kr = EnvelopeKeyring.init();
    const kek: [32]u8 = [0x5A; 32];
    expect_eq(kr.registerKey(1, kek, EnvelopeKeyState.primary, 1), EnvelopeStatus.ok);
    let env_buf: [256]u8 = [0; 256];
    let pt_in: [96]u8 = [0; 96];
    let pt_out: [256]u8 = [0; 256];
    let nonce: [16]u8 = [0; 16];
    let salt: [16]u8 = [0; 16];

    let ei: usize = 0;
    while (ei < 600) : (ei += 1) {
        if (ei < 300) {
            const plen = (rng.next() as usize) % 96;
            let k: usize = 0;
            while (k < plen) : (k += 1) { pt_in[k] = (rng.next() & 0xFF) as u8; }
            k = 0;
            while (k < 16) : (k += 1) {
                nonce[k] = (rng.next() & 0xFF) as u8;
                salt[k] = (rng.next() & 0xFF) as u8;
            }
            const sres = sealEnvelope(kr, nonce[0..], salt[0..], "aad", pt_in[0..plen], env_buf[0..]);
            expect_eq(sres.status, EnvelopeStatus.ok);
            const ores = openEnvelope(kr, env_buf[0..sres.bytes_written], "aad", pt_out[0..]);
            expect_eq(ores.status, EnvelopeStatus.ok);
            expect_true(simdProofCtEqual(pt_out[0..ores.bytes_written], pt_in[0..plen]))?;

            // Flip 1 random bit -> must fail
            const fp = (rng.next() as usize) % sres.bytes_written;
            env_buf[fp] ^= 0x01;
            expect_true(openEnvelope(kr, env_buf[0..sres.bytes_written], "aad", pt_out[0..]).status != EnvelopeStatus.ok)?;
        } else {
            const rlen = (rng.next() as usize) % 160;
            let k: usize = 0;
            while (k < rlen) : (k += 1) { env_buf[k] = (rng.next() & 0xFF) as u8; }
            * = decodeEnvelopeHeader(env_buf[0..rlen]);
            * = openEnvelope(kr, env_buf[0..rlen], "aad", pt_out[0..]);
        }
    }

    // Part B: 600 Macaroon fuzz cases (300 decode malformed + 300 mint/attenuate/wire/tamper)
    let mbuf: [256]u8 = [0; 256];
    let mi: usize = 0;
    while (mi < 600) : (mi += 1) {
        if (mi < 300) {
            const rlen = (rng.next() as usize) % 200;
            let k: usize = 0;
            while (k < rlen) : (k += 1) { mbuf[k] = (rng.next() & 0xFF) as u8; }
            if (tok := Macaroon.decodeBinary(mbuf[0..rlen])) {
                * = tok.verifySignature("rk");
            }
        } else {
            let tok = Macaroon.mint("fuzz-root-key", "loc", "id-fz").?;
            const n_cav = (rng.next() as usize) % 5;
            let c: usize = 0;
            while (c < n_cav) : (c += 1) {
                expect_eq(tok.addCaveat("scope = *"), MacaroonStatus.ok);
            }
            const wl = tok.encodeBinary(mbuf[0..]).?;
            const dec_tok = Macaroon.decodeBinary(mbuf[0..wl]).?;
            expect_true(dec_tok.verifySignature("fuzz-root-key"))?;
            // Flip 1 random bit in signature region -> must fail
            const fp = wl - 1 - ((rng.next() as usize) % 32);
            mbuf[fp] ^= 0x01;
            const bad = Macaroon.decodeBinary(mbuf[0..wl]).?;
            expect_false(bad.verifySignature("fuzz-root-key"))?;
        }
    }

    // Part C: 600 Merkle fuzz cases (random tree sizes 1..=32, random leaf verification & tamper)
    let mki: usize = 0;
    while (mki < 600) : (mki += 1) {
        const tsize = ((rng.next() as usize) % 32) + 1;
        let tree = Rfc6962MerkleTree.init();
        let leaves_raw: [32][8]u8 = [[0; 8]; 32];
        let li: usize = 0;
        while (li < tsize) : (li += 1) {
            let b: usize = 0;
            while (b < 8) : (b += 1) {
                leaves_raw[li][b] = (rng.next() & 0xFF) as u8;
            }
            expect_true(tree.append_leaf(leaves_raw[li][0..]))?;
        }
        const root = tree.compute_root();
        const pick = (rng.next() as usize) % tsize;
        let proof = tree.generate_inclusion_proof(pick).?;
        expect_true(verifyMerkleInclusionProof(leaves_raw[pick][0..], proof, root))?;
        if (proof.depth > 0) {
            const step_idx = (rng.next() as usize) % proof.depth;
            const byte_idx = (rng.next() as usize) % MERKLE_HASH_LEN;
            proof.steps[step_idx].sibling[byte_idx] ^= 0x01;
            expect_false(verifyMerkleInclusionProof(leaves_raw[pick][0..], proof, root))?;
        }
    }
}

fn main() {
    test("Envelope CPE1 seal/open, key rotation, revocation & 1-bit tamper rejection", () => {
        runTestEnvelopeStrict()?;
    });
    test("Macaroon chained attenuation, wire codec, order-swap & cap guards", () => {
        runTestMacaroonStrict()?;
    });
    test("RFC 6962 Merkle domain separation, all tree shapes & 1-bit proof tamper rejection", () => {
        runTestMerkleRfc6962Strict()?;
    });
    test("16-lane SIMD vs Scalar 0-diff differential verification", () => {
        runTestSimdZeroDiff()?;
    });
    test("1800-case deterministic PRNG Fuzz (600 Envelope + 600 Macaroon + 600 Merkle)", () => {
        runTestProofEnvFuzz1800()?;
    });
}
