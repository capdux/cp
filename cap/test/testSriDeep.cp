// cap/test/testSriDeep.cp
// Deep unit tests for cap:sri (Rust `ssri` / W3C SRI §3.3.3 parity: IntegrityList multi-hash sets,
// strongest-algorithm downgrade prevention, toHex, ?options stripping, and streaming IntegrityVerifier).

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    IntegrityTag,
    Integrity,
    IntegrityList,
    IntegrityVerifier,
} from "cap:sri";

fn main() {
    test("1. cap:sri — Compute, Format, Parse & toHex Roundtrip", () => {
        const data = "hello sri";
        const d256 = Integrity.compute(IntegrityTag.sha256, data) catch unreachable;
        expect_eq(d256.verify(data), true);
        expect_eq(d256.verify("hello srj"), false);

        let fmt_buf: [128]u8 = [0; 128];
        const formatted = d256.format(fmt_buf[0..]) catch unreachable;
        expect_eq(std.mem.startsWith(u8, formatted, "sha256-"), true);

        const parsed = Integrity.parse(formatted) catch unreachable;
        expect_eq(parsed.eql(d256), true);

        let hex_buf: [128]u8 = [0; 128];
        const hex = d256.toHex(hex_buf[0..]) catch unreachable;
        expect_eq(hex.len, 64 as usize);
    });

    test("2. cap:sri — W3C SRI §3.3.3 IntegrityList Multi-Candidate & Anti-Downgrade", () => {
        const d512_a = Integrity.compute(IntegrityTag.sha512, "release-v1") catch unreachable;
        const d512_b = Integrity.compute(IntegrityTag.sha512, "release-v2") catch unreachable;
        const d256_only = Integrity.compute(IntegrityTag.sha256, "downgraded") catch unreachable;

        let list = IntegrityList{};
        expect_eq(list.push(d256_only), true);
        expect_eq(list.push(d512_a), true);
        expect_eq(list.push(d512_b), true);
        expect_eq(list.push(d512_b), true); // deduplicated
        expect_eq(list.len, 3 as usize);
        expect_eq(list.strongestTag() == IntegrityTag.sha512, true);

        // Both strongest-algorithm digests verify true
        expect_eq(list.verify("release-v1"), true);
        expect_eq(list.verify("release-v2"), true);
        // Weaker sha256 match MUST NOT bypass stronger sha512 requirement
        expect_eq(list.verify("downgraded"), false);

        // Format and re-parse full list
        let out: [512]u8 = [0; 512];
        const wire = list.format(out[0..]) catch unreachable;
        const reparsed = IntegrityList.parse(wire) catch unreachable;
        expect_eq(reparsed.len, 3 as usize);
        expect_eq(reparsed.verify("release-v2"), true);
    });

    test("3. cap:sri — Streaming IntegrityVerifier & ?options Suffix", () => {
        const full = "chunk1-chunk2-chunk3";
        const d384 = Integrity.compute(IntegrityTag.sha384, full) catch unreachable;

        let v = IntegrityVerifier.init(d384);
        v.update("chunk1-");
        v.update("chunk2-");
        v.update("chunk3");
        expect_eq(v.verify(), true);

        let buf: [160]u8 = [0; 160];
        const s = d384.format(buf[0..128]) catch unreachable;
        let with_opt: [160]u8 = [0; 160];
        @memcpy(with_opt[0..s.len], s);
        const opt = "?ct=application/javascript";
        @memcpy(with_opt[s.len .. s.len + opt.len], opt);

        const parsed = Integrity.parse(with_opt[0 .. s.len + opt.len]) catch unreachable;
        expect_eq(parsed.verify(full), true);
    });
}
