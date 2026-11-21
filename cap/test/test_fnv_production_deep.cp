// Source: cp/cap/test/test_fnv_production_deep.cp (Production deep tests for cap:fnv)

import { test, expect_eq } from "cap:test";
import {
    FNV32_OFFSET_BASIS,
    FNV32_PRIME,
    FNV64_OFFSET_BASIS,
    FNV64_PRIME,
    FNV128_OFFSET_BASIS,
    FNV128_PRIME,
    Fnv1a32,
    Fnv1a64,
    Fnv1a128,
    Fnv1_32,
    Fnv1_64,
    Fnv1_128,
    fnv1a32,
    fnv1a64,
    fnv1a128,
    fnv1_32,
    fnv1_64,
    fnv1_128,
    fnvXorFold32,
    djb2,
    djb2Xor,
    sdbm,
    jenkinsOaat,
} from "cap:fnv";

test("1. IETF FNV-1a & FNV-1 32-bit official test vectors", () => {
    expect_eq(FNV32_OFFSET_BASIS, 0x811C9DC5 as u32);
    expect_eq(FNV32_PRIME, 0x01000193 as u32);

    const chongo = "chongo <Landon Curt Noll> /\\../\\";

    expect_eq(fnv1a32(""), 0x811C9DC5 as u32);
    expect_eq(fnv1a32("a"), 0xE40C292C as u32);
    expect_eq(fnv1a32("foobar"), 0xBF9CF968 as u32);
    expect_eq(fnv1a32(chongo), 0x9A4E92E6 as u32);

    expect_eq(fnv1_32(""), 0x811C9DC5 as u32);
    expect_eq(fnv1_32("a"), 0x050C5D7E as u32);
    expect_eq(fnv1_32("foobar"), 0x31F0B262 as u32);
    expect_eq(fnv1_32(chongo), 0x995FA9C4 as u32);
});

fn u128Pair(hi: u64, lo: u64): u128 {
    return ((hi as u128) << 64) | (lo as u128);
}

test("2. IETF FNV-1a & FNV-1 64-bit & 128-bit official test vectors", () => {
    expect_eq(FNV64_OFFSET_BASIS, 0xCBF29CE484222325 as u64);
    expect_eq(FNV64_PRIME, 0x00000100000001B3 as u64);
    expect_eq(FNV128_OFFSET_BASIS, u128Pair(0x6C62272E07BB0142, 0x62B821756295C58D));
    expect_eq(FNV128_PRIME, u128Pair(0x0000000001000000, 0x000000000000013B));

    const chongo = "chongo <Landon Curt Noll> /\\../\\";

    // 64-bit FNV-1a & FNV-1
    expect_eq(fnv1a64(""), 0xCBF29CE484222325 as u64);
    expect_eq(fnv1a64("a"), 0xAF63DC4C8601EC8C as u64);
    expect_eq(fnv1a64("foobar"), 0x85944171F73967E8 as u64);
    expect_eq(fnv1a64(chongo), 0x2C8F4C9AF81BCF06 as u64);

    expect_eq(fnv1_64(""), 0xCBF29CE484222325 as u64);
    expect_eq(fnv1_64("a"), 0xAF63BD4C8601B7BE as u64);
    expect_eq(fnv1_64("foobar"), 0x340D8765A4DDA9C2 as u64);
    expect_eq(fnv1_64(chongo), 0x8FD0680DA3088A04 as u64);

    // 128-bit FNV-1a & FNV-1
    expect_eq(fnv1a128(""), u128Pair(0x6C62272E07BB0142, 0x62B821756295C58D));
    expect_eq(fnv1a128("a"), u128Pair(0xD228CB696F1A8CAF, 0x78912B704E4A8964));
    expect_eq(fnv1a128("foobar"), u128Pair(0x343E1662793C64BF, 0x6F0D3597BA446F18));
    expect_eq(fnv1a128(chongo), u128Pair(0x51392A3B0D394195, 0x533727930C41B74E));

    expect_eq(fnv1_128(""), u128Pair(0x6C62272E07BB0142, 0x62B821756295C58D));
    expect_eq(fnv1_128("a"), u128Pair(0xD228CB69101A8CAF, 0x78912B704E4A141E));
    expect_eq(fnv1_128("foobar"), u128Pair(0x7896BFEA9C3C64BF, 0x6DC58353D2C293AA));
    expect_eq(fnv1_128(chongo), u128Pair(0xF07E80295B7EC5ED, 0x254A4C20BFBEAB04));
});

test("3. Streaming chunked update & updateByte equivalence across all 6 FNV hashers", () => {
    const full = "chongo <Landon Curt Noll> /\\../\\";

    let h1a32 = Fnv1a32.init();
    h1a32.update("chongo ");
    h1a32.updateByte(0x3C); // '<'
    h1a32.update("Landon Curt Noll> /\\../\\");
    expect_eq(h1a32.finish(), fnv1a32(full));

    let h1a64 = Fnv1a64.init();
    h1a64.update("chongo <Landon ");
    h1a64.update("Curt Noll> ");
    h1a64.update("/\\../\\");
    expect_eq(h1a64.finish(), fnv1a64(full));

    let h1a128 = Fnv1a128.init();
    let i: usize = 0;
    while (i < full.len) {
        h1a128.updateByte(full[i]);
        i += 1;
    }
    expect_eq(h1a128.finish(), fnv1a128(full));

    let h1_32 = Fnv1_32.init();
    h1_32.update("foo");
    h1_32.update("bar");
    expect_eq(h1_32.finish(), fnv1_32("foobar"));

    let h1_64 = Fnv1_64.init();
    h1_64.update("foo");
    h1_64.updateByte(0x62); // 'b'
    h1_64.update("ar");
    expect_eq(h1_64.finish(), fnv1_64("foobar"));

    let h1_128 = Fnv1_128.init();
    h1_128.update("f");
    h1_128.update("ooba");
    h1_128.update("r");
    expect_eq(h1_128.finish(), fnv1_128("foobar"));
});

test("4. fnvXorFold32 official XOR-folding bit-width invariants & bounds rejection", () => {
    const h = fnv1a64("foobar"); // 0x85944171F73967E8

    // 32-bit fold: (h >> 32) ^ (h & 0xFFFFFFFF)
    const f32 = fnvXorFold32(h, 32) ?? unreachable;
    expect_eq(f32, ((h >> 32) ^ (h & 0xFFFFFFFF)));

    // 16-bit & 1-bit folds stay strictly within [0, 2^bits - 1]
    const f16 = fnvXorFold32(h, 16) ?? unreachable;
    expect_eq(f16 <= 0xFFFF, true);

    const f1 = fnvXorFold32(h, 1) ?? unreachable;
    expect_eq(f1 <= 1, true);

    const f63 = fnvXorFold32(h, 63) ?? unreachable;
    expect_eq(f63 <= 0x7FFFFFFFFFFFFFFF, true);

    // Invalid widths (0, 64, 128) rejected
    expect_eq(fnvXorFold32(h, 0) == null, true);
    expect_eq(fnvXorFold32(h, 64) == null, true);
    expect_eq(fnvXorFold32(h, 128) == null, true);
});

test("5. Classic DJB2, DJB2-XOR, SDBM & Jenkins OAAT known reference vectors", () => {
    // DJB2: "" -> 5381, "a" -> 5381 * 33 + 97 = 177670
    expect_eq(djb2(""), 5381 as u32);
    expect_eq(djb2("a"), 177670 as u32);
    expect_eq(djb2("hello"), 0x0F923099 as u32);

    // DJB2-XOR: "" -> 5381, "a" -> (5381 * 33) ^ 97 = 177620
    expect_eq(djb2Xor(""), 5381 as u32);
    expect_eq(djb2Xor("a"), 177620 as u32);

    // SDBM: "" -> 0, "a" -> 97, "ab" -> 97 * 65599 + 98 = 6363201
    expect_eq(sdbm(""), 0 as u32);
    expect_eq(sdbm("a"), 97 as u32);
    expect_eq(sdbm("ab"), 6363201 as u32);

    // Bob Jenkins One-at-a-Time official vectors: "" -> 0, "a" -> 0xCA2E9442
    expect_eq(jenkinsOaat(""), 0 as u32);
    expect_eq(jenkinsOaat("a"), 0xCA2E9442 as u32);
    expect_eq(jenkinsOaat("The quick brown fox jumps over the lazy dog"), 0x519E91F5 as u32);
});
