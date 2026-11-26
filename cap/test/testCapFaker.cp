// cap/test/testCapFaker.cp
// Comprehensive unit tests for @cap/faker:
// deterministic seeding & reseed, cross-seed divergence, RFC 4122 UUID v4,
// IPv4/IPv6/Email/URL/Phone/HexColor/ISO Date format validation,
// Luhn credit card checksum, range boundaries, and Fisher-Yates / Floyd sampling.

const std = @import("std");
import { test, expect_eq } from "cap:test";
import { Faker, isValidLuhn } from "../../packages/@cap/faker/src/index.cp";

fn isHexLower(c: u8): bool {
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f');
}

fn isDigit(c: u8): bool {
    return c >= '0' && c <= '9';
}

fn validateIpv4(s: []const u8): bool {
    let dots: usize = 0;
    let cur: u32 = 0;
    let digits_in_octet: usize = 0;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        const c = s[i];
        if (c == '.') {
            if (digits_in_octet == 0 || cur > 255) return false;
            dots += 1;
            cur = 0;
            digits_in_octet = 0;
        } else if (isDigit(c)) {
            cur = cur * 10 + ((c - '0') as u32);
            digits_in_octet += 1;
            if (digits_in_octet > 3 || cur > 255) return false;
        } else {
            return false;
        }
    }
    return dots == 3 && digits_in_octet > 0 && cur <= 255;
}

fn validateUuidV4(s: []const u8): bool {
    if (s.len != 36) return false;
    let i: usize = 0;
    while (i < 36) : (i += 1) {
        const c = s[i];
        if (i == 8 || i == 13 || i == 18 || i == 23) {
            if (c != '-') return false;
        } else if (!isHexLower(c)) {
            return false;
        }
    }
    // RFC 4122 version 4 at index 14
    if (s[14] != '4') return false;
    // RFC 4122 variant 1 (8, 9, a, b) at index 19
    const v = s[19];
    if (v != '8' && v != '9' && v != 'a' && v != 'b') return false;
    return true;
}

fn main() {
    test("1. @cap/faker — Deterministic Same-Seed Reproduction & Reseed", () => {
        let f1 = Faker.init(42);
        let f2 = Faker.init(42);

        let i: usize = 0;
        while (i < 16) : (i += 1) {
            expect_eq(f1.nextU64(), f2.nextU64());
        }

        let b1: [64]u8 = undefined;
        let b2: [64]u8 = undefined;
        expect_eq(std.mem.eql(u8, f1.fullName(&b1), f2.fullName(&b2)), true);
        expect_eq(std.mem.eql(u8, f1.email(&b1), f2.email(&b2)), true);
        expect_eq(std.mem.eql(u8, f1.uuidV4(&b1), f2.uuidV4(&b2)), true);

        // Reseed restores identical sequence from the start
        f1.reseed(42);
        let f3 = Faker.init(42);
        expect_eq(f1.nextU64(), f3.nextU64());
        expect_eq(std.mem.eql(u8, f1.firstName(), f3.firstName()), true);
        expect_eq(std.mem.eql(u8, f1.lastName(), f3.lastName()), true);
    });

    test("2. @cap/faker — Distinct Seeds Produce Divergent Outputs", () => {
        let fa = Faker.init(1001);
        let fb = Faker.init(2002);
        let fc = Faker.init(0);

        const va = fa.nextU64();
        const vb = fb.nextU64();
        const vc = fc.nextU64();
        expect_eq(va != vb, true);
        expect_eq(va != vc, true);
        expect_eq(vb != vc, true);

        let ua: [40]u8 = undefined;
        let ub: [40]u8 = undefined;
        expect_eq(std.mem.eql(u8, fa.uuidV4(&ua), fb.uuidV4(&ub)), false);
    });

    test("3. @cap/faker — RFC 4122 UUID v4 & Hex Color Validation", () => {
        let f = Faker.init(987654321);
        let uuid_buf: [40]u8 = undefined;
        let hex_buf: [16]u8 = undefined;

        let i: usize = 0;
        while (i < 32) : (i += 1) {
            const u = f.uuidV4(&uuid_buf);
            expect_eq(validateUuidV4(u), true);

            const col = f.hexColor(&hex_buf);
            expect_eq(col.len, 7 as usize);
            expect_eq(col[0] == '#', true);
            let k: usize = 1;
            while (k < 7) : (k += 1) {
                expect_eq(isHexLower(col[k]), true);
            }
        }
    });

    test("4. @cap/faker — IPv4, IPv6, Email, URL, Phone & ISO Date Formats", () => {
        let f = Faker.init(777);
        let buf: [128]u8 = undefined;

        let i: usize = 0;
        while (i < 20) : (i += 1) {
            const ip4 = f.ipv4(&buf);
            expect_eq(validateIpv4(ip4), true);

            const ip6 = f.ipv6(&buf);
            expect_eq(ip6.len, 39 as usize);
            expect_eq(ip6[4] == ':' && ip6[9] == ':' && ip6[34] == ':', true);

            const em = f.email(&buf);
            const at_pos: usize? = std.mem.indexOfScalar(u8, em, '@');
            expect_eq(at_pos != null, true);
            const dot_pos: usize? = std.mem.lastIndexOfScalar(u8, em, '.');
            expect_eq(dot_pos != null, true);

            const u = f.url(&buf);
            expect_eq(std.mem.startsWith(u8, u, "https://"), true);

            const dom = f.domain(&buf);
            expect_eq(std.mem.indexOfScalar(u8, dom, '.') != null, true);

            const ph = f.phone(&buf);
            expect_eq(ph.len >= 12 && ph.len <= 15, true);
            expect_eq(ph[0] == '+', true);
            let d: usize = 1;
            while (d < ph.len) : (d += 1) {
                expect_eq(isDigit(ph[d]), true);
            }

            const dt = f.isoDate(&buf);
            expect_eq(dt.len, 10 as usize);
            expect_eq(dt[4] == '-' && dt[7] == '-', true);
        }
    });

    test("5. @cap/faker — Luhn Credit Card Checksum Validation", () => {
        let f = Faker.init(314159);
        let card_buf: [24]u8 = undefined;

        let i: usize = 0;
        while (i < 50) : (i += 1) {
            const card = f.creditCardNumber(&card_buf);
            expect_eq(card.len, 16 as usize);
            expect_eq(isValidLuhn(card), true);

            // Corrupting the last digit must invalidate the Luhn checksum
            let tampered: [16]u8 = undefined;
            let k: usize = 0;
            while (k < 16) : (k += 1) {
                tampered[k] = card[k];
            }
            tampered[15] = if (tampered[15] == '9') '0' else (tampered[15] + 1);
            expect_eq(isValidLuhn(&tampered), false);
        }
    });

    test("6. @cap/faker — Range Boundaries, Probabilities, Text & Collections", () => {
        let f = Faker.init(123456);

        // nextInt inclusive bounds including negative & degenerate ranges
        expect_eq(f.nextInt(5, 5), 5 as i64);
        expect_eq(f.nextInt(10, 3), 10 as i64);

        let i: usize = 0;
        while (i < 100) : (i += 1) {
            const v = f.nextInt(-25, 25);
            expect_eq(v >= -25 && v <= 25, true);
            expect_eq(f.nextBool(0), false);
            expect_eq(f.nextBool(100), true);
        }

        // pickStr
        const choices: [3][]const u8 = ["red", "green", "blue"];
        const picked = f.pickStr(&choices);
        const valid_choice = std.mem.eql(u8, picked, "red")
            || std.mem.eql(u8, picked, "green")
            || std.mem.eql(u8, picked, "blue");
        expect_eq(valid_choice, true);

        // Text & commerce
        let text_buf: [512]u8 = undefined;
        const sent = f.sentence(6, &text_buf);
        expect_eq(sent.len > 6, true);
        expect_eq(sent[0] >= 'A' && sent[0] <= 'Z', true);
        expect_eq(sent[sent.len - 1] == '.', true);

        const para = f.paragraph(3, &text_buf);
        expect_eq(para.len > 20, true);
        expect_eq(para[para.len - 1] == '.', true);

        const comp = f.company(&text_buf);
        expect_eq(comp.len > 5, true);
        expect_eq(f.city().len > 0, true);
        expect_eq(f.country().len > 0, true);

        // shuffleU64 preserves exact multiset sum & XOR
        let arr: [8]u64 = [1, 2, 3, 4, 5, 6, 7, 8];
        f.shuffleU64(&arr);
        let sum: u64 = 0;
        let xor_acc: u64 = 0;
        let idx: usize = 0;
        while (idx < 8) : (idx += 1) {
            sum += arr[idx];
            xor_acc ^= arr[idx];
        }
        expect_eq(sum, 36 as u64);
        expect_eq(xor_acc, (1 ^ 2 ^ 3 ^ 4 ^ 5 ^ 6 ^ 7 ^ 8) as u64);

        // sampleIndices produces unique in-range indices
        let sampled: [10]usize = undefined;
        const n = f.sampleIndices(50, &sampled);
        expect_eq(n, 10 as usize);
        let a: usize = 0;
        while (a < n) : (a += 1) {
            expect_eq(sampled[a] < 50, true);
            let b: usize = a + 1;
            while (b < n) : (b += 1) {
                expect_eq(sampled[a] != sampled[b], true);
            }
        }

        // Clamping when population < out.len
        let small: [6]usize = undefined;
        expect_eq(f.sampleIndices(3, &small), 3 as usize);
    });
}
