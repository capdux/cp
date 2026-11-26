// cap/test/testCapSsri.cp
// Comprehensive unit tests for @cap/ssri

import { test, expect_eq } from "cap:test";
import {
    SriAlgo,
    SriHash,
    algoStrength,
    parseIntegrity,
    strongestHash,
    pickAlgorithm,
    matchesIntegrity,
    formatHash,
    formatIntegrity,
} from "../../packages/@cap/ssri/src/index.cp";

fn main() {
    test("1. @cap/ssri — Single & multi-algorithm parsing with ?options", () => {
        const sri = parseIntegrity("sha256-47DEQpj8HBSa+/TImW+5JCeuQeRkm5NMpJWZG3hSuFU=  sha512-z4PhNX7vuL3xVChQ1m2AB9Yg5AULVxXcg/SpIdNs6c5H0NE8XYXysP+DGNKHfuwvY7kxvUdBeoGlODJ6+SfaPg==?ct=application/js");
        expect_eq(sri.count, 2 as usize);
        expect_eq(sri.isEmpty(), false);

        expect_eq(sri.hashes[0].valid, true);
        expect_eq(sri.hashes[0].algo == SriAlgo.Sha256, true);
        expect_eq(sri.hashes[0].algo_name, "sha256");
        expect_eq(sri.hashes[0].digest, "47DEQpj8HBSa+/TImW+5JCeuQeRkm5NMpJWZG3hSuFU=");
        expect_eq(sri.hashes[0].options, "");

        expect_eq(sri.hashes[1].valid, true);
        expect_eq(sri.hashes[1].algo == SriAlgo.Sha512, true);
        expect_eq(sri.hashes[1].algo_name, "sha512");
        expect_eq(sri.hashes[1].options, "ct=application/js");
        expect_eq(sri.get(99 as usize) == null, true);
    });

    test("2. @cap/ssri — strongestHash & algoStrength priority (Sha512 > Sha384 > Blake3 > Sha256)", () => {
        expect_eq(algoStrength(SriAlgo.Sha512) > algoStrength(SriAlgo.Sha384), true);
        expect_eq(algoStrength(SriAlgo.Sha384) > algoStrength(SriAlgo.Blake3), true);
        expect_eq(algoStrength(SriAlgo.Blake3) > algoStrength(SriAlgo.Sha256), true);
        expect_eq(algoStrength(SriAlgo.Sha256) > algoStrength(SriAlgo.Unknown), true);

        const sri1 = parseIntegrity("sha256-AAAA blake3-BBBB sha384-CCCC");
        const best1 = strongestHash(sri1);
        expect_eq(best1 != null, true);
        const h1 = best1 ?? SriHash{ valid: false, algo: SriAlgo.Unknown, algo_name: "", digest: "", options: "" };
        expect_eq(h1.algo == SriAlgo.Sha384, true);
        expect_eq(h1.digest, "CCCC");

        const sri2 = parseIntegrity("sha256-AAAA sha512-DDDD sha384-CCCC");
        const best2 = strongestHash(sri2);
        const h2 = best2 ?? SriHash{ valid: false, algo: SriAlgo.Unknown, algo_name: "", digest: "", options: "" };
        expect_eq(h2.algo == SriAlgo.Sha512, true);
        expect_eq(h2.digest, "DDDD");

        const onlyUnknown = parseIntegrity("md5-AAAA");
        expect_eq(onlyUnknown.count, 1 as usize);
        expect_eq(strongestHash(onlyUnknown) == null, true);
    });

    test("3. @cap/ssri — pickAlgorithm lookup", () => {
        const sri = parseIntegrity("SHA256-dGVzdA== blake3-YWJjZA==");
        const p256 = pickAlgorithm(sri, SriAlgo.Sha256);
        expect_eq(p256 != null, true);
        const h256 = p256 ?? SriHash{ valid: false, algo: SriAlgo.Unknown, algo_name: "", digest: "", options: "" };
        expect_eq(h256.digest, "dGVzdA==");
        expect_eq(h256.algo_name, "sha256");

        const pB3 = pickAlgorithm(sri, SriAlgo.Blake3);
        expect_eq(pB3 != null, true);
        const hB3 = pB3 ?? SriHash{ valid: false, algo: SriAlgo.Unknown, algo_name: "", digest: "", options: "" };
        expect_eq(hB3.digest, "YWJjZA==");

        expect_eq(pickAlgorithm(sri, SriAlgo.Sha512) == null, true);
    });

    test("4. @cap/ssri — matchesIntegrity constant-time verification", () => {
        const expected = parseIntegrity("sha256-47DEQpj8HBSa+/TImW+5JCeuQeRkm5NMpJWZG3hSuFU= sha512-z4PhNX7vuL3xVChQ1m2AB9Yg5AULVxXcg/SpIdNs6c5H0NE8XYXysP+DGNKHfuwvY7kxvUdBeoGlODJ6+SfaPg==");
        const actualMatch = parseIntegrity("sha256-47DEQpj8HBSa+/TImW+5JCeuQeRkm5NMpJWZG3hSuFU=");
        const actualWrong = parseIntegrity("sha256-47DEQpj8HBSa+/TImW+5JCeuQeRkm5NMpJWZG3hSuFV=");
        const actualOtherAlgo = parseIntegrity("sha384-47DEQpj8HBSa+/TImW+5JCeuQeRkm5NMpJWZG3hSuFU=");

        expect_eq(matchesIntegrity(expected, actualMatch), true);
        expect_eq(matchesIntegrity(expected, actualWrong), false);
        expect_eq(matchesIntegrity(expected, actualOtherAlgo), false);
        expect_eq(matchesIntegrity(parseIntegrity("md5-AAAA"), parseIntegrity("md5-AAAA")), false);
    });

    test("5. @cap/ssri — formatHash & formatIntegrity canonical output", () => {
        let buf1: [128]u8 = undefined;
        expect_eq(formatHash(SriAlgo.Sha256, "dGVzdA==", buf1[0..128]), "sha256-dGVzdA==");

        let buf2: [128]u8 = undefined;
        expect_eq(formatHash(SriAlgo.Unknown, "dGVzdA==", buf2[0..128]), "");

        let buf3: [128]u8 = undefined;
        expect_eq(formatHash(SriAlgo.Sha512, "bad!digest", buf3[0..128]), "");

        const parsed = parseIntegrity("  SHA256-AAAA   sha512-BBBB?foo=bar  ");
        let buf4: [128]u8 = undefined;
        expect_eq(formatIntegrity(parsed, buf4[0..128]), "sha256-AAAA sha512-BBBB?foo=bar");
    });

    test("6. @cap/ssri — Reject invalid Base64, empty strings & malformed tokens", () => {
        expect_eq(parseIntegrity("").count, 0 as usize);
        expect_eq(parseIntegrity("   \t\n  ").count, 0 as usize);
        expect_eq(parseIntegrity("no-dash-here!").count, 0 as usize);
        expect_eq(parseIntegrity("-AAAA").count, 0 as usize);
        expect_eq(parseIntegrity("sha256-").count, 0 as usize);
        expect_eq(parseIntegrity("sha256-not!valid@base64").count, 0 as usize);
        expect_eq(parseIntegrity("sha256-AA=A").count, 0 as usize);
        expect_eq(parseIntegrity("sha256-A===").count, 0 as usize);
        expect_eq(parseIntegrity("sha256-AAAA?").count, 0 as usize);

        // Valid entry survives alongside malformed tokens
        const mixed = parseIntegrity("badtoken sha256-!invalid sha384-dGVzdA==");
        expect_eq(mixed.count, 1 as usize);
        expect_eq(mixed.hashes[0].algo == SriAlgo.Sha384, true);
    });
}
