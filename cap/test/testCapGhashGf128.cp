import { test, expect_eq } from "cap:test";
import {
    Block128,
    Ghash,
    gf128Mul,
} from "../../packages/universal-hash/src/index.cp";

fn bytesEq16(a: []const u8, b: []const u8): bool {
    if (a.len < 16 || b.len < 16) {
        return false;
    }
    let i: usize = 0;
    while (i < 16) {
        if (a[i] != b[i]) {
            return false;
        }
        i = i + 1;
    }
    return true;
}

fn main() {
    test("1. @cap/ghash-gf128 — gf128Mul commutativity, associativity & distributivity", () => {
        let a: Block128 = Block128 {
            hi: ((0x66e94bd4 as u64) << 32) | (0xef8a2c3b as u64),
            lo: ((0x884cfa59 as u64) << 32) | (0xca342b2e as u64),
        };
        let b: Block128 = Block128 {
            hi: ((0x0388dace as u64) << 32) | (0x60b6a392 as u64),
            lo: ((0xf328c2b9 as u64) << 32) | (0x71b2fe78 as u64),
        };
        let c: Block128 = Block128 {
            hi: ((0x12345678 as u64) << 32) | (0x9abcdef0 as u64),
            lo: ((0xfedcba98 as u64) << 32) | (0x76543210 as u64),
        };

        // Commutativity: A * B == B * A
        expect_eq(gf128Mul(a, b).eq(gf128Mul(b, a)), true);
        // Associativity: (A * B) * C == A * (B * C)
        expect_eq(gf128Mul(gf128Mul(a, b), c).eq(gf128Mul(a, gf128Mul(b, c))), true);
        // Distributivity: A * (B ^ C) == (A * B) ^ (A * C)
        let lhs: Block128 = gf128Mul(a, b.xor(c));
        let rhs: Block128 = gf128Mul(a, b).xor(gf128Mul(a, c));
        expect_eq(lhs.eq(rhs), true);
    });

    test("2. @cap/ghash-gf128 — zero element & multiplicative identity 0x8000...0000", () => {
        let a: Block128 = Block128 {
            hi: ((0xdeadbeef as u64) << 32) | (0xcafebabe as u64),
            lo: ((0x01234567 as u64) << 32) | (0x89abcdef as u64),
        };
        let z: Block128 = Block128.zero();
        let one_b: Block128 = Block128.one();

        expect_eq(gf128Mul(a, z).eq(z), true);
        expect_eq(gf128Mul(z, a).eq(z), true);
        expect_eq(gf128Mul(a, one_b).eq(a), true);
        expect_eq(gf128Mul(one_b, a).eq(a), true);
    });

    test("3. @cap/ghash-gf128 — NIST SP 800-38D GCM Test Case 1 & Test Case 2 vectors", () => {
        // H = AES_0(0^128) = 66e94bd4 ef8a2c3b 884cfa59 ca342b2e
        let h_bytes: [16]u8 = [
            0x66, 0xe9, 0x4b, 0xd4, 0xef, 0x8a, 0x2c, 0x3b,
            0x88, 0x4c, 0xfa, 0x59, 0xca, 0x34, 0x2b, 0x2e,
        ];
        // Test Case 1: empty AAD & empty CT -> GHASH == 0^128
        let g1: Ghash = Ghash.new(h_bytes[0..16]);
        g1.updateLengths(0, 0);
        let tag1: [16]u8 = [0; 16];
        let zeros: [16]u8 = [0; 16];
        g1.finalize(tag1[0..16]);
        expect_eq(bytesEq16(tag1[0..16], zeros[0..16]), true);

        // Test Case 2: CT = 0388dace60b6a392f328c2b971b2fe78, len(AAD)=0, len(CT)=16 bytes
        // Expected GHASH = f38cbb1ad69223dcc3457ae5b6b0f885
        let ct: [16]u8 = [
            0x03, 0x88, 0xda, 0xce, 0x60, 0xb6, 0xa3, 0x92,
            0xf3, 0x28, 0xc2, 0xb9, 0x71, 0xb2, 0xfe, 0x78,
        ];
        let expected: [16]u8 = [
            0xf3, 0x8c, 0xbb, 0x1a, 0xd6, 0x92, 0x23, 0xdc,
            0xc3, 0x45, 0x7a, 0xe5, 0xb6, 0xb0, 0xf8, 0x85,
        ];
        let g2: Ghash = Ghash.new(h_bytes[0..16]);
        g2.updateBlock(ct[0..16]);
        g2.updateLengths(0, 16);
        let tag2: [16]u8 = [0; 16];
        g2.finalize(tag2[0..16]);
        expect_eq(bytesEq16(tag2[0..16], expected[0..16]), true);
    });

    test("4. @cap/ghash-gf128 — streaming updatePadded + updateLengths", () => {
        let h_bytes: [16]u8 = [
            0x66, 0xe9, 0x4b, 0xd4, 0xef, 0x8a, 0x2c, 0x3b,
            0x88, 0x4c, 0xfa, 0x59, 0xca, 0x34, 0x2b, 0x2e,
        ];
        let aad: [5]u8 = [1, 2, 3, 4, 5];
        let ct: [20]u8 = [
            10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
            20, 21, 22, 23, 24, 25, 26, 27, 28, 29,
        ];

        // Compute via updatePadded
        let g_auto: Ghash = Ghash.new(h_bytes[0..16]);
        g_auto.updatePadded(aad[0..5]);
        g_auto.updatePadded(ct[0..20]);
        g_auto.updateLengths(5, 20);
        let out_auto: [16]u8 = [0; 16];
        g_auto.finalize(out_auto[0..16]);

        // Compute manually with explicit zero-padded blocks
        let aad_blk: [16]u8 = [1, 2, 3, 4, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0];
        let ct_blk2: [16]u8 = [26, 27, 28, 29, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0];
        let g_man: Ghash = Ghash.new(h_bytes[0..16]);
        g_man.updateBlock(aad_blk[0..16]);
        g_man.updateBlock(ct[0..16]);
        g_man.updateBlock(ct_blk2[0..16]);
        g_man.updateLengths(5, 20);
        let out_man: [16]u8 = [0; 16];
        g_man.finalize(out_man[0..16]);

        expect_eq(bytesEq16(out_auto[0..16], out_man[0..16]), true);
    });

    test("5. @cap/ghash-gf128 — single-bit flip changes authentication tag", () => {
        let h_bytes: [16]u8 = [
            0x66, 0xe9, 0x4b, 0xd4, 0xef, 0x8a, 0x2c, 0x3b,
            0x88, 0x4c, 0xfa, 0x59, 0xca, 0x34, 0x2b, 0x2e,
        ];
        let msg: [16]u8 = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15];
        let g1: Ghash = Ghash.new(h_bytes[0..16]);
        g1.updateBlock(msg[0..16]);
        g1.updateLengths(0, 16);
        let t1: [16]u8 = [0; 16];
        g1.finalize(t1[0..16]);

        msg[15] = msg[15] ^ 1;
        let g2: Ghash = Ghash.new(h_bytes[0..16]);
        g2.updateBlock(msg[0..16]);
        g2.updateLengths(0, 16);
        let t2: [16]u8 = [0; 16];
        g2.finalize(t2[0..16]);

        expect_eq(bytesEq16(t1[0..16], t2[0..16]), false);
    });

    test("6. @cap/ghash-gf128 — reset & empty updatePadded boundary", () => {
        let h_bytes: [16]u8 = [
            0x66, 0xe9, 0x4b, 0xd4, 0xef, 0x8a, 0x2c, 0x3b,
            0x88, 0x4c, 0xfa, 0x59, 0xca, 0x34, 0x2b, 0x2e,
        ];
        let ct: [16]u8 = [
            0x03, 0x88, 0xda, 0xce, 0x60, 0xb6, 0xa3, 0x92,
            0xf3, 0x28, 0xc2, 0xb9, 0x71, 0xb2, 0xfe, 0x78,
        ];
        let g: Ghash = Ghash.new(h_bytes[0..16]);
        g.updatePadded(ct[0..0]);
        g.updateBlock(ct[0..16]);
        g.updateLengths(0, 16);
        let t1: [16]u8 = [0; 16];
        g.finalize(t1[0..16]);

        g.reset();
        expect_eq(g.y.eq(Block128.zero()), true);
        g.updateBlock(ct[0..16]);
        g.updateLengths(0, 16);
        let t2: [16]u8 = [0; 16];
        g.finalize(t2[0..16]);
        expect_eq(bytesEq16(t1[0..16], t2[0..16]), true);
    });
}
