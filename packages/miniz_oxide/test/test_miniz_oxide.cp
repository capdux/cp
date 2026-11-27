import { test, expect_eq } from "cap:test";
import {
    COMPRESSION_NO_COMPRESSION,
    COMPRESSION_BEST_SPEED,
    COMPRESSION_DEFAULT_LEVEL,
    COMPRESSION_BEST_COMPRESSION,
    MinizBuf,
    versionString,
    compressBound,
    crc32,
    crc32Update,
    adler32,
    adler32Update,
    compressToVec,
    compressToVecZlib,
    decompressToVec,
    decompressToVecZlib,
    decompressToVecWithLimit,
    decompressToVecZlibWithLimit,
    compressToSlice,
    compressToSliceZlib,
    decompressToSlice,
    decompressToSliceZlib,
    compress_to_vec,
    compress_to_vec_zlib,
    decompress_to_vec,
    decompress_to_vec_zlib,
} from "../src/index.cp";

fn isZlibLimitRejected(z: []const u8, limit: usize): bool {
    const _b = decompressToVecZlibWithLimit(z, limit) catch return true;
    return _b.len > limit;
}

fn main() {
    test("1. @cap/miniz_oxide — versionString & compressBound", () => {
        const v = versionString();
        expect_eq(v.len > 0, true);
        expect_eq(compressBound(1024) >= 1024, true);
    });

    test("2. @cap/miniz_oxide — raw DEFLATE compressToVec / decompressToVec roundtrip", () => {
        const payload = "Hello miniz_oxide! Repeating pattern: AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA";
        const compressed = compressToVec(payload, COMPRESSION_DEFAULT_LEVEL) catch {
            expect_eq(true, false);
            return;
        };
        expect_eq(compressed.len > 0, true);
        expect_eq(compressed.len < payload.len, true);

        const restored = decompressToVec(compressed.slice()) catch {
            expect_eq(true, false);
            return;
        };
        expect_eq(restored.slice() as string, payload);
    });

    test("3. @cap/miniz_oxide — Zlib compressToVecZlib / decompressToVecZlib & snake_case aliases", () => {
        const payload = "Zlib stream with Adler-32 checksum verification! 1234567890 1234567890";
        const zbuf = compress_to_vec_zlib(payload, COMPRESSION_BEST_COMPRESSION) catch {
            expect_eq(true, false);
            return;
        };
        expect_eq(zbuf.len > 2, true);
        expect_eq(zbuf.slice()[0] & 0x0F, 8 as u8);

        const out = decompress_to_vec_zlib(zbuf.slice()) catch {
            expect_eq(true, false);
            return;
        };
        expect_eq(out.slice() as string, payload);

        const raw = compress_to_vec("quick test", COMPRESSION_BEST_SPEED) catch {
            expect_eq(true, false);
            return;
        };
        const raw_out = decompress_to_vec(raw.slice()) catch {
            expect_eq(true, false);
            return;
        };
        expect_eq(raw_out.slice() as string, "quick test");
    });

    test("4. @cap/miniz_oxide — output size limit guards", () => {
        const payload = "0123456789012345678901234567890123456789";
        const z = compressToVecZlib(payload, COMPRESSION_DEFAULT_LEVEL) catch {
            expect_eq(true, false);
            return;
        };
        const ok_buf = decompressToVecZlibWithLimit(z.slice(), 100) catch {
            expect_eq(true, false);
            return;
        };
        expect_eq(ok_buf.slice() as string, payload);
        expect_eq(isZlibLimitRejected(z.slice(), 10), true);
    });

    test("5. @cap/miniz_oxide — zero-heap compressToSlice / decompressToSlice (raw & zlib)", () => {
        const payload = "Zero-heap slice compression roundtrip test payload!";
        let comp_buf: [256]u8 = [0; 256];
        let out_buf: [256]u8 = [0; 256];

        const c_raw = compressToSlice(payload, comp_buf[0..256], COMPRESSION_NO_COMPRESSION) catch {
            expect_eq(true, false);
            return;
        };
        const d_raw = decompressToSlice(comp_buf[0..c_raw], out_buf[0..256]) catch {
            expect_eq(true, false);
            return;
        };
        expect_eq(out_buf[0..d_raw] as string, payload);

        const c_zlib = compressToSliceZlib(payload, comp_buf[0..256], COMPRESSION_DEFAULT_LEVEL) catch {
            expect_eq(true, false);
            return;
        };
        const d_zlib = decompressToSliceZlib(comp_buf[0..c_zlib], out_buf[0..256]) catch {
            expect_eq(true, false);
            return;
        };
        expect_eq(out_buf[0..d_zlib] as string, payload);
    });

    test("6. @cap/miniz_oxide — CRC-32 & Adler-32 standard vectors + ZIP single-file roundtrip", () => {
        expect_eq(crc32("123456789"), 0xCBF43926 as u32);
        const partial_crc = crc32Update(0, "12345");
        expect_eq(crc32Update(partial_crc, "6789"), 0xCBF43926 as u32);

        expect_eq(adler32("Wikipedia"), 0x11E60398 as u32);
        const partial_a = adler32Update(1, "Wiki");
        expect_eq(adler32Update(partial_a, "pedia"), 0x11E60398 as u32);

        const zip = MinizBuf.zipSingleFile("hello.txt", "Hello ZIP!") catch {
            expect_eq(true, false);
            return;
        };
        const extracted = MinizBuf.extractFile(zip.slice(), "hello.txt") catch {
            expect_eq(true, false);
            return;
        };
        expect_eq(extracted.slice() as string, "Hello ZIP!");
    });
}
