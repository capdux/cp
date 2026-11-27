import { test, expect_eq } from "cap:test";
import { hashBytes } from "../../packages/crc32fast/src/index.cp";
import { readU32Le } from "../../packages/bytemuck/src/index.cp";
import {
    ZIP_LOCAL_SIG, ZIP_CD_SIG, ZIP_EOCD_SIG,
    ZIP_METHOD_STORED, ZIP_METHOD_DEFLATE,
    ZipLocalHeader, ZipEocd,
    writeStoredZipSingle, parseLocalHeader, findAndParseEocd, verifyStoredEntry
} from "../../packages/zip/src/index.cp";

fn main() {
    test("1. @cap/zip-core — writeStoredZipSingle & parseLocalHeader round-trip", () => {
        let payload: [11]u8 = [104, 101, 108, 108, 111, 32, 119, 111, 114, 108, 100]; // "hello world"
        let out: [512]u8 = [0; 512];
        let written_opt = writeStoredZipSingle("hello.txt", payload[0..11], out[0..512]);
        expect_eq(written_opt != null, true);
        let written: usize = written_opt ?? 0;
        // 30 + 9 + 11 + 46 + 9 + 22 = 127
        expect_eq(written, 127);

        let hdr = parseLocalHeader(out[0..written], 0);
        expect_eq(hdr.valid, true);
        expect_eq(hdr.version_needed, 20);
        expect_eq(hdr.flags, 0);
        expect_eq(hdr.method, ZIP_METHOD_STORED);
        expect_eq(hdr.crc32, hashBytes(payload[0..11]));
        expect_eq(hdr.compressed_size, 11);
        expect_eq(hdr.uncompressed_size, 11);
        expect_eq(hdr.name_len, 9);
        expect_eq(hdr.extra_len, 0);
        expect_eq(hdr.data_offset, 39);
        expect_eq(verifyStoredEntry(out[0..written], hdr), true);
    });

    test("2. @cap/zip-core — findAndParseEocd locates EOCD & Central Directory metadata", () => {
        let payload: [4]u8 = [65, 66, 67, 68]; // "ABCD"
        let out: [256]u8 = [0; 256];
        let total: usize = writeStoredZipSingle("a.bin", payload[0..4], out[0..256]) ?? 0;
        expect_eq(total, 112); // 30+5+4 + 46+5 + 22

        let eocd = findAndParseEocd(out[0..total]);
        expect_eq(eocd.valid, true);
        expect_eq(eocd.disk_num, 0);
        expect_eq(eocd.total_entries, 1);
        expect_eq(eocd.cd_offset, 39);
        expect_eq(eocd.cd_size, 51);

        let cd_sig = readU32Le(out[39..43]) ?? 0;
        expect_eq(cd_sig, ZIP_CD_SIG);
        expect_eq(ZIP_LOCAL_SIG, 67324752);
        expect_eq(ZIP_EOCD_SIG, 101010256);
    });

    test("3. @cap/zip-core — verifyStoredEntry detects corrupted payload CRC-32", () => {
        let payload: [6]u8 = [1, 2, 3, 4, 5, 6];
        let out: [256]u8 = [0; 256];
        let total: usize = writeStoredZipSingle("data.bin", payload[0..6], out[0..256]) ?? 0;

        let hdr = parseLocalHeader(out[0..total], 0);
        expect_eq(verifyStoredEntry(out[0..total], hdr), true);

        // Flip 1 bit in payload
        out[hdr.data_offset + 2] = out[hdr.data_offset + 2] ^ 1;
        expect_eq(verifyStoredEntry(out[0..total], hdr), false);
    });

    test("4. @cap/zip-core — verifyStoredEntry rejects non-Stored (Deflate) entries", () => {
        let payload: [5]u8 = [10, 20, 30, 40, 50];
        let out: [256]u8 = [0; 256];
        let total: usize = writeStoredZipSingle("c.dat", payload[0..5], out[0..256]) ?? 0;

        let hdr = parseLocalHeader(out[0..total], 0);
        let deflate_hdr = ZipLocalHeader {
            version_needed: hdr.version_needed,
            flags: hdr.flags,
            method: ZIP_METHOD_DEFLATE,
            crc32: hdr.crc32,
            compressed_size: hdr.compressed_size,
            uncompressed_size: hdr.uncompressed_size,
            name_len: hdr.name_len,
            extra_len: hdr.extra_len,
            data_offset: hdr.data_offset,
            valid: true,
        };
        expect_eq(verifyStoredEntry(out[0..total], deflate_hdr), false);
    });

    test("5. @cap/zip-core — empty payload Stored ZIP works & verifies CRC-32 == 0", () => {
        let empty: [1]u8 = [0];
        let out: [256]u8 = [0; 256];
        let total: usize = writeStoredZipSingle("empty.txt", empty[0..0], out[0..256]) ?? 0;
        expect_eq(total, 116); // 30+9+0 + 46+9 + 22

        let hdr = parseLocalHeader(out[0..total], 0);
        expect_eq(hdr.valid, true);
        expect_eq(hdr.crc32, 0);
        expect_eq(hdr.compressed_size, 0);
        expect_eq(verifyStoredEntry(out[0..total], hdr), true);
    });

    test("6. @cap/zip-core — rejects empty filename, undersized buffer, bad signatures", () => {
        let payload: [4]u8 = [1, 2, 3, 4];
        let small: [64]u8 = [0; 64];
        let big: [256]u8 = [0; 256];

        expect_eq(writeStoredZipSingle("", payload[0..4], big[0..256]) == null, true);
        expect_eq(writeStoredZipSingle("x.txt", payload[0..4], small[0..64]) == null, true);

        let bad_local = parseLocalHeader(small[0..64], 0);
        expect_eq(bad_local.valid, false);

        let bad_eocd = findAndParseEocd(small[0..64]);
        expect_eq(bad_eocd.valid, false);
    });
}
