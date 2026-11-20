// Source: cp/cap/test/test_codec_production_deep.cp
// Deep Production Test Suite for cap:codec & cap.codec
// Covers:
//   1. Base58 & Base58Check (leading 0x00 preservation, Bitcoin vectors, Double-SHA256 tamper detection)
//   2. Base85 (Adobe ASCII85 <~...~>, 'z' zero compression, 1/2/3/4 remainders, Overflow & RFC 1924)
//   3. Bech32 & Bech32m (BIP-173 / BIP-350 official vectors, MixedCase/InvalidChecksum, encodeBytes/decodeBytes)
//   4. Crockford32 (case-insensitive, '-' ignore, I/L->1, O->0, mod-37 check symbol '*~$=U')
//   5. Dynamic lazy namespace mounting via `import cap from "cap"; cap.codec`

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    CodecError,
    Base58,
    Base85,
    Bech32Variant,
    Bech32Decoded,
    Bech32,
    Crockford32,
} from "cap:codec";
import cap from "cap";

fn emptyMutSlice(): []u8 {
    return std.heap.page_allocator.alloc(u8, 0) catch unreachable;
}

fn testBase58AndCheck()! {
    // 1. Empty & leading zero bytes preservation
    const e_empty = Base58.encode("")?;
    expect_eq(e_empty.len, 0 as usize);
    const d_empty = Base58.decode("")?;
    expect_eq(d_empty.len, 0 as usize);

    const z3: [3]u8 = [0x00, 0x00, 0x00];
    const e_z3 = Base58.encode(&z3)?;
    expect_eq(std.mem.eql(u8, e_z3, "111"), true);
    const d_z3 = Base58.decode("111")?;
    expect_eq(std.mem.eql(u8, d_z3, &z3), true);

    const z2_one: [3]u8 = [0x00, 0x00, 0x01];
    const e_z2_one = Base58.encode(&z2_one)?;
    expect_eq(std.mem.eql(u8, e_z2_one, "112"), true);
    const d_z2_one = Base58.decode("112")?;
    expect_eq(std.mem.eql(u8, d_z2_one, &z2_one), true);

    // 2. Classic Base58 standard test vectors
    const hw = Base58.encode("Hello World!")?;
    expect_eq(std.mem.eql(u8, hw, "2NEpo7TZRRrLZSi2U"), true);
    const hw_dec = Base58.decode("2NEpo7TZRRrLZSi2U")?;
    expect_eq(std.mem.eql(u8, hw_dec, "Hello World!"), true);

    const fox = "The quick brown fox jumps over the lazy dog.";
    const fox_enc = Base58.encode(fox)?;
    expect_eq(std.mem.eql(u8, fox_enc, "USm3fpXnKG5EUBx2ndxBDMPVciP5hGey2Jh4NDv6gmeo1LkMeiKrLJUUBk6Z"), true);
    const fox_dec = Base58.decode(fox_enc)?;
    expect_eq(std.mem.eql(u8, fox_dec, fox), true);

    // Reject excluded characters: '0', 'O', 'I', 'l'
    let bad0_ok = false;
    * = Base58.decode("2NEpo0TZ") catch e => err0: {
        bad0_ok = (e == CodecError.InvalidCharacter);
        break :err0 emptyMutSlice();
    };
    expect_eq(bad0_ok, true);

    let badO_ok = false;
    * = Base58.decode("2NEpOTZ") catch e => errO: {
        badO_ok = (e == CodecError.InvalidCharacter);
        break :errO emptyMutSlice();
    };
    expect_eq(badO_ok, true);

    // 3. Base58Check: Bitcoin Genesis P2PKH address (version 0x00 + 20-byte hash160)
    const genesis_payload: [21]u8 = [
        0x00,
        0x62, 0xe9, 0x07, 0xb1, 0x5c, 0xbf, 0x27, 0xd5, 0x42, 0x53,
        0x99, 0xeb, 0xf6, 0xf0, 0xfb, 0x50, 0xeb, 0xb8, 0x8f, 0x18,
    ];
    const addr = Base58.encodeCheck(&genesis_payload)?;
    expect_eq(std.mem.eql(u8, addr, "1A1zP1eP5QGefi2DMPTfTL5SLmv7DivfNa"), true);

    const addr_dec = Base58.decodeCheck(addr)?;
    expect_eq(std.mem.eql(u8, addr_dec, &genesis_payload), true);

    // Multi-leading zero Base58Check round-trip
    const multi_zero_payload: [6]u8 = [0x00, 0x00, 0x00, 0xAB, 0xCD, 0xEF];
    const mz_chk = Base58.encodeCheck(&multi_zero_payload)?;
    const mz_dec = Base58.decodeCheck(mz_chk)?;
    expect_eq(std.mem.eql(u8, mz_dec, &multi_zero_payload), true);

    // Single-character tamper in Base58Check must trigger InvalidChecksum
    let tampered: [34]u8 = [0; 34];
    @memcpy(tampered[0..34], addr);
    tampered[15] = if (tampered[15] == 'A') 'B' else 'A';
    let chk_err_ok = false;
    * = Base58.decodeCheck(&tampered) catch e => err_chk: {
        chk_err_ok = (e == CodecError.InvalidChecksum);
        break :err_chk emptyMutSlice();
    };
    expect_eq(chk_err_ok, true);
}

fn testBase85AsciiAndRfc1924()! {
    // 1. Adobe ASCII85: 'z' zero compression & <~...~> delimiters
    const zeros8: [8]u8 = [0; 8];
    const enc_z = Base85.encodeAscii85(&zeros8)?;
    expect_eq(std.mem.eql(u8, enc_z, "<~zz~>"), true);

    const dec_z_ws = Base85.decodeAscii85("  <~\n z \t z \r\n~>  ")?;
    expect_eq(std.mem.eql(u8, dec_z_ws, &zeros8), true);

    // Classic 4-byte "Man " -> "<~9jqo^~>"
    const enc_man = Base85.encodeAscii85("Man ")?;
    expect_eq(std.mem.eql(u8, enc_man, "<~9jqo^~>"), true);
    const dec_man = Base85.decodeAscii85("<~9jqo^~>")?;
    expect_eq(std.mem.eql(u8, dec_man, "Man "), true);

    // 2. Remainder 1, 2, 3, 4, 7 bytes round-trip (including 0x00 and 0xFF boundaries)
    const p1: [1]u8 = [0xFF];
    const p2: [2]u8 = [0x00, 0x01];
    const p3: [3]u8 = [0xFF, 0xFF, 0xFF];
    const p4: [4]u8 = [0xFF, 0xFF, 0xFF, 0xFF];
    const p7: [7]u8 = [0x00, 0x00, 0x00, 0x00, 0x12, 0x34, 0x56];

    expect_eq(std.mem.eql(u8, Base85.decodeAscii85(Base85.encodeAscii85(&p1)?)?, &p1), true);
    expect_eq(std.mem.eql(u8, Base85.decodeAscii85(Base85.encodeAscii85(&p2)?)?, &p2), true);
    expect_eq(std.mem.eql(u8, Base85.decodeAscii85(Base85.encodeAscii85(&p3)?)?, &p3), true);

    const max32_enc = Base85.encodeAscii85(&p4)?;
    expect_eq(std.mem.eql(u8, max32_enc, "<~s8W-!~>"), true);
    expect_eq(std.mem.eql(u8, Base85.decodeAscii85(max32_enc)?, &p4), true);
    expect_eq(std.mem.eql(u8, Base85.decodeAscii85(Base85.encodeAscii85(&p7)?)?, &p7), true);

    // Empty round-trip
    const empty_a85 = Base85.encodeAscii85("")?;
    expect_eq(std.mem.eql(u8, empty_a85, "<~~>"), true);
    expect_eq((Base85.decodeAscii85(empty_a85)?).len, 0 as usize);

    // 3. Overflow interception: "s8W-\"" == 0xFFFFFFFF + 1
    let ovf_ok = false;
    * = Base85.decodeAscii85("<~s8W-\"~>") catch e => err_ovf: {
        ovf_ok = (e == CodecError.Overflow);
        break :err_ovf emptyMutSlice();
    };
    expect_eq(ovf_ok, true);

    // InvalidPrefix: unclosed <~
    let pfx_ok = false;
    * = Base85.decodeAscii85("<~9jqo^") catch e => err_pfx: {
        pfx_ok = (e == CodecError.InvalidPrefix);
        break :err_pfx emptyMutSlice();
    };
    expect_eq(pfx_ok, true);

    // 4. RFC 1924 round-trip across arbitrary lengths
    const rfc_sample = "CP-Lang-Base85-RFC1924!";
    const rfc_enc = Base85.encodeRfc1924(rfc_sample)?;
    const rfc_dec = Base85.decodeRfc1924(rfc_enc)?;
    expect_eq(std.mem.eql(u8, rfc_dec, rfc_sample), true);

    const rfc_zeros: [8]u8 = [0; 8];
    const rfc_z_enc = Base85.encodeRfc1924(&rfc_zeros)?;
    expect_eq(std.mem.eql(u8, rfc_z_enc, "0000000000"), true);
    expect_eq(std.mem.eql(u8, Base85.decodeRfc1924(rfc_z_enc)?, &rfc_zeros), true);

    // RFC 1924 overflow check ("~~~~~" > 0xFFFFFFFF)
    let rfc_ovf_ok = false;
    * = Base85.decodeRfc1924("~~~~~") catch e => err_rfc_ovf: {
        rfc_ovf_ok = (e == CodecError.Overflow);
        break :err_rfc_ovf emptyMutSlice();
    };
    expect_eq(rfc_ovf_ok, true);
}

fn testBech32AndBech32m()! {
    // 1. Official BIP-173 Bech32 test vectors
    const b1 = Bech32.decode("A12UEL5L")?;
    expect_eq(std.mem.eql(u8, b1.hrp, "a"), true);
    expect_eq(b1.data.len, 0 as usize);
    expect_eq(b1.variant == Bech32Variant.Bech32, true);

    const b2 = Bech32.decode("an83characterlonghumanreadablepartthatcontainsthenumber1andtheexcludedcharactersbio1tt5tgs")?;
    expect_eq(b2.hrp.len, 83 as usize);
    expect_eq(b2.variant == Bech32Variant.Bech32, true);

    const b3 = Bech32.decode("abcdef1qpzry9x8gf2tvdw0s3jn54khce6mua7lmqqqxw")?;
    expect_eq(std.mem.eql(u8, b3.hrp, "abcdef"), true);
    expect_eq(b3.data.len, 32 as usize);
    expect_eq(b3.variant == Bech32Variant.Bech32, true);
    for (i of 0..32) {
        expect_eq(b3.data[i], i as u8);
    }

    // Re-encode b3 and verify exact match
    const b3_re = Bech32.encode("abcdef", b3.data, Bech32Variant.Bech32)?;
    expect_eq(std.mem.eql(u8, b3_re, "abcdef1qpzry9x8gf2tvdw0s3jn54khce6mua7lmqqqxw"), true);

    // 2. Official BIP-350 Bech32m test vectors
    const m1 = Bech32.decode("A1LQFN3A")?;
    expect_eq(std.mem.eql(u8, m1.hrp, "a"), true);
    expect_eq(m1.data.len, 0 as usize);
    expect_eq(m1.variant == Bech32Variant.Bech32m, true);

    const m2 = Bech32.decode("abcdef1l7aum6echk45nj3s0wdvt2fg8x9yrzpqzd3ryx")?;
    expect_eq(std.mem.eql(u8, m2.hrp, "abcdef"), true);
    expect_eq(m2.data.len, 32 as usize);
    expect_eq(m2.variant == Bech32Variant.Bech32m, true);
    for (i of 0..32) {
        expect_eq(m2.data[i], (31 - i) as u8);
    }

    const taproot = Bech32.decode("bc1p0xlxvlhemja6c4dqv22uapctqupfhlxm9h8z3k2e72q4k9hcz7vqzk5jj0")?;
    expect_eq(std.mem.eql(u8, taproot.hrp, "bc"), true);
    expect_eq(taproot.variant == Bech32Variant.Bech32m, true);
    expect_eq(taproot.data[0], 1 as u8); // witness v1

    // 3. Reject MixedCase, single-char flip InvalidChecksum, > 90 InvalidLength
    const mixed = Bech32.decode("A12UEL5l") catch e => err_mix: {
        const tag = if (e == CodecError.MixedCase) "err" else "wrong";
        break :err_mix Bech32Decoded{ hrp: tag, data: [], variant: Bech32Variant.Bech32 };
    };
    expect_eq(std.mem.eql(u8, mixed.hrp, "err"), true);

    const bad_chk = Bech32.decode("abcdef1qpzry9x8gf2tvdw0s3jn54khce6mua7lmqqqxx") catch e => err_bchk: {
        const tag = if (e == CodecError.InvalidChecksum) "err" else "wrong";
        break :err_bchk Bech32Decoded{ hrp: tag, data: [], variant: Bech32Variant.Bech32 };
    };
    expect_eq(std.mem.eql(u8, bad_chk.hrp, "err"), true);

    const too_long = Bech32.decode("an84characterslonghumanreadablepartthatcontainsthenumber1andtheexcludedcharactersbio1569pvx") catch e => err_len: {
        const tag = if (e == CodecError.InvalidLength) "err" else "wrong";
        break :err_len Bech32Decoded{ hrp: tag, data: [], variant: Bech32Variant.Bech32 };
    };
    expect_eq(std.mem.eql(u8, too_long.hrp, "err"), true);

    // 4. encodeBytes / decodeBytes arbitrary 8-bit binary payload round-trip
    const payload: [12]u8 = [0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xAA, 0xFF];
    const enc_b = Bech32.encodeBytes("cp", &payload, Bech32Variant.Bech32m)?;
    const dec_b = Bech32.decodeBytes(enc_b)?;
    expect_eq(std.mem.eql(u8, dec_b.hrp, "cp"), true);
    expect_eq(dec_b.variant == Bech32Variant.Bech32m, true);
    expect_eq(std.mem.eql(u8, dec_b.data, &payload), true);
}

fn testCrockford32()! {
    // 1. Byte slice encode/decode round-trip
    const samples: [4][]const u8 = [
        "",
        "f",
        "foobar",
        "\x00\xFF\x12\x34\xAB\xCD\xEF",
    ];
    let si: usize = 0;
    while (si < samples.len) : (si += 1) {
        const s = samples[si];
        const enc = Crockford32.encode(s)?;
        const decoded = Crockford32.decode(enc)?;
        expect_eq(std.mem.eql(u8, decoded, s), true);
    }

    // 2. Case-insensitive + '-' separator + I/L->1, O->0 tolerance
    const ref_bytes: [5]u8 = [0x00, 0x01, 0xFF, 0x80, 0x42];
    const clean_enc = Crockford32.encode(&ref_bytes)?;
    const clean_dec = Crockford32.decode(clean_enc)?;
    expect_eq(std.mem.eql(u8, clean_dec, &ref_bytes), true);

    const d_canon = Crockford32.decode("00001111")?;
    const d_noisy = Crockford32.decode("0oO-0iIl-1")?;
    expect_eq(std.mem.eql(u8, d_canon, d_noisy), true);

    // 'U' is forbidden in raw Crockford32 data stream
    let bad_u_ok = false;
    * = Crockford32.decode("123U") catch e => err_u: {
        bad_u_ok = (e == CodecError.InvalidCharacter);
        break :err_u emptyMutSlice();
    };
    expect_eq(bad_u_ok, true);

    // 3. encodeU64Check / decodeU64Check with modulo-37 check symbols (*~$=U)
    const c0 = Crockford32.encodeU64Check(0)?;
    expect_eq(std.mem.eql(u8, c0, "00"), true);
    expect_eq(Crockford32.decodeU64Check(c0)?, 0 as u64);

    // 36 % 37 = 36 -> check symbol 'U'
    const c36 = Crockford32.encodeU64Check(36)?;
    expect_eq(std.mem.eql(u8, c36, "14U"), true);
    expect_eq(Crockford32.decodeU64Check("1-4u")?, 36 as u64);

    // 32..35 -> '*', '~', '$', '='
    const c32 = Crockford32.encodeU64Check(32)?;
    expect_eq(std.mem.eql(u8, c32, "10*"), true);
    expect_eq(Crockford32.decodeU64Check("1o*")?, 32 as u64);

    const c33 = Crockford32.encodeU64Check(33)?;
    expect_eq(std.mem.eql(u8, c33, "11~"), true);
    expect_eq(Crockford32.decodeU64Check("il~")?, 33 as u64);

    const c34 = Crockford32.encodeU64Check(34)?;
    expect_eq(std.mem.eql(u8, c34, "12$"), true);
    expect_eq(Crockford32.decodeU64Check("l2$")?, 34 as u64);

    const c35 = Crockford32.encodeU64Check(35)?;
    expect_eq(std.mem.eql(u8, c35, "13="), true);
    expect_eq(Crockford32.decodeU64Check("I3=")?, 35 as u64);

    // 1234 -> "16JD"
    const c1234 = Crockford32.encodeU64Check(1234)?;
    expect_eq(std.mem.eql(u8, c1234, "16JD"), true);
    expect_eq(Crockford32.decodeU64Check("16-jD")?, 1234 as u64);

    // Corrupted check symbol or digit -> InvalidChecksum
    const bad_c32 = Crockford32.decodeU64Check("16JE") catch e => err_c32: {
        break :err_c32 (if (e == CodecError.InvalidChecksum) (999999 as u64) else (0 as u64));
    };
    expect_eq(bad_c32, 999999 as u64);
}

fn testCapCodecDynamicMount()! {
    const b58 = cap.codec.Base58.encode("cap.codec")?;
    expect_eq(std.mem.eql(u8, cap.codec.Base58.decode(b58)?, "cap.codec"), true);

    const z4: [4]u8 = [0, 0, 0, 0];
    const a85 = cap.codec.Base85.encodeAscii85(&z4)?;
    expect_eq(std.mem.eql(u8, a85, "<~z~>"), true);

    const b32 = cap.codec.Bech32.encodeBytes("cap", "ok", cap.codec.Bech32Variant.Bech32m)?;
    const b32_dec = cap.codec.Bech32.decodeBytes(b32)?;
    expect_eq(std.mem.eql(u8, b32_dec.data, "ok"), true);
    expect_eq(b32_dec.variant == cap.codec.Bech32Variant.Bech32m, true);

    const c_chk = cap.codec.Crockford32.encodeU64Check(42)?;
    expect_eq(cap.codec.Crockford32.decodeU64Check(c_chk)?, 42 as u64);
}

fn main() {
    test("1. Base58 & Base58Check: Leading Zeros, Bitcoin Genesis Vector & Double-SHA256 Checksum", () => {
        testBase58AndCheck()?;
    });

    test("2. Base85: Adobe ASCII85 (<~...~>, 'z', Remainders, Overflow) & RFC 1924 Round-Trip", () => {
        testBase85AsciiAndRfc1924()?;
    });

    test("3. Bech32 & Bech32m: BIP-173/BIP-350 Vectors, MixedCase/Checksum Guards & Binary Bytes", () => {
        testBech32AndBech32m()?;
    });

    test("4. Crockford32: Case-Insensitive, Hyphen/I/L/O Tolerance & Mod-37 Check Symbols", () => {
        testCrockford32()?;
    });

    test("5. Dynamic Lazy Namespace Mounting via `import cap from \"cap\"; cap.codec`", () => {
        testCapCodecDynamicMount()?;
    });
}
