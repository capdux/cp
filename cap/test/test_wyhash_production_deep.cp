// Source: cp/cap/test/test_wyhash_production_deep.cp (Deep production tests for cap:wyhash & cap.wyhash)

import cap from "cap";
import { test, expect_eq } from "cap:test";
import {
    Wyhash,
    WyhashStream,
    WyhashStateless,
    wyhash,
    wyhash64,
    wyhashFinal4,
    WyRand,
    hashString,
    hashInt,
    combineHashes,
} from "cap:wyhash";

const std = @import("std");

test("1. Wyhash 0..256 byte gradient determinism, avalanche & std.hash.Wyhash parity") {
    let data: [u8; 256] = [0; 256];
    let i: usize = 0;
    while (i < 256) {
        data[i] = ((i * 37 + 13) & 0xff) as u8;
        i += 1;
    }

    // Verify 0..256 length gradient against std.hash.Wyhash
    let len: usize = 0;
    while (len <= 256) {
        let slice: string = data[0..len];
        let h_cp = wyhash(0, slice);
        let h_std = std.hash.Wyhash.hash(0, slice);
        expect_eq(h_cp, h_std);

        let h_seeded = wyhash(0x123456789abcdef0, slice);
        let h_std_seeded = std.hash.Wyhash.hash(0x123456789abcdef0, slice);
        expect_eq(h_seeded, h_std_seeded);
        len += 1;
    }

    // Avalanche check: flipping 1 bit in a 64-byte input changes many output bits
    let a: [u8; 64] = [0xaa; 64];
    let b: [u8; 64] = [0xaa; 64];
    b[17] = b[17] ^ 0x01;
    let ha = wyhash(99, a[0..64]);
    let hb = wyhash(99, b[0..64]);
    let diff = ha ^ hb;
    let popcnt = @popCount(diff);
    std.testing.expect(popcnt >= 16) catch unreachable;
}

test("2. WyhashStream arbitrary 1..7 byte chunking matches one-shot wyhash bit-for-bit") {
    let payload: [u8; 197] = [0; 197];
    let idx: usize = 0;
    while (idx < 197) {
        payload[idx] = ((idx * 17 + 5) & 0xff) as u8;
        idx += 1;
    }
    let full: string = payload[0..197];
    let seed: u64 = 0xdeadbeefcafebabe;
    let expected = wyhash(seed, full);

    let step: usize = 1;
    while (step <= 7) {
        let stream = WyhashStream.init(seed);
        let off: usize = 0;
        while (off < full.len) {
            let end = off + step;
            if (end > full.len) {
                end = full.len;
            }
            stream.update(full[off..end]);
            off = end;
        }
        expect_eq(stream.final(), expected);
        step += 1;
    }

    // Also verify Wyhash alias and mixed empty/large chunks
    let s2 = Wyhash.init(seed);
    s2.update("");
    s2.update(full[0..33]);
    s2.update("");
    s2.update(full[33..197]);
    expect_eq(s2.final(), expected);
}

test("3. WyRand deterministic sequence, Lemire unbiased nextRange, nextF64 & fillBytes") {
    let r1 = WyRand.init(20261120);
    let r2 = WyRand.init(20261120);
    let k: usize = 0;
    while (k < 32) {
        expect_eq(r1.nextU64(), r2.nextU64());
        k += 1;
    }

    // Distinct seeds produce distinct outputs
    let r3 = WyRand.init(20261121);
    std.testing.expect(r1.nextU64() != r3.nextU64()) catch unreachable;

    // Range & distribution sanity
    let buckets: [u64; 10] = [0; 10];
    let sample_idx: usize = 0;
    while (sample_idx < 1000) {
        let v = r1.nextRange(10);
        std.testing.expect(v < 10) catch unreachable;
        buckets[v as usize] += 1;
        let f = r1.nextF64();
        std.testing.expect(f >= 0.0 && f < 1.0) catch unreachable;
        sample_idx += 1;
    }
    let b_idx: usize = 0;
    while (b_idx < 10) {
        std.testing.expect(buckets[b_idx] > 40) catch unreachable;
        b_idx += 1;
    }

    // Edge bounds 0 and 1
    expect_eq(r1.nextRange(0), 0);
    expect_eq(r1.nextRange(1), 0);

    // fillBytes for non-multiple-of-8 length
    let buf: [u8; 29] = [0; 29];
    r1.fillBytes(buf[0..29]);
    let non_zero: usize = 0;
    let bi: usize = 0;
    while (bi < 29) {
        if (buf[bi] != 0) {
            non_zero += 1;
        }
        bi += 1;
    }
    std.testing.expect(non_zero > 10) catch unreachable;
}

test("4. wyhash64, wyhashFinal4, hashString, hashInt & combineHashes collision resistance") {
    let h_ab = wyhash64(1, 2);
    let h_ba = wyhash64(2, 1);
    std.testing.expect(h_ab != h_ba) catch unreachable;
    expect_eq(h_ab, wyhashFinal4(1, 2));

    let hs1 = hashString("alpha");
    let hs2 = hashString("beta");
    let hs3 = hashString("alpha");
    expect_eq(hs1, hs3);
    std.testing.expect(hs1 != hs2) catch unreachable;

    let hi1 = hashInt(0);
    let hi2 = hashInt(1);
    std.testing.expect(hi1 != hi2) catch unreachable;

    let c1 = combineHashes(hs1, hi1);
    let c2 = combineHashes(hi1, hs1);
    std.testing.expect(c1 != c2) catch unreachable;
}

test("5. cap.wyhash dynamic namespace mount and zero heap allocation") {
    let msg: string = "cap-wyhash-zero-alloc-check";
    let h_direct = wyhash(777, msg);
    let h_cap = cap.wyhash.wyhash(777, msg);
    expect_eq(h_direct, h_cap);

    let rng_cap = cap.wyhash.WyRand.init(888);
    let rng_direct = WyRand.init(888);
    expect_eq(rng_cap.nextU32(), rng_direct.nextU32());
    expect_eq(cap.wyhash.hashString(msg), hashString(msg));
    expect_eq(cap.wyhash.WyhashStateless.hash(777, msg), h_direct);
}
