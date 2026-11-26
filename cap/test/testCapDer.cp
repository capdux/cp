// cap/test/testCapDer.cp
// Comprehensive unit tests for @cap/der (RustCrypto der / ITU-T X.690 parity)

import { test, expect_eq } from "cap:test";
import {
    TAG_BOOLEAN,
    TAG_INTEGER,
    TAG_OCTET_STRING,
    TAG_NULL,
    TAG_SEQUENCE,
    parseDerTlv,
    decodeDerBool,
    decodeDerU64,
    encodeDerTlv,
    encodeDerBool,
    encodeDerNull,
    encodeDerU64,
    encodeDerOctetString,
    encodeDerSequence,
} from "../../packages/@cap/der/src/index.cp";

fn main() {
    test("1. @cap/der — short-form & long-form (>= 128 bytes) TLV encode/decode", () => {
        let short_buf: [32]u8 = undefined;
        const s_enc = encodeDerOctetString("hello", short_buf[0..32]).?;
        expect_eq(s_enc.len, 7 as usize);
        expect_eq(s_enc[0], TAG_OCTET_STRING);
        expect_eq(s_enc[1], 5 as u8);

        const s_tlv = parseDerTlv(s_enc);
        expect_eq(s_tlv.valid, true);
        expect_eq(s_tlv.tag, TAG_OCTET_STRING);
        expect_eq(s_tlv.value, "hello");
        expect_eq(s_tlv.total_len, 7 as usize);

        // Long-form length: 200-byte OCTET STRING -> 04 81 C8 <200 bytes>
        let payload: [200]u8 = undefined;
        let i: usize = 0;
        while (i < 200) {
            payload[i] = 'X' as u8;
            i += 1;
        }
        let long_buf: [256]u8 = undefined;
        const l_enc = encodeDerOctetString(payload[0..200], long_buf[0..256]).?;
        expect_eq(l_enc.len, 203 as usize);
        expect_eq(l_enc[0], TAG_OCTET_STRING);
        expect_eq(l_enc[1], 0x81 as u8);
        expect_eq(l_enc[2], 200 as u8);

        const l_tlv = parseDerTlv(l_enc);
        expect_eq(l_tlv.valid, true);
        expect_eq(l_tlv.value.len, 200 as usize);
        expect_eq(l_tlv.total_len, 203 as usize);
    });

    test("2. @cap/der — strict rejection of indefinite, non-minimal, leading-zero & truncated lengths", () => {
        // Indefinite length 0x80 (BER only, forbidden in DER)
        let indef: [4]u8 = [0x30, 0x80, 0x00, 0x00];
        expect_eq(parseDerTlv(indef[0..4]).valid, false);

        // Non-minimal long form for length 5 (0x81 0x05)
        let non_min: [8]u8 = [0x04, 0x81, 0x05, 'a', 'b', 'c', 'd', 'e'];
        expect_eq(parseDerTlv(non_min[0..8]).valid, false);

        // Leading zero length octet (0x82 0x00 0x80)
        let lead_zero: [4]u8 = [0x04, 0x82, 0x00, 0x80];
        expect_eq(parseDerTlv(lead_zero[0..4]).valid, false);

        // Truncated payload (claims 10 bytes, only 2 present)
        let trunc: [4]u8 = [0x04, 0x0A, 0x01, 0x02];
        expect_eq(parseDerTlv(trunc[0..4]).valid, false);

        // Too short (< 2 bytes)
        expect_eq(parseDerTlv("").valid, false);
    });

    test("3. @cap/der — strict BOOLEAN (0x00 / 0xFF only) & NULL encoding", () => {
        let buf: [16]u8 = undefined;
        const t_enc = encodeDerBool(true, buf[0..16]).?;
        expect_eq(t_enc.len, 3 as usize);
        expect_eq(t_enc[2], 0xFF as u8);
        expect_eq(decodeDerBool(parseDerTlv(t_enc)).?, true);

        const f_enc = encodeDerBool(false, buf[0..16]).?;
        expect_eq(f_enc[2], 0x00 as u8);
        expect_eq(decodeDerBool(parseDerTlv(f_enc)).?, false);

        // Non-canonical BOOLEAN 0x01 must be rejected in DER
        let bad_bool: [3]u8 = [0x01, 0x01, 0x01];
        expect_eq(decodeDerBool(parseDerTlv(bad_bool[0..3])) == null, true);

        const n_enc = encodeDerNull(buf[0..16]).?;
        expect_eq(n_enc.len, 2 as usize);
        expect_eq(n_enc[0], TAG_NULL);
        expect_eq(n_enc[1], 0x00 as u8);
    });

    test("4. @cap/der — INTEGER high-bit 0x80 leading-zero padding & round-trip", () => {
        let buf: [32]u8 = undefined;

        // 0 -> 02 01 00
        const z = encodeDerU64(0 as u64, buf[0..32]).?;
        expect_eq(z.len, 3 as usize);
        expect_eq(z[2], 0x00 as u8);
        expect_eq(decodeDerU64(parseDerTlv(z)).?, 0 as u64);

        // 127 -> 02 01 7F (no padding needed)
        const v127 = encodeDerU64(127 as u64, buf[0..32]).?;
        expect_eq(v127.len, 3 as usize);
        expect_eq(v127[2], 0x7F as u8);
        expect_eq(decodeDerU64(parseDerTlv(v127)).?, 127 as u64);

        // 128 -> 02 02 00 80 (high bit set -> must prepend 0x00!)
        const v128 = encodeDerU64(128 as u64, buf[0..32]).?;
        expect_eq(v128.len, 4 as usize);
        expect_eq(v128[2], 0x00 as u8);
        expect_eq(v128[3], 0x80 as u8);
        expect_eq(decodeDerU64(parseDerTlv(v128)).?, 128 as u64);

        // u64::MAX -> 02 09 00 FF FF FF FF FF FF FF FF
        const max64: u64 = 0xFFFFFFFFFFFFFFFF as u64;
        const vmax = encodeDerU64(max64, buf[0..32]).?;
        expect_eq(vmax.len, 11 as usize);
        expect_eq(vmax[2], 0x00 as u8);
        expect_eq(decodeDerU64(parseDerTlv(vmax)).?, max64);
    });

    test("5. @cap/der — INTEGER rejects negative & non-minimal leading-zero encodings", () => {
        // Negative integer (-1 = 02 01 FF) rejected by decodeDerU64
        let neg: [3]u8 = [0x02, 0x01, 0xFF];
        expect_eq(decodeDerU64(parseDerTlv(neg[0..3])) == null, true);

        // Non-minimal leading zero (02 02 00 05) rejected by DER
        let non_min: [4]u8 = [0x02, 0x02, 0x00, 0x05];
        expect_eq(decodeDerU64(parseDerTlv(non_min[0..4])) == null, true);

        // Double zero (02 02 00 00) rejected
        let dbl_zero: [4]u8 = [0x02, 0x02, 0x00, 0x00];
        expect_eq(decodeDerU64(parseDerTlv(dbl_zero[0..4])) == null, true);
    });

    test("6. @cap/der — nested SEQUENCE encoding & sequential element parsing", () => {
        let inner: [64]u8 = undefined;
        const p1 = encodeDerU64(42 as u64, inner[0..64]).?;
        const p2 = encodeDerBool(true, inner[p1.len..64]).?;
        const p3 = encodeDerOctetString("cp", inner[(p1.len + p2.len)..64]).?;
        const inner_len = p1.len + p2.len + p3.len;

        let seq_buf: [128]u8 = undefined;
        const seq = encodeDerSequence(inner[0..inner_len], seq_buf[0..128]).?;

        const outer = parseDerTlv(seq);
        expect_eq(outer.valid, true);
        expect_eq(outer.tag, TAG_SEQUENCE);

        // Walk the 3 nested elements inside SEQUENCE
        const e1 = parseDerTlv(outer.value);
        expect_eq(decodeDerU64(e1).?, 42 as u64);

        const rest1 = outer.value[e1.total_len..outer.value.len];
        const e2 = parseDerTlv(rest1);
        expect_eq(decodeDerBool(e2).?, true);

        const rest2 = rest1[e2.total_len..rest1.len];
        const e3 = parseDerTlv(rest2);
        expect_eq(e3.tag, TAG_OCTET_STRING);
        expect_eq(e3.value, "cp");
        expect_eq(e3.total_len, rest2.len);

        // Output buffer too small returns null
        let tiny: [2]u8 = undefined;
        expect_eq(encodeDerTlv(TAG_OCTET_STRING, "abc", tiny[0..2]) == null, true);
    });
}
