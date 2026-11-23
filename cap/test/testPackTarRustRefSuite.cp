import { test, expect_eq } from "cap:test";
import {
    TAR_BLOCK_SIZE,
    TarTypeFlag,
    TarHeader,
    TarReader,
} from "../../lib/pack/tar.cp";
import {
    validateArchivePathScalar,
    validateArchivePath,
    PaxTarWriter,
    PaxTarReader,
} from "../../lib/pack/tarPax.cp";

const std = @import("std");

fn expect_true(cond: bool): void! {
    expect_eq(cond, true);
}

fn bytesEqual(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

// _ref/tar-rs/tests/archives/empty_filename.tar (512 bytes, MIT/Apache-2.0)
const TAR_RS_EMPTY_FILENAME: [512]u8 = [
    0x00, 0x00, 0x00, 0x00, 0x75, 0x73, 0x74, 0x61, 0x72, 0x20, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x30, 0x30, 0x30, 0x30, 0x37, 0x35, 0x35, 0x00, 0x30, 0x30, 0x30, 0x31,
    0x37, 0x35, 0x30, 0x00, 0x30, 0x30, 0x30, 0x31, 0x37, 0x35, 0x30, 0x00, 0x30, 0x30, 0x30, 0x30,
    0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x00, 0x31, 0x32, 0x34, 0x34, 0x30, 0x30, 0x32, 0x31,
    0x35, 0x37, 0x34, 0x00, 0x30, 0x31, 0x30, 0x36, 0x34, 0x37, 0x00, 0x20, 0x35, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x75, 0x73, 0x74, 0x61, 0x72, 0x20, 0x20, 0x00, 0x6d, 0x76, 0x64, 0x34, 0x32, 0x35, 0x33,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x36, 0x34, 0x37, 0x00, 0x20, 0x35,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
];

fn finalizeHeaderChecksum(block: []u8) {
    const chksum = TarHeader.computeChecksum(block[0..TAR_BLOCK_SIZE]);
    TarHeader.writeOctal(block[148..155], chksum as u64);
    block[155] = ' ';
}

fn runTestUstarPaxAndTarRsVectors(): void! {
    // 1a. PaxTarWriter -> PaxTarReader roundtrip (regular, dir, symlink, >120B PAX 'x' path & linkpath)
    let tw = PaxTarWriter.init();
    expect_true(tw.addFile("hello.txt", "Hello USTAR/PAX!", 0o644, 1700000000))?;
    expect_true(tw.addDirectory("sub/dir", 0o755, 1700000001))?;

    let long_name: [130]u8 = ['a'; 130];
    long_name[0] = 'd';
    long_name[1] = '/';
    expect_true(tw.addFile(long_name[0..], "PAX-Long-Content", 0o600, 1700000002))?;

    let long_link: [125]u8 = ['b'; 125];
    long_link[0] = '.';
    long_link[1] = '.';
    long_link[2] = '/';
    expect_true(tw.addSymlink("links/sym", long_link[0..], 0o777, 1700000003))?;

    const tar_bytes = tw.finish();
    expect_true(!TarHeader.isZeroBlock(tar_bytes[0..512]))?;
    expect_true(TarHeader.verifyChecksum(tar_bytes[0..512]))?;
    expect_true(validateArchivePath("hello.txt"))?;
    let pr = PaxTarReader.init(tar_bytes, true);

    const e1 = pr.next().?;
    expect_true(bytesEqual(e1.name, "hello.txt"))?;
    expect_true(e1.isRegularFile())?;
    expect_true(bytesEqual(e1.data, "Hello USTAR/PAX!"))?;
    expect_eq(e1.mode, 0o644);
    expect_eq(e1.mtime, 1700000000);

    const e2 = pr.next().?;
    expect_true(bytesEqual(e2.name, "sub/dir/"))?;
    expect_true(e2.isDirectory())?;

    const e3 = pr.next().?;
    expect_true(bytesEqual(e3.name, long_name[0..]))?;
    expect_true(bytesEqual(e3.data, "PAX-Long-Content"))?;

    const e4 = pr.next().?;
    expect_true(bytesEqual(e4.name, "links/sym"))?;
    expect_true(e4.isSymlink())?;
    expect_true(bytesEqual(e4.linkname, long_link[0..]))?;
    expect_true(pr.next() == null)?;

    // 1b. Backward-compatible TarReader on simple USTAR
    let ustar_buf: [1536]u8 = [0; 1536];
    expect_true(TarHeader.encodeHeader(ustar_buf[0..512], "legacy.txt", 5, 0o644, 12345, '0'))?;
    TarHeader.copyPayloadSimd(ustar_buf[512..517], "12345");
    let tr = TarReader.init(ustar_buf[0..]);
    const le = tr.next().?;
    expect_true(bytesEqual(le.name, "legacy.txt"))?;
    expect_true(bytesEqual(le.data, "12345"))?;
    expect_true(tr.next() == null)?;

    // 1c. _ref/tar-rs empty_filename.tar: strict mode rejects empty name, permissive mode reads dir entry
    let strict_empty = PaxTarReader.init(TAR_RS_EMPTY_FILENAME[0..], true);
    expect_true(strict_empty.next() == null)?;
    let perm_empty = PaxTarReader.init(TAR_RS_EMPTY_FILENAME[0..], false);
    const pe = perm_empty.next().?;
    expect_eq(pe.name.len, 0);
    expect_true(pe.isDirectory())?;
}

fn runTestGnuBase256GlobalPaxAndSignedChecksum(): void! {
    // 2a. GNU Base-256 numeric parsing (>8GB unsigned & negative signed timestamp)
    let b256_u: [12]u8 = [0x80, 0, 0, 0, 0, 0, 0, 0x02, 0x00, 0x00, 0x00, 0x01]; // 2^32 + 1 = 4294967297 (> 8GB? let's use 0x03_0000_0001 = 12884901889)
    b256_u[7] = 0x03;
    const parsed_big = TarHeader.parseNumeric(b256_u[0..]).?;
    expect_eq(parsed_big, 12884901889);

    // Negative Base-256 (-1): all 0xFF
    const b256_neg: [12]u8 = [0xFF; 12];
    expect_true(TarHeader.parseNumeric(b256_neg[0..]) == null)?;
    expect_eq(TarHeader.parseSignedNumeric(b256_neg[0..]).?, -1);

    // 2b. GNU LongName ('L') + GNU LongLink ('K') + PAX Global Header ('g')
    let buf: [4096]u8 = [0; 4096];
    let pos: usize = 0;

    // PAX Global Header ('g'): "13 uid=9999\n" (len=12: "12 uid=9999\n")
    const g_rec = "12 uid=9999\n";
    * = TarHeader.encodeHeader(buf[pos .. pos + 512], "GlobalHead", g_rec.len as u64, 0o644, 0, 'g');
    pos += 512;
    TarHeader.copyPayloadSimd(buf[pos .. pos + g_rec.len], g_rec);
    pos += 512;

    // GNU LongName ('L'): 110-byte path
    let gnu_name: [110]u8 = ['x'; 110];
    gnu_name[0] = 'g';
    gnu_name[1] = 'n';
    gnu_name[2] = 'u';
    gnu_name[3] = '/';
    * = TarHeader.encodeHeader(buf[pos .. pos + 512], "././@LongLink", 110, 0o644, 0, 'L');
    pos += 512;
    TarHeader.copyPayloadSimd(buf[pos .. pos + 110], gnu_name[0..]);
    pos += 512;

    // Actual entry with truncated name, inheriting global uid=9999 and GNU LongName
    * = TarHeader.encodeHeader(buf[pos .. pos + 512], "gnu/xxx", 4, 0o644, 555, '0');
    pos += 512;
    TarHeader.copyPayloadSimd(buf[pos .. pos + 4], "DATA");
    pos += 512;
    // Two zero blocks already zeroed at end

    let pr = PaxTarReader.init(buf[0..], true);
    const ge = pr.next().?;
    expect_true(bytesEqual(ge.name, gnu_name[0..]))?;
    expect_true(bytesEqual(ge.data, "DATA"))?;
    expect_eq(ge.uid, 9999);
    expect_true(pr.next() == null)?;

    // 2c. Legacy signed checksum verification (high-bit byte >= 0x80 in name field)
    let signed_blk: [512]u8 = [0; 512];
    * = TarHeader.encodeHeader(signed_blk[0..], "legacy.bin", 0, 0o644, 0, '0');
    signed_blk[0] = 0xE9; // non-ASCII byte (>= 128 -> negative i8 = -23)
    const s_sum = TarHeader.computeSignedChecksum(signed_blk[0..]);
    expect_true(s_sum > 0)?;
    TarHeader.writeOctal(signed_blk[148..155], s_sum as u64);
    signed_blk[155] = ' ';
    expect_true(TarHeader.verifyChecksum(signed_blk[0..]))?;
}

fn runTestSecurityBoundaries(): void! {
    // 3a. TarSlip all variants rejected
    const unsafe_paths: [12][]const u8 = [
        "",
        "..",
        "../etc/passwd",
        "a/../../b",
        "a/..",
        "/etc/passwd",
        "/",
        "\\windows\\win.ini",
        "C:boot.ini",
        "C:\\escape.txt",
        "a\\..\\b",
        "bad\x00name.txt",
    ];
    let i: usize = 0;
    while (i < unsafe_paths.len) : (i += 1) {
        expect_eq(validateArchivePath(unsafe_paths[i]), false);
    }

    const safe_paths: [5][]const u8 = [
        "a.txt",
        "pkg/src/main.cp",
        ".gitignore",
        "dir/file..name.txt",
        "a/b/c/d/e/f/g/h/i/j/k/l/m/n/o/p/q/ok.txt",
    ];
    i = 0;
    while (i < safe_paths.len) : (i += 1) {
        expect_eq(validateArchivePath(safe_paths[i]), true);
    }

    // 3b. Hardlink traversal rejected in strict mode
    let hl_buf: [1536]u8 = [0; 1536];
    * = TarHeader.encodeHeader(hl_buf[0..512], "safe_link", 0, 0o644, 0, '1');
    TarHeader.copyPayloadSimd(hl_buf[157..170], "../../etc/pw");
    finalizeHeaderChecksum(hl_buf[0..512]);
    let hl_reader = PaxTarReader.init(hl_buf[0..], true);
    expect_true(hl_reader.next() == null)?;

    // 3c. Tampered header checksum rejected
    let bad_sum: [1024]u8 = [0; 1024];
    * = TarHeader.encodeHeader(bad_sum[0..512], "ok.txt", 0, 0o644, 0, '0');
    bad_sum[10] ^= 0x01; // flip 1 bit without updating checksum
    let bad_tr = TarReader.init(bad_sum[0..]);
    expect_true(bad_tr.next() == null)?;
    let bad_pr = PaxTarReader.init(bad_sum[0..], true);
    expect_true(bad_pr.next() == null)?;

    // 3d. Truncated payload (declared size > remaining buffer) rejected
    let trunc_buf: [1024]u8 = [0; 1024];
    * = TarHeader.encodeHeader(trunc_buf[0..512], "big.bin", 999999, 0o644, 0, '0');
    let trunc_tr = TarReader.init(trunc_buf[0..]);
    expect_true(trunc_tr.next() == null)?;
    let trunc_pr = PaxTarReader.init(trunc_buf[0..], true);
    expect_true(trunc_pr.next() == null)?;
}

fn runTestSimdParity(): void! {
    // 4a. isZeroBlockSimd vs isZeroBlockScalar
    let blk: [512]u8 = [0; 512];
    expect_eq(TarHeader.isZeroBlockSimd(blk[0..]), TarHeader.isZeroBlockScalar(blk[0..]));
    expect_true(TarHeader.isZeroBlockSimd(blk[0..]))?;

    const probe_indices: [5]usize = [0, 15, 16, 257, 511];
    let pi: usize = 0;
    while (pi < probe_indices.len) : (pi += 1) {
        const idx = probe_indices[pi];
        blk[idx] = 0x01;
        expect_eq(TarHeader.isZeroBlockSimd(blk[0..]), TarHeader.isZeroBlockScalar(blk[0..]));
        expect_eq(TarHeader.isZeroBlockSimd(blk[0..]), false);
        blk[idx] = 0;
    }

    // 4b. computeChecksumSimd vs computeChecksumScalar
    let hdr_blk: [512]u8 = [0; 512];
    let k: usize = 0;
    while (k < 512) : (k += 1) {
        hdr_blk[k] = ((k * 29 + 7) & 0xFF) as u8;
    }
    expect_eq(TarHeader.computeChecksumSimd(hdr_blk[0..]), TarHeader.computeChecksumScalar(hdr_blk[0..]));

    // 4c. copyPayloadSimd vs copyPayloadScalar
    let dst_sc: [257]u8 = [0; 257];
    let dst_sm: [257]u8 = [0; 257];
    const lens: [7]usize = [0, 1, 15, 16, 17, 64, 257];
    let li: usize = 0;
    while (li < lens.len) : (li += 1) {
        const n = lens[li];
        @memset(dst_sc[0..n], 0);
        @memset(dst_sm[0..n], 0);
        TarHeader.copyPayloadScalar(dst_sc[0..n], hdr_blk[0..n]);
        TarHeader.copyPayloadSimd(dst_sm[0..n], hdr_blk[0..n]);
        expect_true(bytesEqual(dst_sc[0..n], dst_sm[0..n]))?;
    }

    // 4d. validateArchivePath (cap:simd) vs validateArchivePathScalar
    const paths: [8][]const u8 = [
        "a",
        "very/long/safe/relative/path/without/special/chars/file_name_0123456789.tar",
        "dir/sub/..hidden_ok",
        "dir/../evil",
        "long/path/before/backslash\\file",
        "long/path/before/colon:ads",
        "long/path/before/nul\x00byte",
        "a/b/c/d/e/f/g/h/i/j/k/l/m/n/o/p/q/..",
    ];
    let p_idx: usize = 0;
    while (p_idx < paths.len) : (p_idx += 1) {
        expect_eq(validateArchivePath(paths[p_idx]), validateArchivePathScalar(paths[p_idx]));
    }
}

fn runTestMalformedFuzz(): void! {
    let seed: u64 = 0x243F6A8885A308D3;
    let buf: [1024]u8 = [0; 1024];
    let case_idx: usize = 0;
    while (case_idx < 1300) : (case_idx += 1) {
        seed ^= seed << 13;
        seed ^= seed >> 7;
        seed ^= seed << 17;
        const len: usize = ((seed % 1023) as usize) + 1;

        let j: usize = 0;
        while (j < len) : (j += 1) {
            seed ^= seed << 13;
            seed ^= seed >> 7;
            seed ^= seed << 17;
            buf[j] = (seed & 0xFF) as u8;
        }

        // Occasionally inject valid checksum with random typeflag/PAX/Base-256 payload
        if ((case_idx % 4) == 0 && len >= 512) {
            if ((case_idx % 8) == 0) {
                buf[156] = 'x';
            } else if ((case_idx % 12) == 0) {
                buf[156] = 'L';
            }
            finalizeHeaderChecksum(buf[0..512]);
        }

        const slice = buf[0..len];
        if (len >= 512) {
            expect_eq(TarHeader.isZeroBlockSimd(slice[0..512]), TarHeader.isZeroBlockScalar(slice[0..512]));
            expect_eq(TarHeader.computeChecksumSimd(slice[0..512]), TarHeader.computeChecksumScalar(slice[0..512]));
        }

        let tr = TarReader.init(slice);
        let steps: usize = 0;
        while (steps < 8) : (steps += 1) {
            if (tr.next() == null) break;
        }

        let pr = PaxTarReader.init(slice, true);
        steps = 0;
        while (steps < 8) : (steps += 1) {
            if (pr.next() == null) break;
        }
    }
}

fn main() {
    test("1. USTAR / PAX / tar-rs real vector & backward-compatible TarReader", () => {
        runTestUstarPaxAndTarRsVectors()?;
    });
    test("2. GNU Base-256 (>8GB & negative), GNU LongName ('L'), PAX Global ('g') & signed checksum", () => {
        runTestGnuBase256GlobalPaxAndSignedChecksum()?;
    });
    test("3. Security boundaries: TarSlip, hardlink traversal, checksum tamper & truncated entry", () => {
        runTestSecurityBoundaries()?;
    });
    test("4. 16-lane SIMD vs Scalar 0-diff parity (isZeroBlock, computeChecksum, copyPayload, validateArchivePath)", () => {
        runTestSimdParity()?;
    });
    test("5. 1,300 deterministic PRNG malformed TAR/PAX/GNU fuzz cases (0 panic / 0 OOB)", () => {
        runTestMalformedFuzz()?;
    });
}
