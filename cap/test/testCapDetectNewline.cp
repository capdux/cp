import { test, expect_eq } from "cap:test";
import {
    EOL_NONE,
    EOL_LF,
    EOL_CRLF,
    EOL_CR,
    NewlineStats,
    analyzeNewlines,
    detectNewline,
    detectNewlineGraceful,
    normalizeToLf,
    normalizeToCrlf,
} from "../../packages/@cap/detect-newline/src/index.cp";

fn eqStr(buf: []const u8, len: usize, expected: string): bool {
    if (len != expected.len) {
        return false;
    }
    let i: usize = 0;
    while (i < len) {
        if (buf[i] != expected[i]) {
            return false;
        }
        i = i + 1;
    }
    return true;
}

fn main() {
    test("1. @cap/detect-newline — pure LF & pure CRLF detection", () => {
        let lf_s: string = "a\nb\nc\n";
        let st1: NewlineStats = analyzeNewlines(lf_s);
        expect_eq(st1.lf_count, 3 as usize);
        expect_eq(st1.crlf_count, 0 as usize);
        expect_eq(st1.dominant, EOL_LF);
        expect_eq(detectNewline(lf_s), "\n");

        let crlf_s: string = "a\r\nb\r\n";
        let st2: NewlineStats = analyzeNewlines(crlf_s);
        expect_eq(st2.crlf_count, 2 as usize);
        expect_eq(st2.lf_count, 0 as usize);
        expect_eq(st2.dominant, EOL_CRLF);
        expect_eq(detectNewline(crlf_s), "\r\n");
    });

    test("2. @cap/detect-newline — mixed newlines resolved by majority & tie-break", () => {
        // 2 CRLF vs 1 LF -> CRLF wins
        expect_eq(detectNewline("a\r\nb\r\nc\n"), "\r\n");
        // 2 LF vs 1 CRLF -> LF wins
        expect_eq(detectNewline("a\nb\nc\r\n"), "\n");
        // Tie (1 LF vs 1 CRLF) -> LF wins (detect-newline convention)
        expect_eq(detectNewline("a\nb\r\n"), "\n");
    });

    test("3. @cap/detect-newline — no newline: detectNewline='' vs graceful='\\n'", () => {
        let plain: string = "single line without newline";
        let st: NewlineStats = analyzeNewlines(plain);
        expect_eq(st.dominant, EOL_NONE);
        expect_eq(detectNewline(plain), "");
        expect_eq(detectNewlineGraceful(plain), "\n");
        expect_eq(detectNewlineGraceful("x\r\ny"), "\r\n");
    });

    test("4. @cap/detect-newline — normalizeToLf & normalizeToCrlf without double \\r\\r\\n", () => {
        let buf: [64]u8 = [0; 64];
        let buf2: [64]u8 = [0; 64];

        let mixed: string = "a\r\nb\nc\rd";
        let n_lf: usize = normalizeToLf(mixed, buf[0..64]);
        expect_eq(eqStr(buf[0..64], n_lf, "a\nb\nc\nd"), true);

        let n_crlf: usize = normalizeToCrlf(mixed, buf[0..64]);
        expect_eq(eqStr(buf[0..64], n_crlf, "a\r\nb\r\nc\r\nd"), true);

        // Idempotent: normalizing CRLF to CRLF again must NOT produce \r\r\n
        let n_again: usize = normalizeToCrlf(buf[0..n_crlf], buf2[0..64]);
        expect_eq(eqStr(buf2[0..64], n_again, "a\r\nb\r\nc\r\nd"), true);
    });

    test("5. @cap/detect-newline — classic Mac \\r counted accurately", () => {
        let mac: string = "line1\rline2\rline3";
        let st: NewlineStats = analyzeNewlines(mac);
        expect_eq(st.cr_count, 2 as usize);
        expect_eq(st.lf_count, 0 as usize);
        expect_eq(st.crlf_count, 0 as usize);
        expect_eq(st.dominant, EOL_CR);
        expect_eq(detectNewline(mac), "");
        expect_eq(detectNewlineGraceful(mac), "\n");
    });

    test("6. @cap/detect-newline — empty input & small buffer safety", () => {
        let tiny: [3]u8 = [0; 3];
        expect_eq(analyzeNewlines("").dominant, EOL_NONE);
        expect_eq(detectNewline(""), "");
        expect_eq(detectNewlineGraceful(""), "\n");
        expect_eq(normalizeToLf("", tiny[0..3]), 0 as usize);
        expect_eq(normalizeToCrlf("", tiny[0..3]), 0 as usize);

        // Buffer too small returns 0 safely
        expect_eq(normalizeToLf("hello\r\nworld", tiny[0..3]), 0 as usize);
        expect_eq(normalizeToCrlf("a\nb\n", tiny[0..3]), 0 as usize);
    });
}
