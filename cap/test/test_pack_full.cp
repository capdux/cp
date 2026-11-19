// ============================================================================
// cap/test/test_pack_full.cp — cap:pack 全家桶六大核心组件端到端回归测试套件
// 覆盖:
//   1. Gzip / Zlib / RawDeflate 单发与 GzipStream / GunzipStream 流式管道
//   2. Zstd (RFC 8478) 帧头解析、Skippable 帧识别与 Bound 计算
//   3. Brotli (RFC 7932) Bound 计算、BrotliMode 枚举与 BrotliHeader WBITS 解析
//   4. POSIX.1-1988 USTAR Tar 归档编码与 TarReader 零拷贝遍历
//   5. PKZIP 归档编码、Central Directory、ZipEocd 反向定位与 ZipReader 读取
//   6. Crc32 (IEEE 802.3) 与 Adler32 (RFC 1950) 单发与流式增量校验
// ============================================================================

import { test, expect_eq } from "cap:test";
import {
    // 1. Zlib / Gzip / Deflate
    gzip,
    gunzip,
    deflate,
    inflate,
    deflateRaw,
    inflateRaw,
    GzipHeader,
    GzipStream,
    GunzipStream,
    DeflateStream,
    InflateStream,
    GZIP_MAGIC_ID1,
    GZIP_MAGIC_ID2,
    GZIP_FNAME,
    CompressionFormat,
    CompressBoundEstimator,
    Zlib,
    Deflate,
    // 2. Zstd
    Zstd,
    ZstdFrameHeader,
    ZSTD_MAGIC,
    ZSTD_DEFAULT_CLEVEL,
    // 3. Brotli
    Brotli,
    BrotliMode,
    BrotliHeader,
    BrotliDecoderResult,
    BrotliStreamState,
    BrotliEncoderOptions,
    BrotliDecoderOptions,
    BROTLI_DEFAULT_QUALITY,
    BROTLI_DEFAULT_WINDOW,
    // 4. Tar
    TarHeader,
    TarReader,
    TarTypeFlag,
    TAR_BLOCK_SIZE,
    // 5. Zip
    ZipLocalHeader,
    ZipCentralHeader,
    ZipEocd,
    ZipReader,
    ZIP_LOCAL_HEADER_SIZE,
    ZIP_CENTRAL_DIR_SIZE,
    ZIP_EOCD_SIZE,
    ZIP_METHOD_STORE,
    // 6. Checksum
    Crc32,
    Adler32,
} from "cap:pack";

fn main() {
    test("1. Gzip / Deflate / RawDeflate one-shot & GzipStream / GunzipStream streaming", () => {
        const msg = "CP cap:pack full regression suite: RFC 1952 Gzip, RFC 1950 Zlib & RFC 1951 Deflate!";

        // Gzip one-shot
        const gz = gzip(msg, 6);
        expect_eq(gz.len >= 18, true);
        expect_eq(gz[0], GZIP_MAGIC_ID1);
        expect_eq(gz[1], GZIP_MAGIC_ID2);

        const hdr_opt = GzipHeader.parse(gz);
        expect_eq(hdr_opt != null, true);

        const un_gz = gunzip(gz);
        expect_eq(un_gz != null, true);
        expect_eq(un_gz.? == msg, true);

        // Zlib Deflate one-shot
        const zl = deflate(msg, 6);
        const un_zl = inflate(zl);
        expect_eq(un_zl != null, true);
        expect_eq(un_zl.? == msg, true);

        // Raw Deflate one-shot
        const raw_def = deflateRaw(msg, 6);
        const un_raw = inflateRaw(raw_def);
        expect_eq(un_raw != null, true);
        expect_eq(un_raw.? == msg, true);

        // GzipStream & GunzipStream chunked pipeline
        const comp_opt = GzipStream.create(6);
        expect_eq(comp_opt != null, true);
        let comp = comp_opt.?;

        const c1 = comp.write("Streaming part A | ");
        const c2 = comp.write("Streaming part B | ");
        const c3 = comp.write("Streaming part C.");
        const c_fin = comp.finish();

        const decomp_opt = GunzipStream.create();
        expect_eq(decomp_opt != null, true);
        let decomp = decomp_opt.?;

        const d1 = decomp.write(c1);
        const d2 = decomp.write(c2);
        const d3 = decomp.write(c3);
        const d4 = decomp.write(c_fin);
        const d_fin = decomp.finish();

        const total_len = d1.len + d2.len + d3.len + d4.len + d_fin.len;
        let out_buf: [128]u8 = [0; 128];
        let pos: usize = 0;
        @memcpy(out_buf[pos .. pos + d1.len], d1); pos += d1.len;
        @memcpy(out_buf[pos .. pos + d2.len], d2); pos += d2.len;
        @memcpy(out_buf[pos .. pos + d3.len], d3); pos += d3.len;
        @memcpy(out_buf[pos .. pos + d4.len], d4); pos += d4.len;
        @memcpy(out_buf[pos .. pos + d_fin.len], d_fin); pos += d_fin.len;

        const expected = "Streaming part A | Streaming part B | Streaming part C.";
        expect_eq(total_len, expected.len);
        expect_eq(out_buf[0..total_len] == expected, true);
    });

    test("2. Zstandard (RFC 8478) ZstdFrameHeader parsing & Zstd bound calculation", () => {
        expect_eq(Zstd.bound(1024) > 1024, true);
        expect_eq(Zstd.compressBound(4096), Zstd.bound(4096));
        expect_eq(ZSTD_DEFAULT_CLEVEL, 3);

        // Case A: Single-segment frame with 1-byte FCS (FHD = 0x24: Single_Segment=1, Content_Checksum=1, FCS_Flag=0)
        // Magic: 28 B5 2F FD, FHD: 0x24, FCS: 120 (0x78)
        let frame_1b: [6]u8 = [0x28, 0xB5, 0x2F, 0xFD, 0x24, 0x78];
        expect_eq(ZstdFrameHeader.isZstdHeader(frame_1b[0..6]), true);
        expect_eq(ZstdFrameHeader.isSkippableHeader(frame_1b[0..6]), false);

        const parsed1 = ZstdFrameHeader.parse(frame_1b[0..6]);
        expect_eq(parsed1 != null, true);
        if (h1 := parsed1) {
            expect_eq(h1.magic, ZSTD_MAGIC);
            expect_eq(h1.single_segment, true);
            expect_eq(h1.content_checksum, true);
            expect_eq(h1.content_size, 120 as u64);
            expect_eq(h1.window_size, 120 as u64);
            expect_eq(h1.header_size, 6 as usize);
        }
        expect_eq(Zstd.findDecompressedSize(frame_1b[0..6]).?, 120 as u64);

        // Case B: Multi-segment frame with Window_Descriptor and 2-byte FCS (FHD = 0x40: Single_Segment=0, FCS_Flag=1)
        // Window_Descriptor = 0x08 (exp=1, mantissa=0 -> base = 1024 << 1 = 2048)
        // 2-byte FCS = 0x0100 (256) + 256 offset = 512
        let frame_2b: [8]u8 = [0x28, 0xB5, 0x2F, 0xFD, 0x40, 0x08, 0x00, 0x01];
        const parsed2 = ZstdFrameHeader.parse(frame_2b[0..8]);
        expect_eq(parsed2 != null, true);
        if (h2 := parsed2) {
            expect_eq(h2.single_segment, false);
            expect_eq(h2.window_size, 2048 as u64);
            expect_eq(h2.content_size, 512 as u64);
            expect_eq(h2.header_size, 8 as usize);
        }

        // Case C: Skippable frame magic (0x184D2A50 -> 50 2A 4D 18)
        let skippable: [4]u8 = [0x50, 0x2A, 0x4D, 0x18];
        expect_eq(ZstdFrameHeader.isSkippableHeader(skippable[0..4]), true);
        expect_eq(ZstdFrameHeader.isZstdHeader(skippable[0..4]), false);
    });

    test("3. Brotli (RFC 7932) Bound calculation, BrotliMode enums & BrotliHeader WBITS", () => {
        expect_eq(Brotli.bound(0), 2 as usize);
        expect_eq(Brotli.bound(2048) > 2048, true);
        expect_eq(Brotli.compressBound(2048), Brotli.bound(2048));

        expect_eq(BrotliMode.Generic as i32, 0);
        expect_eq(BrotliMode.Text as i32, 1);
        expect_eq(BrotliMode.Font as i32, 2);
        expect_eq(BrotliDecoderResult.Success as i32, 1);
        expect_eq(BrotliStreamState.Inflating as u8, 1);

        expect_eq(Brotli.isValidQuality(BROTLI_DEFAULT_QUALITY), true);
        expect_eq(Brotli.isValidQuality(12), false);
        expect_eq(Brotli.isValidWindowBits(BROTLI_DEFAULT_WINDOW, false), true);
        expect_eq(Brotli.isValidWindowBits(28, false), false);
        expect_eq(Brotli.isValidWindowBits(28, true), true);

        const opts = BrotliEncoderOptions{
            mode: BrotliMode.Text,
            quality: 9,
            lgwin: 20,
        };
        expect_eq(opts.mode == BrotliMode.Text, true);
        expect_eq(opts.quality, 9);

        // RFC 7932 WBITS decoding:
        // Bit 0 == 0 -> WBITS = 16; Bit 1 (ISLAST) = 1; Bit 2 (ISLASTEMPTY) = 1 -> byte = 0b0000_0110 = 0x06
        let empty_br: [1]u8 = [0x06];
        const bhdr_opt = BrotliHeader.parse(empty_br[0..1]);
        expect_eq(bhdr_opt != null, true);
        if (bhdr := bhdr_opt) {
            expect_eq(bhdr.window_bits, 16 as u8);
            expect_eq(bhdr.is_last, true);
            expect_eq(bhdr.is_empty, true);
        }

        // 4-bit WBITS: bit 0 = 1, bits 1..3 = 5 (WBITS = 17 + 5 = 22) -> low 4 bits = 0b1011 = 0x0B
        let w22_br: [1]u8 = [0x0B];
        const bhdr22 = BrotliHeader.parse(w22_br[0..1]);
        expect_eq(bhdr22 != null, true);
        expect_eq(bhdr22.?.window_bits, 22 as u8);
        expect_eq(bhdr22.?.is_last, false);
    });

    test("4. POSIX.1-1988 USTAR TarHeader encoding & TarReader zero-copy iteration", () => {
        // 2 entries: a directory "pkg/" (1 block) + a regular file "pkg/index.cp" (1 header + 1 data block) + 1 zero block = 4 blocks (2048 bytes)
        let tar_buf: [2048]u8 = [0; 2048];

        const ok_dir = TarHeader.encodeHeader(
            tar_buf[0..512],
            "pkg/",
            0,
            0o755,
            1700000000,
            TarTypeFlag.Directory as u8,
        );
        expect_eq(ok_dir, true);

        const file_data = "export const VERSION: i32 = 1;";
        const ok_file = TarHeader.encodeHeader(
            tar_buf[512..1024],
            "pkg/index.cp",
            file_data.len as u64,
            0o644,
            1700000100,
            TarTypeFlag.Regular as u8,
        );
        expect_eq(ok_file, true);
        @memcpy(tar_buf[1024 .. 1024 + file_data.len], file_data);

        let reader = TarReader.init(tar_buf[0..2048]);

        const e1_opt = reader.next();
        expect_eq(e1_opt != null, true);
        if (e1 := e1_opt) {
            expect_eq(e1.name == "pkg/", true);
            expect_eq(e1.isDirectory(), true);
            expect_eq(e1.isRegularFile(), false);
            expect_eq(e1.size, 0 as u64);
        }

        const e2_opt = reader.next();
        expect_eq(e2_opt != null, true);
        if (e2 := e2_opt) {
            expect_eq(e2.name == "pkg/index.cp", true);
            expect_eq(e2.isRegularFile(), true);
            expect_eq(e2.isDirectory(), false);
            expect_eq(e2.mode, 0o644 as u32);
            expect_eq(e2.data == file_data, true);
        }

        expect_eq(reader.next() == null, true);
    });

    test("5. PKZIP LocalHeader, CentralDirectory, EOCD locate & ZipReader verification", () => {
        let zip_buf: [512]u8 = [0; 512];
        const f1_name = "a.txt";
        const f1_data = "Alpha payload";
        const f2_name = "b.txt";
        const f2_data = "Beta payload 12345";

        // Encode 2 local file entries
        const len1 = ZipLocalHeader.encodeEntry(zip_buf[0..128], f1_name, f1_data);
        expect_eq(len1, ZIP_LOCAL_HEADER_SIZE + f1_name.len + f1_data.len);

        const len2 = ZipLocalHeader.encodeEntry(zip_buf[len1 .. len1 + 128], f2_name, f2_data);
        expect_eq(len2, ZIP_LOCAL_HEADER_SIZE + f2_name.len + f2_data.len);

        // Encode Central Directory header for entry 1
        const cd_offset = len1 + len2;
        const cd1 = ZipCentralHeader{
            compression_method: ZIP_METHOD_STORE,
            crc32: Crc32.checksum(f1_data),
            compressed_size: f1_data.len as u32,
            uncompressed_size: f1_data.len as u32,
            name_len: f1_name.len as u16,
            local_header_offset: 0,
        };
        const cd1_written = cd1.write(zip_buf[cd_offset .. cd_offset + ZIP_CENTRAL_DIR_SIZE]);
        expect_eq(cd1_written, ZIP_CENTRAL_DIR_SIZE);
        @memcpy(zip_buf[cd_offset + ZIP_CENTRAL_DIR_SIZE .. cd_offset + ZIP_CENTRAL_DIR_SIZE + f1_name.len], f1_name);
        const cd_size = ZIP_CENTRAL_DIR_SIZE + f1_name.len;

        // Parse Central Directory header back
        const parsed_cd = ZipCentralHeader.parse(zip_buf[cd_offset .. cd_offset + ZIP_CENTRAL_DIR_SIZE]);
        expect_eq(parsed_cd != null, true);
        expect_eq(parsed_cd.?.crc32, Crc32.checksum(f1_data));

        // Encode EOCD at the end
        const eocd_offset = cd_offset + cd_size;
        const eocd = ZipEocd{
            entries_on_disk: 2,
            total_entries: 2,
            central_dir_size: cd_size as u32,
            central_dir_offset: cd_offset as u32,
        };
        const eocd_written = eocd.write(zip_buf[eocd_offset .. eocd_offset + ZIP_EOCD_SIZE]);
        expect_eq(eocd_written, ZIP_EOCD_SIZE);

        const total_zip_len = eocd_offset + ZIP_EOCD_SIZE;

        // Locate EOCD by backward scan
        const found_eocd = ZipEocd.findInBuffer(zip_buf[0..total_zip_len]);
        expect_eq(found_eocd != null, true);
        if (e := found_eocd) {
            expect_eq(e.total_entries, 2 as u16);
            expect_eq(e.central_dir_offset, cd_offset as u32);
            expect_eq(e.central_dir_size, cd_size as u32);
        }

        // Iterate local entries via ZipReader
        let zreader = ZipReader.init(zip_buf[0..total_zip_len]);
        const ze1 = zreader.next();
        expect_eq(ze1 != null, true);
        if (z1 := ze1) {
            expect_eq(z1.name == f1_name, true);
            expect_eq(z1.data == f1_data, true);
            expect_eq(z1.isStored(), true);
            expect_eq(z1.verifyCrc(), true);
        }

        const ze2 = zreader.next();
        expect_eq(ze2 != null, true);
        if (z2 := ze2) {
            expect_eq(z2.name == f2_name, true);
            expect_eq(z2.data == f2_data, true);
            expect_eq(z2.verifyCrc(), true);
        }

        // Next encounters Central Directory signature (not Local Header), so returns null
        expect_eq(zreader.next() == null, true);
    });

    test("6. Crc32 (IEEE 802.3) & Adler32 (RFC 1950) single-shot & incremental streaming", () => {
        // IEEE 802.3 CRC-32 standard check vector: "123456789" -> 0xCBF43926
        expect_eq(Crc32.checksum("123456789"), 0xCBF43926 as u32);

        let crc = Crc32.init();
        crc.update("123");
        crc.update("456");
        crc.update("789");
        expect_eq(crc.final(), 0xCBF43926 as u32);

        crc.reset();
        expect_eq(crc.final(), 0 as u32);

        // RFC 1950 Adler-32 standard check vector: "Wikipedia" -> 0x11E60398
        expect_eq(Adler32.checksum("Wikipedia"), 0x11E60398 as u32);

        let adler = Adler32.init();
        adler.update("Wiki");
        adler.update("pedia");
        expect_eq(adler.final(), 0x11E60398 as u32);

        adler.reset();
        expect_eq(adler.final(), 1 as u32);
    });
}
