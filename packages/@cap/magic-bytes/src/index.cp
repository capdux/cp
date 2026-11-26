// packages/@cap/magic-bytes/src/index.cp
// Zero-allocation binary file magic bytes signature sniffer (25+ formats).
// MIT clean-room pure CP implementation.

pub const KIND_UNKNOWN: u8 = 0;
pub const KIND_IMAGE: u8 = 1;
pub const KIND_ARCHIVE: u8 = 2;
pub const KIND_DOCUMENT: u8 = 3;
pub const KIND_EXECUTABLE: u8 = 4;
pub const KIND_AUDIO: u8 = 5;
pub const KIND_VIDEO: u8 = 6;
pub const KIND_DATABASE: u8 = 7;

/// Detected file type metadata returned by `detect`.
pub struct FileType {
    pub ext: string,
    pub mime: string,
    pub kind: u8,
    pub matched: bool,
}

fn mbMatch(ext: string, mime: string, kind: u8): FileType {
    return FileType{
        ext: ext,
        mime: mime,
        kind: kind,
        matched: true,
    };
}

fn mbEqAt(data: []const u8, offset: usize, lit: []const u8): bool {
    if (offset + lit.len > data.len) {
        return false;
    }
    let i: usize = 0;
    while (i < lit.len) {
        if (data[offset + i] != lit[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

/// Zero-allocation binary magic bytes sniffer covering 25+ formats:
/// PNG, JPEG, GIF, WebP, BMP, ICO, PDF, ZIP, GZIP, ZSTD, LZ4, BZIP2, XZ, 7Z, TAR,
/// ELF, WASM, PE/MZ, Mach-O, SQLite3, MP4/ISO-BMFF, MP3/ID3, OGG, FLAC, WAV.
pub fn detect(data: []const u8): FileType {
    // 1. SQLite3: "SQLite format 3\0" (16 bytes)
    if (data.len >= 16 && mbEqAt(data, 0, "SQLite format 3") && data[15] == (0x00 as u8)) {
        return mbMatch("sqlite", "application/vnd.sqlite3", KIND_DATABASE);
    }

    // 2. PNG: 89 50 4E 47 0D 0A 1A 0A
    if (data.len >= 8 && data[0] == (0x89 as u8) && data[1] == (0x50 as u8) && data[2] == (0x4E as u8) && data[3] == (0x47 as u8) && data[4] == (0x0D as u8) && data[5] == (0x0A as u8) && data[6] == (0x1A as u8) && data[7] == (0x0A as u8)) {
        return mbMatch("png", "image/png", KIND_IMAGE);
    }

    // 3. GIF87a / GIF89a
    if (mbEqAt(data, 0, "GIF87a") || mbEqAt(data, 0, "GIF89a")) {
        return mbMatch("gif", "image/gif", KIND_IMAGE);
    }

    // 4. XZ: FD 37 7A 58 5A 00
    if (data.len >= 6 && data[0] == (0xFD as u8) && data[1] == (0x37 as u8) && data[2] == (0x7A as u8) && data[3] == (0x58 as u8) && data[4] == (0x5A as u8) && data[5] == (0x00 as u8)) {
        return mbMatch("xz", "application/x-xz", KIND_ARCHIVE);
    }

    // 5. 7Z: 37 7A BC AF 27 1C
    if (data.len >= 6 && data[0] == (0x37 as u8) && data[1] == (0x7A as u8) && data[2] == (0xBC as u8) && data[3] == (0xAF as u8) && data[4] == (0x27 as u8) && data[5] == (0x1C as u8)) {
        return mbMatch("7z", "application/x-7z-compressed", KIND_ARCHIVE);
    }

    // 6. PDF: "%PDF-"
    if (mbEqAt(data, 0, "%PDF-")) {
        return mbMatch("pdf", "application/pdf", KIND_DOCUMENT);
    }

    // 7. RIFF containers: WebP & WAV
    if (data.len >= 12 && mbEqAt(data, 0, "RIFF")) {
        if (mbEqAt(data, 8, "WEBP")) {
            return mbMatch("webp", "image/webp", KIND_IMAGE);
        }
        if (mbEqAt(data, 8, "WAVE")) {
            return mbMatch("wav", "audio/wav", KIND_AUDIO);
        }
    }

    // 8. MP4 / ISO-BMFF: "ftyp" at offset 4
    if (data.len >= 12 && mbEqAt(data, 4, "ftyp")) {
        return mbMatch("mp4", "video/mp4", KIND_VIDEO);
    }

    // 9. ELF: 7F 45 4C 46
    if (data.len >= 4 && data[0] == (0x7F as u8) && data[1] == (0x45 as u8) && data[2] == (0x4C as u8) && data[3] == (0x46 as u8)) {
        return mbMatch("elf", "application/x-executable", KIND_EXECUTABLE);
    }

    // 10. WASM: 00 61 73 6D
    if (data.len >= 4 && data[0] == (0x00 as u8) && data[1] == (0x61 as u8) && data[2] == (0x73 as u8) && data[3] == (0x6D as u8)) {
        return mbMatch("wasm", "application/wasm", KIND_EXECUTABLE);
    }

    // 11. Mach-O (32/64-bit LE/BE & Universal Fat)
    if (data.len >= 4) {
        const b0 = data[0];
        const b1 = data[1];
        const b2 = data[2];
        const b3 = data[3];
        const isMachBe = b0 == (0xFE as u8) && b1 == (0xED as u8) && b2 == (0xFA as u8) && (b3 == (0xCE as u8) || b3 == (0xCF as u8));
        const isMachLe = (b0 == (0xCE as u8) || b0 == (0xCF as u8)) && b1 == (0xFA as u8) && b2 == (0xED as u8) && b3 == (0xFE as u8);
        const isMachFat = b0 == (0xCA as u8) && b1 == (0xFE as u8) && b2 == (0xBA as u8) && b3 == (0xBE as u8);
        if (isMachBe || isMachLe || isMachFat) {
            return mbMatch("macho", "application/x-mach-binary", KIND_EXECUTABLE);
        }
    }

    // 12. ZIP: PK\x03\x04, PK\x05\x06, PK\x07\x08
    if (data.len >= 4 && data[0] == (0x50 as u8) && data[1] == (0x4B as u8)) {
        const z2 = data[2];
        const z3 = data[3];
        if ((z2 == (0x03 as u8) && z3 == (0x04 as u8)) || (z2 == (0x05 as u8) && z3 == (0x06 as u8)) || (z2 == (0x07 as u8) && z3 == (0x08 as u8))) {
            return mbMatch("zip", "application/zip", KIND_ARCHIVE);
        }
    }

    // 13. ZSTD: 28 B5 2F FD
    if (data.len >= 4 && data[0] == (0x28 as u8) && data[1] == (0xB5 as u8) && data[2] == (0x2F as u8) && data[3] == (0xFD as u8)) {
        return mbMatch("zst", "application/zstd", KIND_ARCHIVE);
    }

    // 14. LZ4: 04 22 4D 18
    if (data.len >= 4 && data[0] == (0x04 as u8) && data[1] == (0x22 as u8) && data[2] == (0x4D as u8) && data[3] == (0x18 as u8)) {
        return mbMatch("lz4", "application/x-lz4", KIND_ARCHIVE);
    }

    // 15. OGG: "OggS"
    if (mbEqAt(data, 0, "OggS")) {
        return mbMatch("ogg", "audio/ogg", KIND_AUDIO);
    }

    // 16. FLAC: "fLaC"
    if (mbEqAt(data, 0, "fLaC")) {
        return mbMatch("flac", "audio/flac", KIND_AUDIO);
    }

    // 17. ICO: 00 00 01 00
    if (data.len >= 4 && data[0] == (0x00 as u8) && data[1] == (0x00 as u8) && data[2] == (0x01 as u8) && data[3] == (0x00 as u8)) {
        return mbMatch("ico", "image/x-icon", KIND_IMAGE);
    }

    // 18. JPEG: FF D8 FF
    if (data.len >= 3 && data[0] == (0xFF as u8) && data[1] == (0xD8 as u8) && data[2] == (0xFF as u8)) {
        return mbMatch("jpg", "image/jpeg", KIND_IMAGE);
    }

    // 19. BZIP2: "BZh"
    if (mbEqAt(data, 0, "BZh")) {
        return mbMatch("bz2", "application/x-bzip2", KIND_ARCHIVE);
    }

    // 20. MP3 / ID3: "ID3" or MPEG frame sync FF FB / FF F3 / FF F2
    if (mbEqAt(data, 0, "ID3")) {
        return mbMatch("mp3", "audio/mpeg", KIND_AUDIO);
    }
    if (data.len >= 2 && data[0] == (0xFF as u8) && (data[1] == (0xFB as u8) || data[1] == (0xF3 as u8) || data[1] == (0xF2 as u8))) {
        return mbMatch("mp3", "audio/mpeg", KIND_AUDIO);
    }

    // 21. GZIP: 1F 8B
    if (data.len >= 2 && data[0] == (0x1F as u8) && data[1] == (0x8B as u8)) {
        return mbMatch("gz", "application/gzip", KIND_ARCHIVE);
    }

    // 22. TAR (POSIX ustar at offset 257)
    if (data.len >= 262 && mbEqAt(data, 257, "ustar")) {
        return mbMatch("tar", "application/x-tar", KIND_ARCHIVE);
    }

    // 23. BMP: "BM"
    if (data.len >= 2 && data[0] == (0x42 as u8) && data[1] == (0x4D as u8)) {
        return mbMatch("bmp", "image/bmp", KIND_IMAGE);
    }

    // 24. PE / MZ: "MZ"
    if (data.len >= 2 && data[0] == (0x4D as u8) && data[1] == (0x5A as u8)) {
        return mbMatch("exe", "application/vnd.microsoft.portable-executable", KIND_EXECUTABLE);
    }

    return FileType{
        ext: "",
        mime: "",
        kind: KIND_UNKNOWN,
        matched: false,
    };
}

/// Returns true if `data` matches a known image format signature.
pub fn isImage(data: []const u8): bool {
    const ft = detect(data);
    return ft.matched && ft.kind == KIND_IMAGE;
}

/// Returns true if `data` matches a known archive/compression format signature.
pub fn isArchive(data: []const u8): bool {
    const ft = detect(data);
    return ft.matched && ft.kind == KIND_ARCHIVE;
}

/// Returns true if `data` matches a known executable format signature (ELF, WASM, PE/MZ, Mach-O).
pub fn isExecutable(data: []const u8): bool {
    const ft = detect(data);
    return ft.matched && ft.kind == KIND_EXECUTABLE;
}

export {
    KIND_UNKNOWN,
    KIND_IMAGE,
    KIND_ARCHIVE,
    KIND_DOCUMENT,
    KIND_EXECUTABLE,
    KIND_AUDIO,
    KIND_VIDEO,
    KIND_DATABASE,
    FileType,
    detect,
    isImage,
    isArchive,
    isExecutable,
};
