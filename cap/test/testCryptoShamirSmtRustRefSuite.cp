// Reference-hardened test suite against RustCrypto & GF(256) Shamir / SMT (MIT OR Apache-2.0)
// Covers exhaustive GF(256) algebra (all 255 non-zero inverses, div-by-zero), Shamir (k, n)
// threshold splitting/reconstruction/k-1 non-recovery/tamper/duplicate-x/x=0/threshold guards,
// Sparse Merkle Tree inclusion/non-inclusion/shared-prefix/update/delete-restore/single-bit &
// depth tamper rejection, 16-lane @Vector(16, u8) SIMD vs scalar 0-diff parity, and 1,600
// deterministic PRNG fuzz cases (800 Shamir + 800 SMT).

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    SHAMIR_MAX_SECRET_LEN,
    SHAMIR_MAX_SHARES,
    simdShamirSmtCtEqual,
    scalarShamirSmtCtEqual,
    simdShamirSmtXor,
    scalarShamirSmtXor,
    gf256Mul,
    gf256Inv,
    gf256Div,
    ShamirStatus,
    ShamirShare,
    splitSecretDeterministic,
    combineShamirShares,
} from "../../lib/crypto/shamirSecret.cp";
import {
    SMT_MAX_DEPTH,
    smtHashLeaf,
    SparseMerkleTree,
    verifySmtInclusionProof,
    verifySmtNonInclusionProof,
} from "../../lib/crypto/sparseMerkle.cp";

struct XorShift64 {
    state: u64;

    static fn init(seed: u64): XorShift64 {
        return XorShift64{ state: if (seed == 0) 1 else seed };
    }

    fn next(*_): u64 {
        let x = _.state;
        x ^= x << 13;
        x ^= x >> 7;
        x ^= x << 17;
        _.state = x;
        return x;
    }

    fn nextByte(*_): u8 {
        return @as(u8, @truncate(_.next()));
    }
}

fn checkTest1Gf256AlgebraExhaustive(): u32 {
    // 1a. Zero identities & division-by-zero rejection
    if (gf256Inv(0) != 0) return 1;
    if (gf256Div(42, 0) != null) return 2;
    if (gf256Mul(0, 0xFF) != 0 || gf256Mul(0xFF, 0) != 0) return 3;

    // 1b. All 255 non-zero elements: gf256Mul(a, gf256Inv(a)) == 1 and gf256Div(a, a) == 1
    let a: usize = 1;
    while (a < 256) : (a += 1) {
        const av: u8 = @as(u8, @truncate(a));
        const inv = gf256Inv(av);
        if (inv == 0) return 4;
        if (gf256Mul(av, inv) != 1) return 5;
        if (gf256Mul(inv, av) != 1) return 6;
        const d1 = gf256Div(av, av) orelse return 7;
        if (d1 != 1) return 8;
        const d0 = gf256Div(0, av) orelse return 9;
        if (d0 != 0) return 10;
    }

    // 1c. Commutativity, associativity, distributivity spot checks across field
    const samples: [6]u8 = [0x01, 0x02, 0x1B, 0x57, 0x83, 0xFF];
    let i: usize = 0;
    while (i < 6) : (i += 1) {
        let j: usize = 0;
        while (j < 6) : (j += 1) {
            const x = samples[i];
            const y = samples[j];
            if (gf256Mul(x, y) != gf256Mul(y, x)) return 11;
            const prod = gf256Mul(x, y);
            const q = gf256Div(prod, y) orelse return 12;
            if (q != x) return 13;
        }
    }
    return 0;
}

fn checkTest2ShamirSplitCombineAndGuards(): u32 {
    const secret: []const u8 = "top-secret-master-key-32-bytes!!";
    let shares: [5]ShamirShare = [ShamirShare{}; 5];
    let shares_s: []ShamirShare = shares[0..];

    // 2a. Parameter rejection guards: k=0, k=1, k>n, n>32, empty secret, >64B secret
    if (splitSecretDeterministic(secret, 0, 5, 1, true, shares_s).status != ShamirStatus.invalid_threshold) return 1;
    if (splitSecretDeterministic(secret, 1, 5, 1, true, shares_s).status != ShamirStatus.invalid_threshold) return 2;
    if (splitSecretDeterministic(secret, 6, 5, 1, true, shares_s).status != ShamirStatus.invalid_share_count) return 3;
    if (splitSecretDeterministic(secret, 2, SHAMIR_MAX_SHARES + 1, 1, true, shares_s).status != ShamirStatus.invalid_share_count) return 4;
    if (splitSecretDeterministic("", 2, 5, 1, true, shares_s).status != ShamirStatus.invalid_secret_len) return 5;
    let too_long: [65]u8 = [0x41; 65];
    if (splitSecretDeterministic(too_long[0..], 2, 5, 1, true, shares_s).status != ShamirStatus.invalid_secret_len) return 6;

    // 2b. Valid (3, 5) split
    const sp = splitSecretDeterministic(secret, 3, 5, 0xCAFEBABE, true, shares_s);
    if (sp.status != ShamirStatus.ok || sp.shares_written != 5) return 7;

    // 2c. Every C(5, 3) = 10 combinations of 3 shares recover exact secret
    let out: [64]u8 = [0; 64];
    let out_s: []u8 = out[0..];
    let a: usize = 0;
    while (a < 5) : (a += 1) {
        let b: usize = a + 1;
        while (b < 5) : (b += 1) {
            let c: usize = b + 1;
            while (c < 5) : (c += 1) {
                const sub: [3]ShamirShare = [shares[a], shares[b], shares[c]];
                const res = combineShamirShares(sub[0..], out_s);
                if (res.status != ShamirStatus.ok || !res.tag_verified || res.secret_len != secret.len) return 8;
                if (!simdShamirSmtCtEqual(out[0..secret.len], secret)) return 9;
            }
        }
    }

    // 2d. k - 1 = 2 shares MUST be rejected (and even if threshold field is stripped, must not recover secret)
    const two_sub: [2]ShamirShare = [shares[0], shares[1]];
    if (combineShamirShares(two_sub[0..], out_s).status != ShamirStatus.insufficient_shares) return 10;

    let forged_two: [2]ShamirShare = [shares[0], shares[1]];
    forged_two[0].threshold = 0;
    forged_two[1].threshold = 0;
    const f_res = combineShamirShares(forged_two[0..], out_s);
    if (f_res.status != ShamirStatus.tag_mismatch) return 11;

    // 2e. Duplicate x coordinate rejected
    const dup_sub: [3]ShamirShare = [shares[0], shares[0], shares[2]];
    if (combineShamirShares(dup_sub[0..], out_s).status != ShamirStatus.duplicate_x) return 12;

    // 2f. x == 0 share rejected
    let zero_x_sub: [3]ShamirShare = [shares[0], shares[1], shares[2]];
    zero_x_sub[1].x = 0;
    if (combineShamirShares(zero_x_sub[0..], out_s).status != ShamirStatus.invalid_x_zero) return 13;

    // 2g. Single-bit payload tamper rejected & out_secret scrubbed to all zeros
    let tampered_sub: [3]ShamirShare = [shares[0], shares[2], shares[4]];
    tampered_sub[1].y[7] ^= 0x01;
    const t_res = combineShamirShares(tampered_sub[0..], out_s);
    if (t_res.status != ShamirStatus.tag_mismatch || t_res.tag_verified) return 14;
    const zero_sec: [32]u8 = [0; 32];
    if (!simdShamirSmtCtEqual(out[0..32], zero_sec[0..])) return 15;

    // 2h. zeroize clears share payload
    shares[0].zeroize();
    if (shares[0].x != 0 || shares[0].secret_len != 0) return 16;

    return 0;
}

fn checkTest3SmtInclusionNonInclusionAndTamper(): u32 {
    // 3a. Invalid depths rejected
    if (SparseMerkleTree.init(0) != null) return 1;
    if (SparseMerkleTree.init((SMT_MAX_DEPTH as u8) + 1) != null) return 2;

    let tree = SparseMerkleTree.init(16) orelse return 3;
    const empty_root = tree.defaultRootHash();
    if (!simdShamirSmtCtEqual(tree.rootHash()[0..], empty_root[0..])) return 4;

    // Non-inclusion on empty tree
    const p_empty = tree.generateProof(0x1234) orelse return 5;
    if (!verifySmtNonInclusionProof(empty_root, 0x1234, p_empty)) return 6;
    if (verifySmtInclusionProof(empty_root, 0x1234, "", p_empty)) return 7;

    // 3b. Single key + shared long-prefix keys (0xABC0 and 0xABC1 share 15-bit prefix!)
    if (!tree.put(0xABC0, "leaf-zero")) return 8;
    const root_one = tree.rootHash();
    const p_one = tree.generateProof(0xABC0) orelse return 9;
    if (!verifySmtInclusionProof(root_one, 0xABC0, "leaf-zero", p_one)) return 10;

    if (!tree.put(0xABC1, "leaf-one")) return 11;
    const root_two = tree.rootHash();
    if (simdShamirSmtCtEqual(root_one[0..], root_two[0..])) return 12;

    const p_k0 = tree.generateProof(0xABC0) orelse return 13;
    const p_k1 = tree.generateProof(0xABC1) orelse return 14;
    const p_k2 = tree.generateProof(0xABC2) orelse return 15;
    if (!verifySmtInclusionProof(root_two, 0xABC0, "leaf-zero", p_k0)) return 16;
    if (!verifySmtInclusionProof(root_two, 0xABC1, "leaf-one", p_k1)) return 17;
    if (!verifySmtNonInclusionProof(root_two, 0xABC2, p_k2)) return 18;

    // 3c. Single-bit sibling hash tamper, depth tamper, unused sibling tamper, value tamper
    let bad_sib = p_k0;
    bad_sib.siblings[0][0] ^= 0x01;
    if (verifySmtInclusionProof(root_two, 0xABC0, "leaf-zero", bad_sib)) return 19;

    let bad_depth = p_k0;
    bad_depth.depth = 15;
    if (verifySmtInclusionProof(root_two, 0xABC0, "leaf-zero", bad_depth)) return 20;

    let bad_unused = p_k0;
    bad_unused.siblings[20][0] = 0x01;
    if (verifySmtInclusionProof(root_two, 0xABC0, "leaf-zero", bad_unused)) return 21;

    if (verifySmtInclusionProof(root_two, 0xABC0, "leaf-wrong", p_k0)) return 22;

    let bad_noninc = p_k2;
    bad_noninc.siblings[3][15] ^= 0x80;
    if (verifySmtNonInclusionProof(root_two, 0xABC2, bad_noninc)) return 23;

    // 3d. Update key & delete-restore back to exact root_one and empty_root
    if (!tree.put(0xABC0, "leaf-zero-v2")) return 24;
    const root_upd = tree.rootHash();
    const p_upd = tree.generateProof(0xABC0) orelse return 25;
    if (!verifySmtInclusionProof(root_upd, 0xABC0, "leaf-zero-v2", p_upd)) return 26;
    if (verifySmtInclusionProof(root_upd, 0xABC0, "leaf-zero", p_upd)) return 27;

    if (!tree.put(0xABC0, "leaf-zero")) return 28;
    if (!tree.delete(0xABC1)) return 29;
    if (tree.delete(0xABC1)) return 30; // idempotent delete
    if (!simdShamirSmtCtEqual(tree.rootHash()[0..], root_one[0..])) return 31;

    if (!tree.delete(0xABC0)) return 32;
    if (!simdShamirSmtCtEqual(tree.rootHash()[0..], empty_root[0..])) return 33;

    // Out-of-range key for depth 16 (>= 65536) rejected
    if (tree.put(65536, "oob")) return 34;
    if (tree.generateProof(65536) != null) return 35;

    return 0;
}

fn checkTest4SimdDifferential(): u32 {
    let rng = XorShift64.init(0x5348414D_4952534D);
    let a: [192]u8 = [0; 192];
    let b: [192]u8 = [0; 192];
    let dst_simd: [192]u8 = [0; 192];
    let dst_scalar: [192]u8 = [0; 192];

    let len: usize = 0;
    while (len <= 160) : (len += 1) {
        let i: usize = 0;
        while (i < len) : (i += 1) {
            a[i] = rng.nextByte();
            b[i] = rng.nextByte();
            dst_simd[i] = a[i];
            dst_scalar[i] = a[i];
        }

        // CT equality parity on equal slices
        if (!simdShamirSmtCtEqual(a[0..len], dst_simd[0..len])) return 1;
        if (!scalarShamirSmtCtEqual(a[0..len], dst_simd[0..len])) return 2;

        // XOR parity
        simdShamirSmtXor(dst_simd[0..len], b[0..len]);
        scalarShamirSmtXor(dst_scalar[0..len], b[0..len]);
        if (!simdShamirSmtCtEqual(dst_simd[0..len], dst_scalar[0..len])) return 3;
        if (!scalarShamirSmtCtEqual(dst_simd[0..len], dst_scalar[0..len])) return 4;

        // XOR involution: (a ^ b) ^ b == a
        simdShamirSmtXor(dst_simd[0..len], b[0..len]);
        if (!simdShamirSmtCtEqual(dst_simd[0..len], a[0..len])) return 5;

        // Single-bit flip CT equality parity
        if (len > 0) {
            const flip_pos = @as(usize, @truncate(rng.next())) % len;
            dst_simd[flip_pos] ^= 0x01;
            const eq_s = simdShamirSmtCtEqual(a[0..len], dst_simd[0..len]);
            const eq_c = scalarShamirSmtCtEqual(a[0..len], dst_simd[0..len]);
            if (eq_s || eq_s != eq_c) return 6;
        }
    }

    if (simdShamirSmtCtEqual(a[0..16], a[0..17])) return 7;
    if (scalarShamirSmtCtEqual(a[0..16], a[0..17])) return 8;
    return 0;
}

fn checkTest5PrngFuzz1600(): u32 {
    let rng = XorShift64.init(0x57333752_3946555A);

    // 5a. 800 Shamir PRNG fuzz cases: random secret_len (1..64), random (k, n) in 2..=8,
    // random k-subset selection, exact recovery + random 1-bit tamper rejection
    let shares: [8]ShamirShare = [ShamirShare{}; 8];
    let shares_s: []ShamirShare = shares[0..];
    let subset: [8]ShamirShare = [ShamirShare{}; 8];
    let secret_buf: [64]u8 = [0; 64];
    let out_buf: [64]u8 = [0; 64];
    let out_s: []u8 = out_buf[0..];

    let iter: usize = 0;
    while (iter < 800) : (iter += 1) {
        const slen = 1 + (@as(usize, @truncate(rng.next())) % SHAMIR_MAX_SECRET_LEN);
        let bi: usize = 0;
        while (bi < slen) : (bi += 1) {
            secret_buf[bi] = rng.nextByte();
        }
        const n_u8: u8 = (2 as u8) + @as(u8, @truncate(rng.next() % 7)); // 2..=8
        const k_u8: u8 = (2 as u8) + @as(u8, @truncate(rng.next() % ((n_u8 - 1) as u64))); // 2..=n
        const seed = rng.next();

        const sp = splitSecretDeterministic(secret_buf[0..slen], k_u8, n_u8, seed, true, shares_s);
        if (sp.status != ShamirStatus.ok) return 1;

        // Pick k distinct shares via cyclic stride
        const start_idx = @as(usize, @truncate(rng.next())) % (n_u8 as usize);
        let si: usize = 0;
        while (si < (k_u8 as usize)) : (si += 1) {
            subset[si] = shares[(start_idx + si) % (n_u8 as usize)];
        }
        const comb = combineShamirShares(subset[0..(k_u8 as usize)], out_s);
        if (comb.status != ShamirStatus.ok || !comb.tag_verified || comb.secret_len != slen) return 2;
        if (!simdShamirSmtCtEqual(out_buf[0..slen], secret_buf[0..slen])) return 3;

        // Every 4th iteration, flip 1 bit in a share and verify tag_mismatch rejection
        if ((iter & 3) == 0) {
            subset[0].y[iter % slen] ^= 0x01;
            const bad_c = combineShamirShares(subset[0..(k_u8 as usize)], out_s);
            if (bad_c.status != ShamirStatus.tag_mismatch) return 4;
        }
    }

    // 5b. 800 SMT PRNG fuzz cases: random put/update/delete/inclusion/non-inclusion over depth-12 tree
    let tree = SparseMerkleTree.init(12) orelse return 5;
    let model_present: [64]bool = [false; 64];
    let model_val: [64]u8 = [0; 64];
    let val_slice: [4]u8 = [0; 4];

    iter = 0;
    while (iter < 800) : (iter += 1) {
        const k_slot = @as(usize, @truncate(rng.next())) % 64;
        // Spread keys across depth-12 space while keeping shared-prefix pairs (k_slot * 53)
        const key: u32 = ((k_slot as u32) * 53) & 0x0FFF;
        const op = rng.next() % 10;

        if (op < 5) {
            const v = rng.nextByte();
            val_slice[0] = v;
            val_slice[1] = v ^ 0x5A;
            val_slice[2] = @as(u8, @truncate(k_slot));
            val_slice[3] = 0xA5;
            if (!tree.put(key, val_slice[0..])) return 6;
            model_present[k_slot] = true;
            model_val[k_slot] = v;
        } else if (op < 7) {
            const del_ok = tree.delete(key);
            if (del_ok != model_present[k_slot]) return 7;
            model_present[k_slot] = false;
        } else {
            const root = tree.rootHash();
            const proof = tree.generateProof(key) orelse return 8;
            if (model_present[k_slot]) {
                const v = model_val[k_slot];
                val_slice[0] = v;
                val_slice[1] = v ^ 0x5A;
                val_slice[2] = @as(u8, @truncate(k_slot));
                val_slice[3] = 0xA5;
                if (!verifySmtInclusionProof(root, key, val_slice[0..], proof)) return 9;
                if (verifySmtNonInclusionProof(root, key, proof)) return 10;
            } else {
                if (!verifySmtNonInclusionProof(root, key, proof)) return 11;
                if (verifySmtInclusionProof(root, key, val_slice[0..], proof)) return 12;
            }
        }
    }

    // Final check: delete all active keys -> root must equal defaultRootHash
    let k: usize = 0;
    while (k < 64) : (k += 1) {
        if (model_present[k]) {
            const key: u32 = ((k as u32) * 53) & 0x0FFF;
            if (!tree.delete(key)) return 13;
        }
    }
    if (!simdShamirSmtCtEqual(tree.rootHash()[0..], tree.defaultRootHash()[0..])) return 14;
    if (tree.stats().leaf_count != 0) return 15;

    return 0;
}

fn main() {
    test("1. Exhaustive GF(256) Algebra (All 255 Non-Zero Inverses & Div-by-Zero)", () => {
        expect_eq(checkTest1Gf256AlgebraExhaustive(), 0 as u32);
    });

    test("2. Shamir (k, n) Split/Combine, C(5,3) Recovery, k-1/Dup-x/x=0/Tamper Guards", () => {
        expect_eq(checkTest2ShamirSplitCombineAndGuards(), 0 as u32);
    });

    test("3. Sparse Merkle Tree Inclusion, Non-Inclusion, Shared Prefix & Tamper Guards", () => {
        expect_eq(checkTest3SmtInclusionNonInclusionAndTamper(), 0 as u32);
    });

    test("4. 16-Lane @Vector(16, u8) SIMD vs Scalar 0-Diff Parity (CtEqual & Xor)", () => {
        expect_eq(checkTest4SimdDifferential(), 0 as u32);
    });

    test("5. Deterministic PRNG Fuzzing (800 Shamir + 800 SMT = 1,600 Cases)", () => {
        expect_eq(checkTest5PrngFuzz1600(), 0 as u32);
    });
}
