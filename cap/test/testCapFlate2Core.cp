import { test, expect_eq } from "cap:test";
import {
    GZIP_ID1, GZIP_ID2, GZIP_CM_DEFLATE,
    GzipHeader,
    parseGzipHeader, encodeGzipStored, decodeGzipStored,
    encodeZlibStored, decodeZlibStored
} from "../../packages/flate2/src/index.cp";

fn main() {
    test("1. @cap/flate2-core — Gzip stored encode/decode round-trip & header metadata", () => {
        let payload: [11]u8 = [104, 101, 108, 108, 111, 32, 119, 111, 114, 108, 100]; // "hello world"
        let gz: [128]u8 = [0; 128];
        let enc_len: usize = encodeGzipStored(payload[0..11], 1700000000, gz[0..128]) ?? 0;
        // 10 header + 5 stored block hdr + 11 payload + 8 trailer = 34
        expect_eq(enc_len, 34);
        expect_eq(gz[0], GZIP_ID1);
        expect_eq(gz[1], GZIP_ID2);
        expect_eq(gz[2], GZIP_CM_DEFLATE);

        let hdr = parseGzipHeader(gz[0..enc_len]);
        expect_eq(hdr.valid, true);
        expect_eq(hdr.method, 8);
        expect_eq(hdr.flags, 0);
        expect_eq(hdr.mtime, 1700000000);
        expect_eq(hdr.os, 255);
        expect_eq(hdr.data_offset, 10);

        let out: [64]u8 = [0; 64];
        let dec_len: usize = decodeGzipStored(gz[0..enc_len], out[0..64]) ?? 0;
        expect_eq(dec_len, 11);
        expect_eq(out[0], 104);
        expect_eq(out[10], 100);
    });

    test("2. @cap/flate2-core — Gzip rejects corrupted CRC-32, ISIZE, or NLEN", () => {
        let payload: [8]u8 = [1, 2, 3, 4, 5, 6, 7, 8];
        let gz: [64]u8 = [0; 64];
        let enc_len: usize = encodeGzipStored(payload[0..8], 12345, gz[0..64]) ?? 0;
        let out: [32]u8 = [0; 32];

        // Corrupt CRC-32 byte
        gz[enc_len - 8] = gz[enc_len - 8] ^ 1;
        expect_eq(decodeGzipStored(gz[0..enc_len], out[0..32]) == null, true);
        gz[enc_len - 8] = gz[enc_len - 8] ^ 1;

        // Corrupt ISIZE byte
        gz[enc_len - 4] = gz[enc_len - 4] ^ 1;
        expect_eq(decodeGzipStored(gz[0..enc_len], out[0..32]) == null, true);
        gz[enc_len - 4] = gz[enc_len - 4] ^ 1;

        // Corrupt stored block NLEN
        gz[13] = gz[13] ^ 1;
        expect_eq(decodeGzipStored(gz[0..enc_len], out[0..32]) == null, true);
    });

    test("3. @cap/flate2-core — Zlib stored encode/decode round-trip & header FCHECK", () => {
        let payload: [9]u8 = [87, 105, 107, 105, 112, 101, 100, 105, 97]; // "Wikipedia"
        let zl: [64]u8 = [0; 64];
        let enc_len: usize = encodeZlibStored(payload[0..9], zl[0..64]) ?? 0;
        // 2 header + 5 stored hdr + 9 payload + 4 Adler-32 = 20
        expect_eq(enc_len, 20);
        expect_eq(zl[0], 120); // 0x78
        expect_eq(zl[1], 1);   // 0x01

        let out: [32]u8 = [0; 32];
        let dec_len: usize = decodeZlibStored(zl[0..enc_len], out[0..32]) ?? 0;
        expect_eq(dec_len, 9);
        expect_eq(out[0], 87);
        expect_eq(out[8], 97);
    });

    test("4. @cap/flate2-core — Zlib rejects invalid FCHECK and corrupted Adler-32", () => {
        let payload: [5]u8 = [10, 20, 30, 40, 50];
        let zl: [64]u8 = [0; 64];
        let enc_len: usize = encodeZlibStored(payload[0..5], zl[0..64]) ?? 0;
        let out: [32]u8 = [0; 32];

        // Corrupt FCHECK header byte
        zl[1] = 2; // (0x78*256 + 2) % 31 != 0
        expect_eq(decodeZlibStored(zl[0..enc_len], out[0..32]) == null, true);
        zl[1] = 1;

        // Corrupt Adler-32 trailer byte
        zl[enc_len - 1] = zl[enc_len - 1] ^ 1;
        expect_eq(decodeZlibStored(zl[0..enc_len], out[0..32]) == null, true);
    });

    test("5. @cap/flate2-core — empty payload round-trip for both Gzip and Zlib", () => {
        let empty: [1]u8 = [0];
        let buf: [64]u8 = [0; 64];
        let out: [16]u8 = [0; 16];

        let gz_len: usize = encodeGzipStored(empty[0..0], 0, buf[0..64]) ?? 0;
        expect_eq(gz_len, 23); // 10 + 5 + 8
        let gz_dec = decodeGzipStored(buf[0..gz_len], out[0..16]);
        expect_eq(gz_dec != null, true);
        expect_eq(gz_dec ?? 99, 0);

        let zl_len: usize = encodeZlibStored(empty[0..0], buf[0..64]) ?? 0;
        expect_eq(zl_len, 11); // 2 + 5 + 4
        let zl_dec = decodeZlibStored(buf[0..zl_len], out[0..16]);
        expect_eq(zl_dec != null, true);
        expect_eq(zl_dec ?? 99, 0);
    });

    test("6. @cap/flate2-core — parseGzipHeader handles FNAME & rejects undersized buffers", () => {
        // Minimal Gzip header with FNAME ("a.txt\0")
        let hdr_buf: [16]u8 = [31, 139, 8, 8, 0, 0, 0, 0, 0, 3, 97, 46, 116, 120, 116, 0];
        let hdr = parseGzipHeader(hdr_buf[0..16]);
        expect_eq(hdr.valid, true);
        expect_eq(hdr.flags, 8);
        expect_eq(hdr.os, 3);
        expect_eq(hdr.data_offset, 16);

        // Undersized output buffers rejected
        let payload: [10]u8 = [65; 10];
        let tiny: [12]u8 = [0; 12];
        expect_eq(encodeGzipStored(payload[0..10], 0, tiny[0..12]) == null, true);
        expect_eq(encodeZlibStored(payload[0..10], tiny[0..12]) == null, true);
    });
}
