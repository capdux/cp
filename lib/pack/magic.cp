// Source: cp/lib/pack/magic.cp (Archive & Compression Format Sniffer)
// ============================================================================
// lib/pack/magic.cp — 归档与压缩容器魔数嗅探引擎 (ArchiveSniffer)
// 支持 Gzip / Zlib / Zstd / Zip / Tar / Xz / Bz2 / Lz4 / BrotliHint 识别与 MIME 映射
// ============================================================================

import { BrotliHeader } from "./brotli.cp";

/// 归档与压缩格式魔数识别枚举（避免与 _archive.cp 中的 libarchive C 常量 ArchiveFormat 重名）
pub enum ArchiveMagicFormat: u8 {
    Unknown = 0,
    Gzip = 1,
    Zlib = 2,
    Zstd = 3,
    Zip = 4,
    Tar = 5,
    Xz = 6,
    Bz2 = 7,
    Lz4 = 8,
    BrotliHint = 9,
}

/// 零分配、纯字节切片驱动的归档与压缩格式魔数嗅探器
pub struct ArchiveSniffer {
    /// 基于二进制魔数与 RFC/POSIX 头部校验自动识别容器格式
    pub static fn detect(bytes: []const u8): ArchiveMagicFormat {
        if (bytes.len == 0) {
            return ArchiveMagicFormat.Unknown;
        }

        // 1. Gzip (RFC 1952): 1F 8B [08]
        if (bytes.len >= 2 && bytes[0] == 0x1F && bytes[1] == 0x8B) {
            if (bytes.len == 2 || bytes[2] == 0x08) {
                return ArchiveMagicFormat.Gzip;
            }
        }

        // 2. Zstandard (RFC 8478): 28 B5 2F FD 或 Skippable Frame 50..5F 2A 4D 18
        if (bytes.len >= 4) {
            if (bytes[0] == 0x28 && bytes[1] == 0xB5 && bytes[2] == 0x2F && bytes[3] == 0xFD) {
                return ArchiveMagicFormat.Zstd;
            }
            if ((bytes[0] & 0xF0) == 0x50 && bytes[1] == 0x2A && bytes[2] == 0x4D && bytes[3] == 0x18) {
                return ArchiveMagicFormat.Zstd;
            }
        }

        // 3. PKZIP: 50 4B 03 04 (Local), 50 4B 05 06 (Empty EOCD), 50 4B 07 08 (Spanned)
        if (bytes.len >= 4 && bytes[0] == 0x50 && bytes[1] == 0x4B) {
            if ((bytes[2] == 0x03 && bytes[3] == 0x04)
                || (bytes[2] == 0x05 && bytes[3] == 0x06)
                || (bytes[2] == 0x07 && bytes[3] == 0x08)) {
                return ArchiveMagicFormat.Zip;
            }
        }

        // 4. XZ Utils: FD 37 7A 58 5A 00
        if (bytes.len >= 6
            && bytes[0] == 0xFD
            && bytes[1] == 0x37
            && bytes[2] == 0x7A
            && bytes[3] == 0x58
            && bytes[4] == 0x5A
            && bytes[5] == 0x00) {
            return ArchiveMagicFormat.Xz;
        }

        // 5. Bzip2: "BZh" (42 5A 68) + block size '1'..'9'
        if (bytes.len >= 3 && bytes[0] == 0x42 && bytes[1] == 0x5A && bytes[2] == 0x68) {
            if (bytes.len == 3 || (bytes[3] >= '1' && bytes[3] <= '9')) {
                return ArchiveMagicFormat.Bz2;
            }
        }

        // 6. LZ4 Frame: 04 22 4D 18
        if (bytes.len >= 4
            && bytes[0] == 0x04
            && bytes[1] == 0x22
            && bytes[2] == 0x4D
            && bytes[3] == 0x18) {
            return ArchiveMagicFormat.Lz4;
        }

        // 7. POSIX USTAR / GNU TAR: 偏移 257 处为 "ustar\0" 或 "ustar "
        if (bytes.len >= 263
            && bytes[257] == 'u'
            && bytes[258] == 's'
            && bytes[259] == 't'
            && bytes[260] == 'a'
            && bytes[261] == 'r'
            && (bytes[262] == 0 || bytes[262] == ' ')) {
            return ArchiveMagicFormat.Tar;
        }

        // 8. Zlib (RFC 1950): CM == 8 (Deflate), CINFO <= 7, (CMF * 256 + FLG) % 31 == 0
        if (bytes.len >= 2) {
            const cmf = bytes[0];
            const flg = bytes[1];
            const cm = cmf & 0x0F;
            const cinfo = (cmf >> 4) & 0x0F;
            const check = (((cmf as u16) * 256) + (flg as u16)) % 31;
            if (cm == 8 && cinfo <= 7 && check == 0) {
                return ArchiveMagicFormat.Zlib;
            }
        }

        // 9. Brotli (RFC 7932): 识别明确空终止帧头 (ISLAST=1 & ISLASTEMPTY=1)
        if (bytes.len == 1 && bytes[0] == 0x06) {
            return ArchiveMagicFormat.BrotliHint;
        }
        if (bytes.len == 1 && (bytes[0] & 0x01) == 1 && ((bytes[0] >> 1) & 0x07) != 0 && (bytes[0] & 0xF0) == 0x30) {
            return ArchiveMagicFormat.BrotliHint;
        }
        if (bytes.len == 2 && (bytes[0] & 0x0F) == 0x01 && (bytes[0] & 0x80) != 0 && bytes[1] == 0x01) {
            if (BrotliHeader.parse(bytes) != null) {
                return ArchiveMagicFormat.BrotliHint;
            }
        }

        return ArchiveMagicFormat.Unknown;
    }

    /// 辅助启发式检测字节流头部是否符合 RFC 7932 Brotli WBITS 规范
    pub static fn looksLikeBrotli(bytes: []const u8): bool {
        return BrotliHeader.parse(bytes) != null;
    }

    /// 返回格式对应的标准 MIME 类型字符串
    pub static fn mimeType(fmt: ArchiveMagicFormat): []const u8 {
        if (fmt == ArchiveMagicFormat.Gzip) { return "application/gzip"; }
        if (fmt == ArchiveMagicFormat.Zlib) { return "application/zlib"; }
        if (fmt == ArchiveMagicFormat.Zstd) { return "application/zstd"; }
        if (fmt == ArchiveMagicFormat.Zip) { return "application/zip"; }
        if (fmt == ArchiveMagicFormat.Tar) { return "application/x-tar"; }
        if (fmt == ArchiveMagicFormat.Xz) { return "application/x-xz"; }
        if (fmt == ArchiveMagicFormat.Bz2) { return "application/x-bzip2"; }
        if (fmt == ArchiveMagicFormat.Lz4) { return "application/x-lz4"; }
        if (fmt == ArchiveMagicFormat.BrotliHint) { return "application/x-brotli"; }
        return "application/octet-stream";
    }

    /// 返回格式对应的常规文件扩展名
    pub static fn extension(fmt: ArchiveMagicFormat): []const u8 {
        if (fmt == ArchiveMagicFormat.Gzip) { return ".gz"; }
        if (fmt == ArchiveMagicFormat.Zlib) { return ".zz"; }
        if (fmt == ArchiveMagicFormat.Zstd) { return ".zst"; }
        if (fmt == ArchiveMagicFormat.Zip) { return ".zip"; }
        if (fmt == ArchiveMagicFormat.Tar) { return ".tar"; }
        if (fmt == ArchiveMagicFormat.Xz) { return ".xz"; }
        if (fmt == ArchiveMagicFormat.Bz2) { return ".bz2"; }
        if (fmt == ArchiveMagicFormat.Lz4) { return ".lz4"; }
        if (fmt == ArchiveMagicFormat.BrotliHint) { return ".br"; }
        return "";
    }
}
