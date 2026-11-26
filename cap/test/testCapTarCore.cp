import { test, expect_eq } from "cap:test";
import {
    TAR_BLOCK_SIZE, TYPE_FILE, TYPE_SYMLINK, TYPE_DIR,
    TarHeader, TarEntrySpan, TarArchiveCursor,
    encodeTarHeader, parseTarHeader, isZeroBlock, tarPaddedSize
} from "../../packages/@cap/tar-core/src/index.cp";

fn main() {
    test("1. @cap/tar-core — encodeTarHeader & parseTarHeader round-trip USTAR file", () => {
        let blk: [512]u8 = [0; 512];
        let ok = encodeTarHeader("hello/world.txt", 13, 420, 1700000000, TYPE_FILE, blk[0..512]);
        expect_eq(ok, true);
        expect_eq(isZeroBlock(blk[0..512]), false);

        let hdr = parseTarHeader(blk[0..512]);
        expect_eq(hdr.valid, true);
        expect_eq(hdr.is_ustar, true);
        expect_eq(hdr.name, "hello/world.txt");
        expect_eq(hdr.mode, 420);
        expect_eq(hdr.uid, 0);
        expect_eq(hdr.gid, 0);
        expect_eq(hdr.size, 13);
        expect_eq(hdr.mtime, 1700000000);
        expect_eq(hdr.typeflag, TYPE_FILE);
        expect_eq(hdr.checksum > 0, true);
    });

    test("2. @cap/tar-core — checksum verification rejects corrupted header", () => {
        let blk: [512]u8 = [0; 512];
        let ok = encodeTarHeader("config.json", 256, 420, 1710000000, TYPE_FILE, blk[0..512]);
        expect_eq(ok, true);

        blk[5] = blk[5] ^ 1;
        let bad = parseTarHeader(blk[0..512]);
        expect_eq(bad.valid, false);
    });

    test("3. @cap/tar-core — isZeroBlock detects EOF zero block & rejects short/non-zero", () => {
        let zeros: [512]u8 = [0; 512];
        expect_eq(isZeroBlock(zeros[0..512]), true);
        expect_eq(isZeroBlock(zeros[0..256]), false);

        let eof_hdr = parseTarHeader(zeros[0..512]);
        expect_eq(eof_hdr.valid, false);

        zeros[511] = 1;
        expect_eq(isZeroBlock(zeros[0..512]), false);
    });

    test("4. @cap/tar-core — tarPaddedSize aligns to 512-byte block boundaries", () => {
        expect_eq(TAR_BLOCK_SIZE, 512);
        expect_eq(tarPaddedSize(0), 0);
        expect_eq(tarPaddedSize(1), 512);
        expect_eq(tarPaddedSize(511), 512);
        expect_eq(tarPaddedSize(512), 512);
        expect_eq(tarPaddedSize(513), 1024);
        expect_eq(tarPaddedSize(1024), 1024);
    });

    test("5. @cap/tar-core — directory & symlink typeflags plus validation guards", () => {
        let dir_blk: [512]u8 = [0; 512];
        expect_eq(encodeTarHeader("src/", 0, 493, 1700000100, TYPE_DIR, dir_blk[0..512]), true);
        let dir_hdr = parseTarHeader(dir_blk[0..512]);
        expect_eq(dir_hdr.valid, true);
        expect_eq(dir_hdr.typeflag, TYPE_DIR);
        expect_eq(dir_hdr.mode, 493);

        let sym_blk: [512]u8 = [0; 512];
        expect_eq(encodeTarHeader("latest", 0, 511, 1700000200, TYPE_SYMLINK, sym_blk[0..512]), true);
        let sym_hdr = parseTarHeader(sym_blk[0..512]);
        expect_eq(sym_hdr.valid, true);
        expect_eq(sym_hdr.typeflag, TYPE_SYMLINK);

        // Empty name and short buffer rejected
        expect_eq(encodeTarHeader("", 0, 420, 0, TYPE_FILE, dir_blk[0..512]), false);
        expect_eq(encodeTarHeader("a.txt", 0, 420, 0, TYPE_FILE, dir_blk[0..100]), false);
    });

    test("6. @cap/tar-core — TarArchiveCursor traverses multi-entry archive to EOF", () => {
        // Layout: [hdr1:512][data1:512 (5 bytes "hello")][hdr2:512 (dir, 0 bytes)][hdr3:512][data3:512 (3 bytes "abc")][eof:1024]
        let archive: [3584]u8 = [0; 3584];

        expect_eq(encodeTarHeader("a.txt", 5, 420, 1000, TYPE_FILE, archive[0..512]), true);
        archive[512] = 104; // 'h'
        archive[513] = 101; // 'e'
        archive[514] = 108; // 'l'
        archive[515] = 108; // 'l'
        archive[516] = 111; // 'o'

        expect_eq(encodeTarHeader("sub/", 0, 493, 1001, TYPE_DIR, archive[1024..1536]), true);

        expect_eq(encodeTarHeader("sub/b.bin", 3, 420, 1002, TYPE_FILE, archive[1536..2048]), true);
        archive[2048] = 97; // 'a'
        archive[2049] = 98; // 'b'
        archive[2050] = 99; // 'c'

        let cursor = TarArchiveCursor.init();
        let e1 = cursor.nextEntry(archive[0..3584]);
        expect_eq(e1 != null, true);
        let s1 = e1 ?? TarEntrySpan { header: parseTarHeader(archive[2560..3072]), data_offset: 0, data_len: 0 };
        expect_eq(s1.header.name, "a.txt");
        expect_eq(s1.data_offset, 512);
        expect_eq(s1.data_len, 5);
        expect_eq(archive[s1.data_offset], 104);

        let e2 = cursor.nextEntry(archive[0..3584]);
        expect_eq(e2 != null, true);
        let s2 = e2 ?? TarEntrySpan { header: parseTarHeader(archive[2560..3072]), data_offset: 0, data_len: 0 };
        expect_eq(s2.header.name, "sub/");
        expect_eq(s2.header.typeflag, TYPE_DIR);
        expect_eq(s2.data_len, 0);

        let e3 = cursor.nextEntry(archive[0..3584]);
        expect_eq(e3 != null, true);
        let s3 = e3 ?? TarEntrySpan { header: parseTarHeader(archive[2560..3072]), data_offset: 0, data_len: 0 };
        expect_eq(s3.header.name, "sub/b.bin");
        expect_eq(s3.data_offset, 2048);
        expect_eq(s3.data_len, 3);
        expect_eq(archive[s3.data_offset + 2], 99);

        let e4 = cursor.nextEntry(archive[0..3584]);
        expect_eq(e4 == null, true);
        expect_eq(cursor.finished, true);
    });
}
