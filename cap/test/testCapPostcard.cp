import { test, expect_eq } from "cap:test";
import {
    encodeVarintU64,
    decodeVarintU64,
    zigzagEncode,
    zigzagDecode,
    encodeVarintI64,
    decodeVarintI64,
    cobsEncode,
    cobsDecode,
} from "../../packages/@cap/postcard/src/index.cp";

fn main() {
    test("1. @cap/postcard — unsigned varint u64 round-trip and boundaries", () => {
        let buf: [16]u8 = undefined;
        const n0 = encodeVarintU64(0, buf[0..]);
        expect_eq(n0, 1);
        expect_eq(buf[0], 0);
        const r0 = decodeVarintU64(buf[0..n0]);
        expect_eq(r0.ok, true);
        expect_eq(r0.value, 0);
        expect_eq(r0.consumed, 1);

        const n1 = encodeVarintU64(300, buf[0..]);
        expect_eq(n1, 2);
        expect_eq(buf[0], 0xAC);
        expect_eq(buf[1], 0x02);
        const r1 = decodeVarintU64(buf[0..n1]);
        expect_eq(r1.ok, true);
        expect_eq(r1.value, 300);
        expect_eq(r1.consumed, 2);

        const max_u64: u64 = (u64(0xFFFF_FFFF) << 32) | u64(0xFFFF_FFFF);
        const nmax = encodeVarintU64(max_u64, buf[0..]);
        expect_eq(nmax, 10);
        const rmax = decodeVarintU64(buf[0..nmax]);
        expect_eq(rmax.ok, true);
        expect_eq(rmax.value, max_u64);
        expect_eq(rmax.consumed, 10);
    });

    test("2. @cap/postcard — zigzag encode/decode and signed varint i64", () => {
        expect_eq(zigzagEncode(0), 0);
        expect_eq(zigzagEncode(-1), 1);
        expect_eq(zigzagEncode(1), 2);
        expect_eq(zigzagEncode(-2), 3);
        expect_eq(zigzagDecode(0), 0);
        expect_eq(zigzagDecode(1), -1);
        expect_eq(zigzagDecode(2), 1);
        expect_eq(zigzagDecode(3), -2);

        let buf: [16]u8 = undefined;
        const n = encodeVarintI64(-123456789, buf[0..]);
        const r = decodeVarintI64(buf[0..n]);
        expect_eq(r.ok, true);
        expect_eq(r.value, -123456789);
        expect_eq(r.consumed, n);

        const min_i64: i64 = @as(i64, @bitCast(u64(0x8000_0000) << 32));
        const nmin = encodeVarintI64(min_i64, buf[0..]);
        const rmin = decodeVarintI64(buf[0..nmin]);
        expect_eq(rmin.ok, true);
        expect_eq(rmin.value, min_i64);
    });

    test("3. @cap/postcard — decodeVarintU64 rejects truncated and overflowing input", () => {
        const r_empty = decodeVarintU64("");
        expect_eq(r_empty.ok, false);

        const trunc: [2]u8 = [0x80, 0x80];
        const r_trunc = decodeVarintU64(trunc[0..]);
        expect_eq(r_trunc.ok, false);

        const ovf: [10]u8 = [0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x02];
        const r_ovf = decodeVarintU64(ovf[0..]);
        expect_eq(r_ovf.ok, false);
    });

    test("4. @cap/postcard — COBS encode/decode standard vectors", () => {
        let enc: [32]u8 = undefined;
        let out_dec: [32]u8 = undefined;

        const src1: [4]u8 = [0x11, 0x22, 0x00, 0x33];
        const elen1 = cobsEncode(src1[0..], enc[0..]);
        expect_eq(elen1, 5);
        expect_eq(enc[0], 0x03);
        expect_eq(enc[1], 0x11);
        expect_eq(enc[2], 0x22);
        expect_eq(enc[3], 0x02);
        expect_eq(enc[4], 0x33);

        const dlen1 = cobsDecode(enc[0..elen1], out_dec[0..]);
        expect_eq(dlen1, 4);
        expect_eq(out_dec[0], 0x11);
        expect_eq(out_dec[1], 0x22);
        expect_eq(out_dec[2], 0x00);
        expect_eq(out_dec[3], 0x33);

        const src2: [2]u8 = [0x00, 0x00];
        const elen2 = cobsEncode(src2[0..], enc[0..]);
        expect_eq(elen2, 3);
        expect_eq(enc[0], 0x01);
        expect_eq(enc[1], 0x01);
        expect_eq(enc[2], 0x01);
        const dlen2 = cobsDecode(enc[0..elen2], out_dec[0..]);
        expect_eq(dlen2, 2);
        expect_eq(out_dec[0], 0x00);
        expect_eq(out_dec[1], 0x00);
    });

    test("5. @cap/postcard — COBS 254-byte and 255-byte non-zero block boundaries", () => {
        let src: [255]u8 = undefined;
        let i: usize = 0;
        while (i < 255) : (i += 1) {
            src[i] = u8((i % 250) + 1);
        }
        let enc: [264]u8 = undefined;
        let out_dec: [264]u8 = undefined;

        const elen254 = cobsEncode(src[0..254], enc[0..]);
        expect_eq(elen254, 255);
        expect_eq(enc[0], 0xFF);
        const dlen254 = cobsDecode(enc[0..elen254], out_dec[0..]);
        expect_eq(dlen254, 254);
        expect_eq(out_dec[0], src[0]);
        expect_eq(out_dec[253], src[253]);

        const elen255 = cobsEncode(src[0..255], enc[0..]);
        expect_eq(elen255, 257);
        expect_eq(enc[0], 0xFF);
        expect_eq(enc[255], 0x02);
        const dlen255 = cobsDecode(enc[0..elen255], out_dec[0..]);
        expect_eq(dlen255, 255);
        expect_eq(out_dec[254], src[254]);
    });

    test("6. @cap/postcard — COBS rejects empty, embedded-zero, and truncated frames", () => {
        let out_dec: [16]u8 = undefined;
        expect_eq(cobsDecode("", out_dec[0..]), -1);

        const zero_code: [2]u8 = [0x00, 0x01];
        expect_eq(cobsDecode(zero_code[0..], out_dec[0..]), -1);

        const embedded_zero: [3]u8 = [0x03, 0x11, 0x00];
        expect_eq(cobsDecode(embedded_zero[0..], out_dec[0..]), -1);

        const truncated: [2]u8 = [0x04, 0x11];
        expect_eq(cobsDecode(truncated[0..], out_dec[0..]), -1);
    });
}
