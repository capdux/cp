// cap/test/testCapMagicBytes.cp
// Comprehensive unit tests for @cap/magic-bytes

import { test, expect_eq } from "cap:test";
import {
    KIND_UNKNOWN,
    KIND_IMAGE,
    KIND_ARCHIVE,
    KIND_DOCUMENT,
    KIND_EXECUTABLE,
    KIND_AUDIO,
    KIND_VIDEO,
    KIND_DATABASE,
    detect,
    isImage,
    isArchive,
    isExecutable,
} from "../../packages/@cap/magic-bytes/src/index.cp";

fn sliceEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn main() {
    test("1. @cap/magic-bytes — Image formats (PNG, JPEG, GIF, WebP)", () => {
        let png: [8]u8 = undefined;
        png[0] = 0x89 as u8; png[1] = 0x50 as u8; png[2] = 0x4E as u8; png[3] = 0x47 as u8;
        png[4] = 0x0D as u8; png[5] = 0x0A as u8; png[6] = 0x1A as u8; png[7] = 0x0A as u8;
        const ftPng = detect(png[0..8]);
        expect_eq(ftPng.matched, true);
        expect_eq(ftPng.kind, KIND_IMAGE);
        expect_eq(sliceEq(ftPng.ext, "png"), true);
        expect_eq(sliceEq(ftPng.mime, "image/png"), true);
        expect_eq(isImage(png[0..8]), true);

        let jpg: [4]u8 = undefined;
        jpg[0] = 0xFF as u8; jpg[1] = 0xD8 as u8; jpg[2] = 0xFF as u8; jpg[3] = 0xE0 as u8;
        expect_eq(sliceEq(detect(jpg[0..4]).ext, "jpg"), true);

        expect_eq(sliceEq(detect("GIF89a......").ext, "gif"), true);
        expect_eq(sliceEq(detect("RIFF1234WEBP").ext, "webp"), true);
    });

    test("2. @cap/magic-bytes — Archive formats (ZIP, GZIP, ZSTD, XZ, 7Z)", () => {
        let zip: [4]u8 = undefined;
        zip[0] = 0x50 as u8; zip[1] = 0x4B as u8; zip[2] = 0x03 as u8; zip[3] = 0x04 as u8;
        expect_eq(sliceEq(detect(zip[0..4]).ext, "zip"), true);
        expect_eq(isArchive(zip[0..4]), true);

        let gz: [3]u8 = undefined;
        gz[0] = 0x1F as u8; gz[1] = 0x8B as u8; gz[2] = 0x08 as u8;
        expect_eq(sliceEq(detect(gz[0..3]).ext, "gz"), true);

        let zst: [4]u8 = undefined;
        zst[0] = 0x28 as u8; zst[1] = 0xB5 as u8; zst[2] = 0x2F as u8; zst[3] = 0xFD as u8;
        expect_eq(sliceEq(detect(zst[0..4]).ext, "zst"), true);

        let xz: [6]u8 = undefined;
        xz[0] = 0xFD as u8; xz[1] = 0x37 as u8; xz[2] = 0x7A as u8; xz[3] = 0x58 as u8; xz[4] = 0x5A as u8; xz[5] = 0x00 as u8;
        expect_eq(sliceEq(detect(xz[0..6]).ext, "xz"), true);

        let z7: [6]u8 = undefined;
        z7[0] = 0x37 as u8; z7[1] = 0x7A as u8; z7[2] = 0xBC as u8; z7[3] = 0xAF as u8; z7[4] = 0x27 as u8; z7[5] = 0x1C as u8;
        const ft7z = detect(z7[0..6]);
        expect_eq(ft7z.kind, KIND_ARCHIVE);
        expect_eq(sliceEq(ft7z.ext, "7z"), true);
    });

    test("3. @cap/magic-bytes — Document (PDF) & Database (SQLite3)", () => {
        const ftPdf = detect("%PDF-1.7\n");
        expect_eq(ftPdf.matched, true);
        expect_eq(ftPdf.kind, KIND_DOCUMENT);
        expect_eq(sliceEq(ftPdf.ext, "pdf"), true);

        let sq: [16]u8 = undefined;
        const hdr = "SQLite format 3";
        let i: usize = 0;
        while (i < 15) {
            sq[i] = hdr[i];
            i += 1;
        }
        sq[15] = 0x00 as u8;
        const ftSq = detect(sq[0..16]);
        expect_eq(ftSq.matched, true);
        expect_eq(ftSq.kind, KIND_DATABASE);
        expect_eq(sliceEq(ftSq.ext, "sqlite"), true);
    });

    test("4. @cap/magic-bytes — Executables (ELF, WASM, PE/MZ, Mach-O)", () => {
        let elf: [4]u8 = undefined;
        elf[0] = 0x7F as u8; elf[1] = 0x45 as u8; elf[2] = 0x4C as u8; elf[3] = 0x46 as u8;
        expect_eq(sliceEq(detect(elf[0..4]).ext, "elf"), true);
        expect_eq(isExecutable(elf[0..4]), true);

        let wasm: [4]u8 = undefined;
        wasm[0] = 0x00 as u8; wasm[1] = 0x61 as u8; wasm[2] = 0x73 as u8; wasm[3] = 0x6D as u8;
        const ftWasm = detect(wasm[0..4]);
        expect_eq(ftWasm.kind, KIND_EXECUTABLE);
        expect_eq(sliceEq(ftWasm.mime, "application/wasm"), true);

        expect_eq(sliceEq(detect("MZ\x90\x00").ext, "exe"), true);

        let macho: [4]u8 = undefined;
        macho[0] = 0xCF as u8; macho[1] = 0xFA as u8; macho[2] = 0xED as u8; macho[3] = 0xFE as u8;
        expect_eq(sliceEq(detect(macho[0..4]).ext, "macho"), true);
    });

    test("5. @cap/magic-bytes — Audio (OGG, FLAC, WAV, MP3) & Video (MP4)", () => {
        expect_eq(detect("OggS1234").kind, KIND_AUDIO);
        expect_eq(sliceEq(detect("fLaC1234").ext, "flac"), true);
        expect_eq(sliceEq(detect("RIFF0000WAVE").ext, "wav"), true);
        expect_eq(sliceEq(detect("ID3\x04\x00").ext, "mp3"), true);

        let mp4: [12]u8 = undefined;
        mp4[0] = 0x00 as u8; mp4[1] = 0x00 as u8; mp4[2] = 0x00 as u8; mp4[3] = 0x18 as u8;
        mp4[4] = 'f' as u8; mp4[5] = 't' as u8; mp4[6] = 'y' as u8; mp4[7] = 'p' as u8;
        mp4[8] = 'i' as u8; mp4[9] = 's' as u8; mp4[10] = 'o' as u8; mp4[11] = 'm' as u8;
        const ftMp4 = detect(mp4[0..12]);
        expect_eq(ftMp4.kind, KIND_VIDEO);
        expect_eq(sliceEq(ftMp4.ext, "mp4"), true);
    });

    test("6. @cap/magic-bytes — Empty, short & unknown random bytes return matched=false", () => {
        const emptyFt = detect("");
        expect_eq(emptyFt.matched, false);
        expect_eq(emptyFt.kind, KIND_UNKNOWN);

        let oneByte: [1]u8 = undefined;
        oneByte[0] = 0xFF as u8;
        expect_eq(detect(oneByte[0..1]).matched, false);

        expect_eq(detect("plain text hello world").matched, false);
        expect_eq(isImage("plain text"), false);
        expect_eq(isArchive("plain text"), false);
        expect_eq(isExecutable("plain text"), false);
    });
}
