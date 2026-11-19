// Source: Bun src/zlib/zlib.cp (RFC 1952 / RFC 1950 / RFC 1951 Native Engine)
// ============================================================================
// lib/pack/gzip.cp — RFC 1952 Gzip & RFC 1950/1951 Deflate/Inflate 工业级压缩与解压引擎
// 吸收自 Bun src/zlib/zlib.cp，由底层 vendor/zlib 纯静态库 (libz.a) 驱动
// ============================================================================

import { Crc32 } from "./checksum.cp";
import {
    ZStream,
    NativeZlib,
    zlibVersion,
} from "./_codecs.cp";

// ============================================================================
// 1. RFC 1952 规范常量
// ============================================================================

pub const GZIP_MAGIC_ID1: u8 = 0x1F;
pub const GZIP_MAGIC_ID2: u8 = 0x8B;
pub const GZIP_METHOD_DEFLATE: u8 = 8;

pub const GZIP_FTEXT: u8 = 0x01;
pub const GZIP_FHCRC: u8 = 0x02;
pub const GZIP_FEXTRA: u8 = 0x04;
pub const GZIP_FNAME: u8 = 0x08;
pub const GZIP_FCOMMENT: u8 = 0x10;

// WindowBits 参数配置约定（驱动 zlib 内部状态机）
pub const WINDOW_BITS_ZLIB: i32 = 15;        // RFC 1950 标准 Zlib 封装
pub const WINDOW_BITS_RAW: i32 = -15;        // RFC 1951 裸 Deflate 流（无头尾）
pub const WINDOW_BITS_GZIP: i32 = 31;        // RFC 1952 标准 Gzip 封装 (15 + 16)
pub const WINDOW_BITS_AUTO: i32 = 47;        // 自动探测 Zlib 或 Gzip 格式 (15 + 32)

// ============================================================================
// 2. RFC 1952 10 字节 Gzip 帧头与 8 字节帧尾编解码
// ============================================================================

pub struct GzipHeader {
    flags: u8 = 0,
    mtime: u32 = 0,
    xfl: u8 = 0,
    os: u8 = 3, // 3 = Unix

    /// 将 10 字节标准 Gzip Header 写入 out[0..10]
    pub fn write(out: []u8): usize {
        if (out.len < 10) { return 0; }
        out[0] = GZIP_MAGIC_ID1;
        out[1] = GZIP_MAGIC_ID2;
        out[2] = GZIP_METHOD_DEFLATE;
        out[3] = _.flags;
        out[4] = (_.mtime & 0xFF) as u8;
        out[5] = ((_.mtime >> 8) & 0xFF) as u8;
        out[6] = ((_.mtime >> 16) & 0xFF) as u8;
        out[7] = ((_.mtime >> 24) & 0xFF) as u8;
        out[8] = _.xfl;
        out[9] = _.os;
        return 10;
    }

    /// 解析 10 字节标准 Gzip Header
    pub static fn parse(buf: []const u8): GzipHeader? {
        if (buf.len < 10) { return null; }
        if (buf[0] != GZIP_MAGIC_ID1 || buf[1] != GZIP_MAGIC_ID2 || buf[2] != GZIP_METHOD_DEFLATE) {
            return null;
        }
        let mtime: u32 = (buf[4] as u32)
            | ((buf[5] as u32) << 8)
            | ((buf[6] as u32) << 16)
            | ((buf[7] as u32) << 24);
        return GzipHeader{
            flags: buf[3],
            mtime: mtime,
            xfl: buf[8],
            os: buf[9],
        };
    }

    /// 写入 8 字节小端序 Gzip Trailer (CRC32 + Uncompressed ISIZE mod 2^32)
    pub static fn writeTrailer(out: []u8, crc32_val: u32, isize_val: u32): usize {
        if (out.len < 8) { return 0; }
        out[0] = (crc32_val & 0xFF) as u8;
        out[1] = ((crc32_val >> 8) & 0xFF) as u8;
        out[2] = ((crc32_val >> 16) & 0xFF) as u8;
        out[3] = ((crc32_val >> 24) & 0xFF) as u8;
        out[4] = (isize_val & 0xFF) as u8;
        out[5] = ((isize_val >> 8) & 0xFF) as u8;
        out[6] = ((isize_val >> 16) & 0xFF) as u8;
        out[7] = ((isize_val >> 24) & 0xFF) as u8;
        return 8;
    }

    /// 校验 8 字节 Gzip Trailer 是否与预期数据匹配
    pub static fn verifyTrailer(trailer: []const u8, payload: []const u8): bool {
        if (trailer.len < 8) { return false; }
        let expected_crc: u32 = (trailer[0] as u32)
            | ((trailer[1] as u32) << 8)
            | ((trailer[2] as u32) << 16)
            | ((trailer[3] as u32) << 24);
        let expected_size: u32 = (trailer[4] as u32)
            | ((trailer[5] as u32) << 8)
            | ((trailer[6] as u32) << 16)
            | ((trailer[7] as u32) << 24);
        let actual_crc = Crc32.checksum(payload);
        let actual_size = (payload.len & 0xFFFFFFFF) as u32;
        return expected_crc == actual_crc && expected_size == actual_size;
    }
}

// ============================================================================
// 3. 压缩配置参数
// ============================================================================

pub struct GzipOptions {
    level: i32 = 6,        // 压缩级别: 0 (无压缩) 到 9 (极致压缩), -1 为默认
    mem_level: i32 = 8,    // 内部工作区大小 (1-9), 默认 8 为最优性能/内存折中
    strategy: i32 = 0,     // 0 = 默认策略, 1 = 滤波, 2 = 仅霍夫曼, 3 = RLE, 4 = 固定
}

// ============================================================================
// 4. 底层 Zlib 状态机驱动核心（带动态扩容保证）
// ============================================================================

fn compressWithWindowBits(src: []const u8, window_bits: i32, level: i32): []u8 {
    if (src.len == 0) {
        // 空字符串边界：即使空输入，Gzip/Zlib 仍需输出合法的空帧头/帧尾
        let strm = ZStream{
            next_in: null,
            avail_in: 0,
            total_in: 0,
            next_out: null,
            avail_out: 0,
            total_out: 0,
            msg: null,
            state: null,
            zalloc: null,
            zfree: null,
            opaque_ctx: null,
            data_type: 0,
            adler: 0,
            reserved: 0,
        };
        let init_rc = NativeZlib.deflateInit2_(
            &strm,
            level,
            GZIP_METHOD_DEFLATE as i32,
            window_bits,
            8,
            0,
            zlibVersion(),
            @sizeOf(ZStream) as i32,
        );
        if (init_rc != 0) { return ""; }
        let bound = NativeZlib.deflateBound(&strm, 0) + 64;
        let buf: [*]u8 = @alloc(bound);
        strm.next_out = buf;
        strm.avail_out = bound as u32;
        * = NativeZlib.deflate(&strm, 4); // Z_FINISH
        let out_len: usize = strm.total_out;
        * = NativeZlib.deflateEnd(&strm);
        return buf[0..out_len];
    }

    let strm = ZStream{
        next_in: src.ptr,
        avail_in: src.len as u32,
        total_in: 0,
        next_out: null,
        avail_out: 0,
        total_out: 0,
        msg: null,
        state: null,
        zalloc: null,
        zfree: null,
        opaque_ctx: null,
        data_type: 0,
        adler: 0,
        reserved: 0,
    };

    let init_rc = NativeZlib.deflateInit2_(
        &strm,
        level,
        GZIP_METHOD_DEFLATE as i32,
        window_bits,
        8,
        0,
        zlibVersion(),
        @sizeOf(ZStream) as i32,
    );
    if (init_rc != 0) { return ""; }

    let bound = NativeZlib.deflateBound(&strm, src.len) + 64;
    let out_buf: [*]u8 = @alloc(bound);
    strm.next_out = out_buf;
    strm.avail_out = bound as u32;

    let res = NativeZlib.deflate(&strm, 4); // 4 = Z_FINISH
    if (res != 1) { // 1 = Z_STREAM_END
        * = NativeZlib.deflateEnd(&strm);
        return "";
    }

    let comp_len: usize = strm.total_out;
    * = NativeZlib.deflateEnd(&strm);
    return out_buf[0..comp_len];
}

fn decompressWithWindowBits(src: []const u8, window_bits: i32): ([]u8)? {
    if (src.len == 0) {
        return "";
    }

    let strm = ZStream{
        next_in: src.ptr,
        avail_in: src.len as u32,
        total_in: 0,
        next_out: null,
        avail_out: 0,
        total_out: 0,
        msg: null,
        state: null,
        zalloc: null,
        zfree: null,
        opaque_ctx: null,
        data_type: 0,
        adler: 0,
        reserved: 0,
    };

    let init_rc = NativeZlib.inflateInit2_(
        &strm,
        window_bits,
        zlibVersion(),
        @sizeOf(ZStream) as i32,
    );
    if (init_rc != 0) {
        return null;
    }

    // 容量启发式预估：
    // 若为 Gzip 格式且长度充足，提取尾部 4 字节的 ISIZE 作为极速容量锚点
    let initial_cap: usize = src.len * 2 + 1024;
    if (src.len >= 18 && src[0] == GZIP_MAGIC_ID1 && src[1] == GZIP_MAGIC_ID2) {
        let isize_hint: usize = (src[src.len - 4] as usize)
            | ((src[src.len - 3] as usize) << 8)
            | ((src[src.len - 2] as usize) << 16)
            | ((src[src.len - 1] as usize) << 24);
        if (isize_hint > 0 && isize_hint <= 256 * 1024 * 1024) {
            initial_cap = isize_hint + 32;
        }
    }

    let cur_cap: usize = initial_cap;
    let out_buf: [*]u8 = @alloc(cur_cap);
    strm.next_out = out_buf;
    strm.avail_out = cur_cap as u32;

    while (true) {
        let ret = NativeZlib.inflate(&strm, 0); // 0 = Z_NO_FLUSH
        if (ret == 1) { // 1 = Z_STREAM_END
            let final_len: usize = strm.total_out;
            * = NativeZlib.inflateEnd(&strm);
            return out_buf[0..final_len];
        }
        if (ret != 0 && ret != 1) {
            * = NativeZlib.inflateEnd(&strm);
            return null;
        }

        // 缓冲区耗尽，触发动态倍增扩容
        if (strm.avail_out == 0) {
            let used: usize = strm.total_out;
            let next_cap: usize = cur_cap * 2;
            let new_buf: [*]u8 = @alloc(next_cap);
            @memcpy(new_buf[0..used], out_buf[0..used]);
            out_buf = new_buf;
            cur_cap = next_cap;
            strm.next_out = &out_buf[used];
            strm.avail_out = (next_cap - used) as u32;
        }
    }
}

// ============================================================================
// 5. 面向用户的标准化顶级 API (与 Bun / Node.js 100% 对标)
// ============================================================================

/// 将数据使用 RFC 1952 规范压缩为 Gzip 二进制格式（含 10 字节头与 CRC32/ISIZE 尾）
pub fn gzip(src: []const u8, level: i32 = 6): []u8 {
    return compressWithWindowBits(src, WINDOW_BITS_GZIP, level);
}

/// 解压符合 RFC 1952 规范的 Gzip 二进制流（支持自动探测 Zlib/Gzip）
pub fn gunzip(src: []const u8): ([]u8)? {
    return decompressWithWindowBits(src, WINDOW_BITS_AUTO);
}

/// 将数据使用 RFC 1950 规范压缩为 Zlib Deflate 格式（含 2 字节头与 Adler32 尾）
pub fn deflate(src: []const u8, level: i32 = 6): []u8 {
    return compressWithWindowBits(src, WINDOW_BITS_ZLIB, level);
}

/// 解压符合 RFC 1950 规范的 Zlib Deflate 数据流
pub fn inflate(src: []const u8): ([]u8)? {
    return decompressWithWindowBits(src, WINDOW_BITS_ZLIB);
}

/// 将数据使用 RFC 1951 规范压缩为裸 Deflate 流（无任何头尾，供 HTTP Chunk / Zip 局部流使用）
pub fn deflateRaw(src: []const u8, level: i32 = 6): []u8 {
    return compressWithWindowBits(src, WINDOW_BITS_RAW, level);
}

/// 解压 RFC 1951 裸 Deflate 数据流
pub fn inflateRaw(src: []const u8): ([]u8)? {
    return decompressWithWindowBits(src, WINDOW_BITS_RAW);
}

// ============================================================================
// 6. 面向对象便捷包装类 (Gzip / Gunzip / DeflateCodec / InflateCodec)
// ============================================================================

pub struct Gzip {
    pub static fn compress(src: []const u8, level: i32 = 6): []u8 {
        return gzip(src, level);
    }

    pub static fn sync(src: []const u8): []u8 {
        return gzip(src, 6);
    }
}

pub struct Gunzip {
    pub static fn decompress(src: []const u8): ([]u8)? {
        return gunzip(src);
    }

    pub static fn sync(src: []const u8): ([]u8)? {
        return gunzip(src);
    }
}

pub struct DeflateCodec {
    pub static fn compress(src: []const u8, level: i32 = 6): []u8 {
        return deflate(src, level);
    }

    pub static fn sync(src: []const u8): []u8 {
        return deflate(src, 6);
    }
}

pub struct InflateCodec {
    pub static fn decompress(src: []const u8): ([]u8)? {
        return inflate(src);
    }

    pub static fn sync(src: []const u8): ([]u8)? {
        return inflate(src);
    }
}

pub struct DeflateRawCodec {
    pub static fn compress(src: []const u8, level: i32 = 6): []u8 {
        return deflateRaw(src, level);
    }

    pub static fn decompress(src: []const u8): ([]u8)? {
        return inflateRaw(src);
    }
}

// ============================================================================
// 7. 流式分块压缩与解压引擎 (Streaming Chunked Pipeline)
// 对标 Bun 中的 NewZlibReader 与 ZlibCompressorArrayList，支持分块读写与超大文件流
// ============================================================================

pub class GzipStream {
    strm: ZStream,
    is_active: bool = false,

    pub static fn create(level: i32 = 6, window_bits: i32 = WINDOW_BITS_GZIP): (*GzipStream)? {
        let raw: [*]u8 = @alloc(@sizeOf(GzipStream));
        let comp: *GzipStream = @ptrCast(@alignCast(raw));
        comp.strm = ZStream{
            next_in: null,
            avail_in: 0,
            total_in: 0,
            next_out: null,
            avail_out: 0,
            total_out: 0,
            msg: null,
            state: null,
            zalloc: null,
            zfree: null,
            opaque_ctx: null,
            data_type: 0,
            adler: 0,
            reserved: 0,
        };
        comp.is_active = false;

        let rc = NativeZlib.deflateInit2_(
            &comp.strm,
            level,
            GZIP_METHOD_DEFLATE as i32,
            window_bits,
            8,
            0,
            zlibVersion(),
            @sizeOf(ZStream) as i32,
        );

        if (rc != 0) {
            return null;
        }

        comp.is_active = true;
        return comp;
    }

    pub fn write(*_, chunk: []const u8): []u8 {
        if (!_.is_active || chunk.len == 0) {
            return "";
        }

        _.strm.next_in = chunk.ptr;
        _.strm.avail_in = chunk.len as u32;

        let out_cap: usize = chunk.len + 256;
        let out_buf: [*]u8 = @alloc(out_cap);
        _.strm.next_out = out_buf;
        _.strm.avail_out = out_cap as u32;

        let initial_total: usize = _.strm.total_out;
        * = NativeZlib.deflate(&_.strm, 2); // 2 = Z_SYNC_FLUSH
        let produced: usize = _.strm.total_out - initial_total;

        return out_buf[0..produced];
    }

    pub fn finish(*_): []u8 {
        if (!_.is_active) {
            return "";
        }

        _.strm.next_in = null;
        _.strm.avail_in = 0;

        let out_cap: usize = 256;
        let out_buf: [*]u8 = @alloc(out_cap);
        _.strm.next_out = out_buf;
        _.strm.avail_out = out_cap as u32;

        let initial_total: usize = _.strm.total_out;
        * = NativeZlib.deflate(&_.strm, 4); // 4 = Z_FINISH
        let produced: usize = _.strm.total_out - initial_total;

        * = NativeZlib.deflateEnd(&_.strm);
        _.is_active = false;

        return out_buf[0..produced];
    }
}

pub class GunzipStream {
    strm: ZStream,
    is_active: bool = false,

    pub static fn create(window_bits: i32 = WINDOW_BITS_AUTO): (*GunzipStream)? {
        let raw: [*]u8 = @alloc(@sizeOf(GunzipStream));
        let decomp: *GunzipStream = @ptrCast(@alignCast(raw));
        decomp.strm = ZStream{
            next_in: null,
            avail_in: 0,
            total_in: 0,
            next_out: null,
            avail_out: 0,
            total_out: 0,
            msg: null,
            state: null,
            zalloc: null,
            zfree: null,
            opaque_ctx: null,
            data_type: 0,
            adler: 0,
            reserved: 0,
        };
        decomp.is_active = false;

        let rc = NativeZlib.inflateInit2_(
            &decomp.strm,
            window_bits,
            zlibVersion(),
            @sizeOf(ZStream) as i32,
        );

        if (rc != 0) {
            return null;
        }

        decomp.is_active = true;
        return decomp;
    }

    pub fn write(*_, chunk: []const u8): []u8 {
        if (!_.is_active || chunk.len == 0) {
            return "";
        }

        _.strm.next_in = chunk.ptr;
        _.strm.avail_in = chunk.len as u32;

        let out_cap: usize = chunk.len * 4 + 1024;
        let out_buf: [*]u8 = @alloc(out_cap);
        _.strm.next_out = out_buf;
        _.strm.avail_out = out_cap as u32;

        let initial_total: usize = _.strm.total_out;
        while (_.strm.avail_in > 0) {
            let ret = NativeZlib.inflate(&_.strm, 2); // Z_SYNC_FLUSH
            if (ret == 1 || ret != 0) {
                break;
            }
            if (_.strm.avail_out == 0) {
                let used: usize = _.strm.total_out - initial_total;
                let next_cap: usize = out_cap * 2;
                let new_buf: [*]u8 = @alloc(next_cap);
                @memcpy(new_buf[0..used], out_buf[0..used]);
                out_buf = new_buf;
                out_cap = next_cap;
                _.strm.next_out = &out_buf[used];
                _.strm.avail_out = (next_cap - used) as u32;
            }
        }

        let produced: usize = _.strm.total_out - initial_total;
        return out_buf[0..produced];
    }

    pub fn finish(*_): []u8 {
        if (!_.is_active) {
            return "";
        }

        _.strm.next_in = null;
        _.strm.avail_in = 0;

        let out_cap: usize = 256;
        let out_buf: [*]u8 = @alloc(out_cap);
        _.strm.next_out = out_buf;
        _.strm.avail_out = out_cap as u32;

        let initial_total: usize = _.strm.total_out;
        * = NativeZlib.inflate(&_.strm, 4); // Z_FINISH
        let produced: usize = _.strm.total_out - initial_total;

        * = NativeZlib.inflateEnd(&_.strm);
        _.is_active = false;

        return out_buf[0..produced];
    }
}

pub class DeflateStream {
    strm: ZStream,
    is_active: bool = false,

    pub static fn create(level: i32 = 6): (*DeflateStream)? {
        let raw: [*]u8 = @alloc(@sizeOf(DeflateStream));
        let comp: *DeflateStream = @ptrCast(@alignCast(raw));
        comp.strm = ZStream{
            next_in: null,
            avail_in: 0,
            total_in: 0,
            next_out: null,
            avail_out: 0,
            total_out: 0,
            msg: null,
            state: null,
            zalloc: null,
            zfree: null,
            opaque_ctx: null,
            data_type: 0,
            adler: 0,
            reserved: 0,
        };
        comp.is_active = false;

        let rc = NativeZlib.deflateInit2_(
            &comp.strm,
            level,
            GZIP_METHOD_DEFLATE as i32,
            WINDOW_BITS_ZLIB,
            8,
            0,
            zlibVersion(),
            @sizeOf(ZStream) as i32,
        );

        if (rc != 0) {
            return null;
        }

        comp.is_active = true;
        return comp;
    }

    pub fn write(*_, chunk: []const u8): []u8 {
        if (!_.is_active || chunk.len == 0) {
            return "";
        }

        _.strm.next_in = chunk.ptr;
        _.strm.avail_in = chunk.len as u32;

        let out_cap: usize = chunk.len + 256;
        let out_buf: [*]u8 = @alloc(out_cap);
        _.strm.next_out = out_buf;
        _.strm.avail_out = out_cap as u32;

        let initial_total: usize = _.strm.total_out;
        * = NativeZlib.deflate(&_.strm, 2);
        let produced: usize = _.strm.total_out - initial_total;

        return out_buf[0..produced];
    }

    pub fn finish(*_): []u8 {
        if (!_.is_active) {
            return "";
        }

        _.strm.next_in = null;
        _.strm.avail_in = 0;

        let out_cap: usize = 256;
        let out_buf: [*]u8 = @alloc(out_cap);
        _.strm.next_out = out_buf;
        _.strm.avail_out = out_cap as u32;

        let initial_total: usize = _.strm.total_out;
        * = NativeZlib.deflate(&_.strm, 4);
        let produced: usize = _.strm.total_out - initial_total;

        * = NativeZlib.deflateEnd(&_.strm);
        _.is_active = false;

        return out_buf[0..produced];
    }
}

pub class InflateStream {
    strm: ZStream,
    is_active: bool = false,

    pub static fn create(): (*InflateStream)? {
        let raw: [*]u8 = @alloc(@sizeOf(InflateStream));
        let decomp: *InflateStream = @ptrCast(@alignCast(raw));
        decomp.strm = ZStream{
            next_in: null,
            avail_in: 0,
            total_in: 0,
            next_out: null,
            avail_out: 0,
            total_out: 0,
            msg: null,
            state: null,
            zalloc: null,
            zfree: null,
            opaque_ctx: null,
            data_type: 0,
            adler: 0,
            reserved: 0,
        };
        decomp.is_active = false;

        let rc = NativeZlib.inflateInit2_(
            &decomp.strm,
            WINDOW_BITS_ZLIB,
            zlibVersion(),
            @sizeOf(ZStream) as i32,
        );

        if (rc != 0) {
            return null;
        }

        decomp.is_active = true;
        return decomp;
    }

    pub fn write(*_, chunk: []const u8): []u8 {
        if (!_.is_active || chunk.len == 0) {
            return "";
        }

        _.strm.next_in = chunk.ptr;
        _.strm.avail_in = chunk.len as u32;

        let out_cap: usize = chunk.len * 4 + 1024;
        let out_buf: [*]u8 = @alloc(out_cap);
        _.strm.next_out = out_buf;
        _.strm.avail_out = out_cap as u32;

        let initial_total: usize = _.strm.total_out;
        while (_.strm.avail_in > 0) {
            let ret = NativeZlib.inflate(&_.strm, 2);
            if (ret == 1 || ret != 0) {
                break;
            }
            if (_.strm.avail_out == 0) {
                let used: usize = _.strm.total_out - initial_total;
                let next_cap: usize = out_cap * 2;
                let new_buf: [*]u8 = @alloc(next_cap);
                @memcpy(new_buf[0..used], out_buf[0..used]);
                out_buf = new_buf;
                out_cap = next_cap;
                _.strm.next_out = &out_buf[used];
                _.strm.avail_out = (next_cap - used) as u32;
            }
        }

        let produced: usize = _.strm.total_out - initial_total;
        return out_buf[0..produced];
    }

    pub fn finish(*_): []u8 {
        if (!_.is_active) {
            return "";
        }

        _.strm.next_in = null;
        _.strm.avail_in = 0;

        let out_cap: usize = 256;
        let out_buf: [*]u8 = @alloc(out_cap);
        _.strm.next_out = out_buf;
        _.strm.avail_out = out_cap as u32;

        let initial_total: usize = _.strm.total_out;
        * = NativeZlib.inflate(&_.strm, 4);
        let produced: usize = _.strm.total_out - initial_total;

        * = NativeZlib.inflateEnd(&_.strm);
        _.is_active = false;

        return out_buf[0..produced];
    }
}

// ============================================================================
// 8. 零依赖 Run-Length Encoding (RLE) 编解码器（保留向后兼容）
// ============================================================================

pub struct RleCodec {
    pub static fn compress(src: []const u8, out: []u8): usize {
        let in_pos: usize = 0;
        let out_pos: usize = 0;
        while (in_pos < src.len) {
            let b = src[in_pos];
            let run_len: usize = 1;
            while (in_pos + run_len < src.len && src[in_pos + run_len] == b && run_len < 255) {
                run_len += 1;
            }
            if (run_len >= 4 || b == 0xFF) {
                if (out_pos + 3 > out.len) { return 0; }
                out[out_pos] = 0xFF;
                out[out_pos + 1] = run_len as u8;
                out[out_pos + 2] = b;
                out_pos += 3;
                in_pos += run_len;
            } else {
                if (out_pos + 1 > out.len) { return 0; }
                out[out_pos] = b;
                out_pos += 1;
                in_pos += 1;
            }
        }
        return out_pos;
    }

    pub static fn decompress(src: []const u8, out: []u8): usize {
        let in_pos: usize = 0;
        let out_pos: usize = 0;
        while (in_pos < src.len) {
            let b = src[in_pos];
            if (b == 0xFF) {
                if (in_pos + 3 > src.len) { return 0; }
                let count = src[in_pos + 1] as usize;
                let val = src[in_pos + 2];
                if (out_pos + count > out.len) { return 0; }
                let k: usize = 0;
                while (k < count) {
                    out[out_pos + k] = val;
                    k += 1;
                }
                out_pos += count;
                in_pos += 3;
            } else {
                if (out_pos + 1 > out.len) { return 0; }
                out[out_pos] = b;
                out_pos += 1;
                in_pos += 1;
            }
        }
        return out_pos;
    }
}
