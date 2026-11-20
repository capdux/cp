// ============================================================================
// cap/test/test_pack_production_deep.cp
// cap:pack 工业级压缩与归档引擎深度回归测试套件
// 覆盖:
//   1. ZipWriter (Stored + Deflate + Directory + UTF-8 bit 11) 与 ZipReader 无损读回 & CRC32 校验
//   2. PaxTarWriter / PaxTarReader 常规文件、目录、符号链接、权限位与 >150 字节超长路径 PAX 'x' 往返
//   3. validateArchivePath 路径穿越 / 绝对路径 / 空字节拦截与 PaxTarReader 严格安全防御
//   4. ArchiveSniffer.detect 全魔数格式识别、边界短切片、mimeType 与 extension 映射
//   5. cap:pack 聚合导入、实际产物嗅探联动与既有 gzip / tar / zip 零回归
// ============================================================================

import { test, expect_eq } from "cap:test";
import {
    // New modules
    ZipCompressionMethod,
    ZipWriteEntry,
    ZipWriter,
    ZIP_FLAG_UTF8,
    PaxTarEntry,
    PaxTarWriter,
    PaxTarReader,
    validateArchivePath,
    ArchiveMagicFormat,
    ArchiveSniffer,
    // Existing modules (compatibility & cross-verification)
    ZipLocalHeader,
    ZipCentralHeader,
    ZipEocd,
    ZipEntryView,
    ZipReader,
    ZIP_METHOD_STORE,
    ZIP_METHOD_DEFLATE,
    TarHeader,
    TarEntryView,
    TarReader,
    TarTypeFlag,
    gzip,
    gunzip,
    deflate,
    inflate,
    deflateRaw,
    inflateRaw,
    Crc32,
    Adler32,
} from "cap:pack";

fn main() {
    test("1. ZipWriter Stored + Deflate + Directory + UTF-8 flag & ZipReader lossless roundtrip", () => {
        let zw = ZipWriter.init();
        expect_eq(zw.count(), 0 as u16);

        const ok_dir = zw.addDirectory("docs");
        expect_eq(ok_dir, true);

        const stored_name = "docs/readme.txt";
        const stored_data = "PKZIP Stored entry payload with exact IEEE CRC-32 verification.";
        expect_eq(zw.addStored(stored_name, stored_data), true);

        const deflate_name = "src/重复压缩测试_payload.txt";
        const deflate_data = "CP-Lang Deflate ZIP compression! CP-Lang Deflate ZIP compression! CP-Lang Deflate ZIP compression! 1234567890.";
        expect_eq(zw.addDeflate(deflate_name, deflate_data, 6), true);

        expect_eq(zw.setComment("cap:pack ZipWriter"), true);
        expect_eq(zw.count(), 3 as u16);

        const zip_bytes = zw.finish();
        expect_eq(zip_bytes.len > 100, true);

        // Verify EOCD via ZipEocd.findInBuffer
        const eocd_opt = ZipEocd.findInBuffer(zip_bytes);
        expect_eq(eocd_opt != null, true);
        if (eocd := eocd_opt) {
            expect_eq(eocd.total_entries, 3 as u16);
            expect_eq(eocd.entries_on_disk, 3 as u16);
            expect_eq(eocd.comment_len, 18 as u16);

            // Parse first Central Directory Header and check UTF-8 flag (bit 11)
            const cd_off = eocd.central_dir_offset as usize;
            const cd1_opt = ZipCentralHeader.parse(zip_bytes[cd_off..]);
            expect_eq(cd1_opt != null, true);
            if (cd1 := cd1_opt) {
                expect_eq((cd1.flags & ZIP_FLAG_UTF8) != 0, true);
            }
        }

        // Verify Local Header UTF-8 flag on first entry
        const lhdr1 = ZipLocalHeader.parse(zip_bytes).?;
        expect_eq((lhdr1.flags & ZIP_FLAG_UTF8) != 0, true);

        // Read all 3 entries back via ZipReader
        let zr = ZipReader.init(zip_bytes);

        const e1 = zr.next().?;
        expect_eq(e1.name == "docs/", true);
        expect_eq(e1.isStored(), true);
        expect_eq(e1.uncompressed_size, 0 as u32);
        expect_eq(e1.verifyCrc(), true);

        const e2 = zr.next().?;
        expect_eq(e2.name == stored_name, true);
        expect_eq(e2.isStored(), true);
        expect_eq(e2.verifyCrc(), true);
        const e2_plain = ZipWriter.decompressAndVerify(e2).?;
        expect_eq(e2_plain == stored_data, true);

        const e3 = zr.next().?;
        expect_eq(e3.name == deflate_name, true);
        expect_eq(e3.compression_method, ZIP_METHOD_DEFLATE);
        expect_eq(e3.crc32, Crc32.checksum(deflate_data));
        expect_eq(e3.uncompressed_size, deflate_data.len as u32);
        const e3_plain = ZipWriter.decompressAndVerify(e3).?;
        expect_eq(e3_plain == deflate_data, true);

        // End of local entries
        expect_eq(zr.next() == null, true);
    });

    test("2. PaxTarWriter & PaxTarReader regular, directory, symlink & >150B PAX 'x' long path roundtrip", () => {
        let tw = PaxTarWriter.init();

        // Standard directory with POSIX mode 0o750
        expect_eq(tw.addDirectory("workspace/sub", 0o750, 1700000010), true);

        // Standard regular file with executable mode 0o755
        const bin_data = "#!/bin/sh\necho ok\n";
        expect_eq(tw.addFile("workspace/sub/run.sh", bin_data, 0o755, 1700000020), true);

        // >150 byte ultra-long path forcing POSIX.1-2001 PAX extended header (typeflag 'x')
        const long_path = "deep/nested/directory/hierarchy/level_01/level_02/level_03/level_04/level_05/level_06/level_07/level_08/level_09/level_10/very_long_filename_exceeding_150_bytes_limit_check.cp";
        expect_eq(long_path.len > 150, true);
        const long_payload = "PAX extended header payload verified!";
        expect_eq(tw.addFile(long_path, long_payload, 0o640, 1700000030), true);

        // Symlink with >100 byte linkpath
        const long_target = "../relative/symlink/target/segment_a/segment_b/segment_c/segment_d/segment_e/segment_f/segment_g/actual_target_file.txt";
        expect_eq(long_target.len > 100, true);
        expect_eq(tw.addSymlink("workspace/sub/link_to_target", long_target, 0o777, 1700000040), true);

        const tar_bytes = tw.finish();
        expect_eq(tar_bytes.len >= 512 * 6, true);

        let tr = PaxTarReader.init(tar_bytes);

        const r1 = tr.next().?;
        expect_eq(r1.name == "workspace/sub/", true);
        expect_eq(r1.isDirectory(), true);
        expect_eq(r1.mode, 0o750 as u32);

        const r2 = tr.next().?;
        expect_eq(r2.name == "workspace/sub/run.sh", true);
        expect_eq(r2.isRegularFile(), true);
        expect_eq(r2.mode, 0o755 as u32);
        expect_eq(r2.data == bin_data, true);

        const r3 = tr.next().?;
        expect_eq(r3.name == long_path, true);
        expect_eq(r3.isRegularFile(), true);
        expect_eq(r3.mode, 0o640 as u32);
        expect_eq(r3.size, long_payload.len as u64);
        expect_eq(r3.data == long_payload, true);

        const r4 = tr.next().?;
        expect_eq(r4.name == "workspace/sub/link_to_target", true);
        expect_eq(r4.isSymlink(), true);
        expect_eq(r4.linkname == long_target, true);

        expect_eq(tr.next() == null, true);
    });

    test("3. validateArchivePath traversal/absolute/NUL rejection & PaxTarReader strict defense", () => {
        // Valid relative paths
        expect_eq(validateArchivePath("a.txt"), true);
        expect_eq(validateArchivePath("pkg/src/main.cp"), true);
        expect_eq(validateArchivePath(".gitignore"), true);
        expect_eq(validateArchivePath("dir/file..name.txt"), true);

        // Invalid / unsafe paths
        expect_eq(validateArchivePath(""), false);
        expect_eq(validateArchivePath("/etc/passwd"), false);
        expect_eq(validateArchivePath("\\Windows\\System32\\cmd.exe"), false);
        expect_eq(validateArchivePath("C:\\escape.txt"), false);
        expect_eq(validateArchivePath(".."), false);
        expect_eq(validateArchivePath("../secret.txt"), false);
        expect_eq(validateArchivePath("a/b/../../etc/shadow"), false);
        expect_eq(validateArchivePath("a/.."), false);
        expect_eq(validateArchivePath("a\\..\\b"), false);

        let nul_path: [7]u8 = ['a', '/', 'b', 0, 'c', '.', 't'];
        expect_eq(validateArchivePath(nul_path[0..7]), false);

        // Writer refuses unsafe paths
        let tw = PaxTarWriter.init();
        expect_eq(tw.addFile("../escape.txt", "bad", 0o644, 0), false);
        expect_eq(tw.addFile("/abs/path.txt", "bad", 0o644, 0), false);
        expect_eq(tw.addDirectory("a/../b", 0o755, 0), false);

        // Hand-craft a raw USTAR archive containing a traversal path and verify strict reader blocks it
        let bad_tar: [1024]u8 = [0; 1024];
        expect_eq(TarHeader.encodeHeader(bad_tar[0..512], "../evil.sh", 0, 0o644, 0, '0'), true);
        let strict_reader = PaxTarReader.init(bad_tar[0..1024], true);
        expect_eq(strict_reader.next() == null, true);

        // Non-strict mode still parses when explicitly requested
        let permissive_reader = PaxTarReader.init(bad_tar[0..1024], false);
        const raw_entry = permissive_reader.next().?;
        expect_eq(raw_entry.name == "../evil.sh", true);
    });

    test("4. ArchiveSniffer.detect magic formats, short slices, mimeType & extension", () => {
        // Boundary short / unknown slices
        expect_eq(ArchiveSniffer.detect("") == ArchiveMagicFormat.Unknown, true);
        let one_zero: [1]u8 = [0x00];
        expect_eq(ArchiveSniffer.detect(one_zero[0..1]) == ArchiveMagicFormat.Unknown, true);
        let rand_bytes: [4]u8 = [0xFF, 0x00, 0x12, 0x34];
        expect_eq(ArchiveSniffer.detect(rand_bytes[0..4]) == ArchiveMagicFormat.Unknown, true);
        expect_eq(ArchiveSniffer.detect("plain ascii text") == ArchiveMagicFormat.Unknown, true);

        // Gzip (1F 8B 08) & 2-byte prefix
        let gz_magic: [3]u8 = [0x1F, 0x8B, 0x08];
        expect_eq(ArchiveSniffer.detect(gz_magic[0..2]) == ArchiveMagicFormat.Gzip, true);
        expect_eq(ArchiveSniffer.detect(gz_magic[0..3]) == ArchiveMagicFormat.Gzip, true);
        expect_eq(ArchiveSniffer.mimeType(ArchiveMagicFormat.Gzip) == "application/gzip", true);
        expect_eq(ArchiveSniffer.extension(ArchiveMagicFormat.Gzip) == ".gz", true);

        // Zlib RFC 1950 (78 9C -> 0x789C % 31 == 0, 78 01, 78 DA)
        let zl_default: [2]u8 = [0x78, 0x9C];
        let zl_best: [2]u8 = [0x78, 0xDA];
        let zl_bad_chk: [2]u8 = [0x78, 0x9D];
        expect_eq(ArchiveSniffer.detect(zl_default[0..2]) == ArchiveMagicFormat.Zlib, true);
        expect_eq(ArchiveSniffer.detect(zl_best[0..2]) == ArchiveMagicFormat.Zlib, true);
        expect_eq(ArchiveSniffer.detect(zl_bad_chk[0..2]) == ArchiveMagicFormat.Unknown, true);
        expect_eq(ArchiveSniffer.mimeType(ArchiveMagicFormat.Zlib) == "application/zlib", true);
        expect_eq(ArchiveSniffer.extension(ArchiveMagicFormat.Zlib) == ".zz", true);

        // Zstd (28 B5 2F FD)
        let zst_magic: [4]u8 = [0x28, 0xB5, 0x2F, 0xFD];
        expect_eq(ArchiveSniffer.detect(zst_magic[0..4]) == ArchiveMagicFormat.Zstd, true);
        expect_eq(ArchiveSniffer.mimeType(ArchiveMagicFormat.Zstd) == "application/zstd", true);
        expect_eq(ArchiveSniffer.extension(ArchiveMagicFormat.Zstd) == ".zst", true);

        // ZIP Local (50 4B 03 04) & Empty EOCD (50 4B 05 06)
        let zip_local: [4]u8 = [0x50, 0x4B, 0x03, 0x04];
        let zip_empty: [4]u8 = [0x50, 0x4B, 0x05, 0x06];
        expect_eq(ArchiveSniffer.detect(zip_local[0..4]) == ArchiveMagicFormat.Zip, true);
        expect_eq(ArchiveSniffer.detect(zip_empty[0..4]) == ArchiveMagicFormat.Zip, true);
        expect_eq(ArchiveSniffer.mimeType(ArchiveMagicFormat.Zip) == "application/zip", true);
        expect_eq(ArchiveSniffer.extension(ArchiveMagicFormat.Zip) == ".zip", true);

        // XZ (FD 37 7A 58 5A 00)
        let xz_magic: [6]u8 = [0xFD, 0x37, 0x7A, 0x58, 0x5A, 0x00];
        expect_eq(ArchiveSniffer.detect(xz_magic[0..6]) == ArchiveMagicFormat.Xz, true);
        expect_eq(ArchiveSniffer.mimeType(ArchiveMagicFormat.Xz) == "application/x-xz", true);
        expect_eq(ArchiveSniffer.extension(ArchiveMagicFormat.Xz) == ".xz", true);

        // BZ2 ("BZh9")
        let bz2_magic: [4]u8 = ['B', 'Z', 'h', '9'];
        expect_eq(ArchiveSniffer.detect(bz2_magic[0..4]) == ArchiveMagicFormat.Bz2, true);
        expect_eq(ArchiveSniffer.mimeType(ArchiveMagicFormat.Bz2) == "application/x-bzip2", true);
        expect_eq(ArchiveSniffer.extension(ArchiveMagicFormat.Bz2) == ".bz2", true);

        // LZ4 (04 22 4D 18)
        let lz4_magic: [4]u8 = [0x04, 0x22, 0x4D, 0x18];
        expect_eq(ArchiveSniffer.detect(lz4_magic[0..4]) == ArchiveMagicFormat.Lz4, true);
        expect_eq(ArchiveSniffer.mimeType(ArchiveMagicFormat.Lz4) == "application/x-lz4", true);
        expect_eq(ArchiveSniffer.extension(ArchiveMagicFormat.Lz4) == ".lz4", true);

        // TAR (ustar at offset 257)
        let tar_hdr: [512]u8 = [0; 512];
        expect_eq(TarHeader.encodeHeader(tar_hdr[0..512], "test.txt", 0, 0o644, 0, '0'), true);
        expect_eq(ArchiveSniffer.detect(tar_hdr[0..512]) == ArchiveMagicFormat.Tar, true);
        expect_eq(ArchiveSniffer.mimeType(ArchiveMagicFormat.Tar) == "application/x-tar", true);
        expect_eq(ArchiveSniffer.extension(ArchiveMagicFormat.Tar) == ".tar", true);

        // BrotliHint (empty final Brotli stream 0x06)
        let br_empty: [1]u8 = [0x06];
        expect_eq(ArchiveSniffer.detect(br_empty[0..1]) == ArchiveMagicFormat.BrotliHint, true);
        expect_eq(ArchiveSniffer.mimeType(ArchiveMagicFormat.BrotliHint) == "application/x-brotli", true);
        expect_eq(ArchiveSniffer.extension(ArchiveMagicFormat.BrotliHint) == ".br", true);
        expect_eq(ArchiveSniffer.mimeType(ArchiveMagicFormat.Unknown) == "application/octet-stream", true);
        expect_eq(ArchiveSniffer.extension(ArchiveMagicFormat.Unknown) == "", true);
    });

    test("5. cap:pack aggregate integration: live outputs sniffed & zero regression on gzip/tar/zip", () => {
        const sample = "End-to-end integration of cap:pack codecs, archives, and ArchiveSniffer.";

        // Live gzip output sniffed as Gzip and restored cleanly
        const gz = gzip(sample, 6);
        expect_eq(ArchiveSniffer.detect(gz) == ArchiveMagicFormat.Gzip, true);
        expect_eq(gunzip(gz).? == sample, true);

        // Live zlib deflate output sniffed as Zlib and restored cleanly
        const zl = deflate(sample, 6);
        expect_eq(ArchiveSniffer.detect(zl) == ArchiveMagicFormat.Zlib, true);
        expect_eq(inflate(zl).? == sample, true);

        // Live ZipWriter output sniffed as Zip
        let zw = ZipWriter.init();
        expect_eq(zw.addDeflate("hello.txt", sample, 6), true);
        const zbytes = zw.finish();
        expect_eq(ArchiveSniffer.detect(zbytes) == ArchiveMagicFormat.Zip, true);

        // Live PaxTarWriter output sniffed as Tar and readable by legacy TarReader for <=100B USTAR entries
        let tw = PaxTarWriter.init();
        expect_eq(tw.addFile("hello.txt", sample, 0o644, 1700000000), true);
        const tbytes = tw.finish();
        expect_eq(ArchiveSniffer.detect(tbytes) == ArchiveMagicFormat.Tar, true);

        let legacy_tr = TarReader.init(tbytes);
        const t_entry = legacy_tr.next().?;
        expect_eq(t_entry.name == "hello.txt", true);
        expect_eq(t_entry.data == sample, true);

        // Checksums zero-regression
        expect_eq(Crc32.checksum("123456789"), 0xCBF43926 as u32);
        expect_eq(Adler32.checksum("Wikipedia"), 0x11E60398 as u32);
    });
}
