// Source: /data/dev/bun/src/zstd/zstd.cp
// ============================================================================
// lib/pack/zstd.cp — Zstandard (RFC 8478) 压缩/解压缩与帧解析引擎
// 吸收自 Bun src/zstd/zstd.cp，适配 CP 现代强类型与原生内存生命周期
// ============================================================================

import { ZSTD_compress, ZSTD_decompress, ZSTD_compressBound, ZSTD_findDecompressedSize, CompressBoundEstimator } from "./_codecs.cp";

/// Zstandard 帧魔数 (RFC 8478 Section 3.1.1: 0xFD2FB528, 小端序: 28 B5 2F FD)
pub const ZSTD_MAGIC: u32 = 0xFD2FB528;

/// Zstandard 可跳过帧魔数前缀 (0x184D2A50 ~ 0x184D2A5F)
pub const ZSTD_SKIPPABLE_MAGIC_MIN: u32 = 0x184D2A50;
pub const ZSTD_SKIPPABLE_MAGIC_MAX: u32 = 0x184D2A5F;

pub const ZSTD_DEFAULT_CLEVEL: i32 = 3;
pub const ZSTD_MAX_CLEVEL: i32 = 22;
pub const ZSTD_MIN_CLEVEL: i32 = 1;

/// Zstd 帧头元数据解析视图
pub struct ZstdFrameHeader {
    magic: u32 = ZSTD_MAGIC,
    single_segment: bool = false,
    content_checksum: bool = false,
    dictionary_id: u32 = 0,
    window_size: u64 = 0,
    content_size: u64 = 0,
    header_size: usize = 0,

    /// 校验切片是否以合法的 Zstandard 帧头起始
    pub static fn isZstdHeader(src: []const u8): bool {
        if (src.len < 4) { return false; }
        const magic = (src[0] as u32) | ((src[1] as u32) << 8) | ((src[2] as u32) << 16) | ((src[3] as u32) << 24);
        return magic == ZSTD_MAGIC;
    }

    /// 校验是否为可跳过帧 (Skippable Frame)
    pub static fn isSkippableHeader(src: []const u8): bool {
        if (src.len < 4) { return false; }
        const magic = (src[0] as u32) | ((src[1] as u32) << 8) | ((src[2] as u32) << 16) | ((src[3] as u32) << 24);
        return magic >= ZSTD_SKIPPABLE_MAGIC_MIN && magic <= ZSTD_SKIPPABLE_MAGIC_MAX;
    }

    /// 从原始字节流中快速解析 Zstandard 帧描述符 (RFC 8478 Section 3.1.1.1)
    pub static fn parse(src: []const u8): ZstdFrameHeader? {
        if (src.len < 5) { return null; }
        const magic = (src[0] as u32) | ((src[1] as u32) << 8) | ((src[2] as u32) << 16) | ((src[3] as u32) << 24);
        if (magic != ZSTD_MAGIC) { return null; }

        const fhd = src[4];
        // Reserved bit (bit 3) and unused bit (bit 4) must be 0 per RFC 8478
        const single_segment = (fhd & 0x20) != 0;
        const content_checksum = (fhd & 0x04) != 0;
        const dict_id_flag = fhd & 0x03;
        const fcs_flag = (fhd >> 6) & 0x03;

        let pos: usize = 5;
        let window_size: u64 = 0;
        if (!single_segment) {
            if (pos >= src.len) { return null; }
            const win_desc = src[pos];
            pos += 1;
            const exp = (win_desc >> 3) & 0x1F;
            const mantissa = win_desc & 0x07;
            let base: u64 = 1024;
            let e: u8 = 0;
            while (e < exp) {
                base = base << 1;
                e += 1;
            }
            window_size = base + (base >> 3) * (mantissa as u64);
        }

        let dict_id: u32 = 0;
        if (dict_id_flag == 1) {
            if (pos + 1 > src.len) { return null; }
            dict_id = src[pos] as u32;
            pos += 1;
        } else if (dict_id_flag == 2) {
            if (pos + 2 > src.len) { return null; }
            dict_id = (src[pos] as u32) | ((src[pos + 1] as u32) << 8);
            pos += 2;
        } else if (dict_id_flag == 3) {
            if (pos + 4 > src.len) { return null; }
            dict_id = (src[pos] as u32)
                | ((src[pos + 1] as u32) << 8)
                | ((src[pos + 2] as u32) << 16)
                | ((src[pos + 3] as u32) << 24);
            pos += 4;
        }

        let content_size: u64 = 0;
        if (fcs_flag == 0) {
            if (single_segment) {
                if (pos >= src.len) { return null; }
                content_size = src[pos] as u64;
                pos += 1;
            }
        } else if (fcs_flag == 1) {
            if (pos + 2 > src.len) { return null; }
            content_size = ((src[pos] as u64) | ((src[pos + 1] as u64) << 8)) + 256;
            pos += 2;
        } else if (fcs_flag == 2) {
            if (pos + 4 > src.len) { return null; }
            content_size = (src[pos] as u64)
                | ((src[pos + 1] as u64) << 8)
                | ((src[pos + 2] as u64) << 16)
                | ((src[pos + 3] as u64) << 24);
            pos += 4;
        } else if (fcs_flag == 3) {
            if (pos + 8 > src.len) { return null; }
            const low32: u64 = ((src[pos] as u32)
                | ((src[pos + 1] as u32) << 8)
                | ((src[pos + 2] as u32) << 16)
                | ((src[pos + 3] as u32) << 24)) as u64;
            const high32: u64 = ((src[pos + 4] as u32)
                | ((src[pos + 5] as u32) << 8)
                | ((src[pos + 6] as u32) << 16)
                | ((src[pos + 7] as u32) << 24)) as u64;
            content_size = low32 | (high32 << 32);
            pos += 8;
        }

        if (single_segment) {
            window_size = content_size;
        }

        return ZstdFrameHeader {
            magic: magic,
            single_segment: single_segment,
            content_checksum: content_checksum,
            dictionary_id: dict_id,
            window_size: window_size,
            content_size: content_size,
            header_size: pos,
        };
    }
}

/// Zstandard 面向对象门面与核心算法容器
pub struct Zstd {
    /// 纯算术预估最大压缩输出大小（零 C 依赖）
    pub static fn bound(source_len: usize): usize {
        return CompressBoundEstimator.zstdBound(source_len);
    }

    /// 查询 C-FFI 官方推荐分配大小
    pub static fn compressBound(source_len: usize): usize {
        return CompressBoundEstimator.zstdBound(source_len);
    }

    /// 解析 Zstd 压缩包内的原始解压后体积
    pub static fn findDecompressedSize(src: []const u8): u64? {
        const hdr = ZstdFrameHeader.parse(src);
        if (hdr != null && (hdr.?.content_size > 0 || hdr.?.single_segment)) {
            return hdr.?.content_size;
        }
        return null;
    }

    /// 单次执行 Zstd 压缩至预分配目标切片
    pub static fn compress(dest: []u8, src: []const u8, level: i32 = ZSTD_DEFAULT_CLEVEL): usize? {
        if (dest.len < Zstd.bound(src.len)) {
            return null;
        }
        const out_size = ZSTD_compress(dest.ptr, dest.len, src.ptr, src.len, level);
        if (out_size == 0) {
            return null;
        }
        return out_size;
    }

    /// 单次执行 Zstd 解压至预分配目标切片
    pub static fn decompress(dest: []u8, src: []const u8): usize? {
        if (!ZstdFrameHeader.isZstdHeader(src)) {
            return null;
        }
        const out_size = ZSTD_decompress(dest.ptr, dest.len, src.ptr, src.len);
        if (out_size == 0) {
            return null;
        }
        return out_size;
    }

    /// 自动分配缓冲区并完成解压
    pub static fn decompressAlloc(src: []const u8): ([]u8)? {
        const expected_size = Zstd.findDecompressedSize(src);
        const target_size: usize = if (expected_size != null && expected_size.? > 0) (expected_size.? as usize) else 65536;

        let raw_buf: [*]u8 = @alloc(target_size);
        let out_buf = unsafe { raw_buf[0..target_size] };

        const actual_size = Zstd.decompress(out_buf, src);
        if (actual_size == null) {
            return null;
        }
        return out_buf[0 .. actual_size.?];
    }
}
