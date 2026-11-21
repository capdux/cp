// Source: Absorbed from /data/dev/bun/src/runtime/image/codec_png.cp | Synced: 2026-11-20T11:18:32Z
// ============================================================================
// Absorbed from Bun: bun/src/runtime/image/codec_png.cp
// Upstream Sync: Refer to bun/src/runtime/image/codec_png.zig for future upstream updates.
// ============================================================================
// ============================================================================
// lib/image/png.cp — 工业级纯 CP 原生 PNG 图像编解码器
// 专为 CapSurf 网页截图、Canvas 像素导出与无损图像处理设计
// 符合 W3C PNG 规范与 RFC 2083，支持 RGBA/RGB 编码、全滤镜解码与 CRC32 校验
// ============================================================================

import { Crc32, compress, uncompress, compressBound } from "cap:pack";
import { readFileSync, writeFileSync } from "cap:fs";

/// PNG 颜色类型（RFC 2083 定义）
pub enum PngColorType: u8 {
    Grayscale = 0,
    Rgb = 2,
    Indexed = 3,
    GrayscaleAlpha = 4,
    Rgba = 6,
}

/// PNG 扫描线滤波类型
pub enum PngFilterType: u8 {
    None = 0,
    Sub = 1,
    Up = 2,
    Average = 3,
    Paeth = 4,
}

/// 标准 RGBA 像素四元组
pub struct RgbaColor {
    pub r: u8 = 0,
    pub g: u8 = 0,
    pub b: u8 = 0,
    pub a: u8 = 255,

    pub fn eql(*_, other: RgbaColor): bool {
        return _.r == other.r && _.g == other.g && _.b == other.b && _.a == other.a;
    }
}

/// 解码后或待渲染的 PNG 图像对象
pub struct PngImage {
    pub width: u32 = 0,
    pub height: u32 = 0,
    pub color_type: PngColorType = PngColorType.Rgba,
    pub bit_depth: u8 = 8,
    pub rgba: []u8 = undefined,

    /// 获取坐标 (x, y) 的 RGBA 像素颜色
    pub fn getPixel(*_, x: u32, y: u32): RgbaColor {
        if (x >= _.width || y >= _.height) {
            return RgbaColor{ r: 0, g: 0, b: 0, a: 0 };
        }
        let idx = ((y * _.width + x) * 4) as usize;
        if (idx + 3 >= _.rgba.len) {
            return RgbaColor{ r: 0, g: 0, b: 0, a: 0 };
        }
        return RgbaColor{
            r: _.rgba[idx],
            g: _.rgba[idx + 1],
            b: _.rgba[idx + 2],
            a: _.rgba[idx + 3],
        };
    }

    /// 设置坐标 (x, y) 的像素颜色
    pub fn setPixel(*_, x: u32, y: u32, r: u8, g: u8, b: u8, a: u8) {
        if (x >= _.width || y >= _.height) return;
        let idx = ((y * _.width + x) * 4) as usize;
        if (idx + 3 >= _.rgba.len) return;
        _.rgba[idx] = r;
        _.rgba[idx + 1] = g;
        _.rgba[idx + 2] = b;
        _.rgba[idx + 3] = a;
    }

    /// 编码为 PNG 格式字节流
    pub fn encode(*_): []u8 {
        return Png.encodeRgba(_.rgba, _.width, _.height);
    }

    /// 直接保存为 PNG 文件
    pub fn save(*_, path: string): bool {
        return Png.save(path, _.rgba, _.width, _.height);
    }
}

pub class Png {
    /// 大端序读取 32 位整数
    static fn readU32Be(b: []const u8, offset: usize): u32 {
        if (offset + 4 > b.len) return 0;
        let b0 = b[offset] as u32;
        let b1 = b[offset + 1] as u32;
        let b2 = b[offset + 2] as u32;
        let b3 = b[offset + 3] as u32;
        return (b0 << 24) | (b1 << 16) | (b2 << 8) | b3;
    }

    /// 大端序写入 32 位整数
    static fn writeU32Be(b: []u8, offset: usize, val: u32) {
        if (offset + 4 > b.len) return;
        b[offset] = ((val >> 24) & 0xFF) as u8;
        b[offset + 1] = ((val >> 16) & 0xFF) as u8;
        b[offset + 2] = ((val >> 8) & 0xFF) as u8;
        b[offset + 3] = (val & 0xFF) as u8;
    }

    /// Paeth 预测器算法（RFC 2083 第 6 节）
    static fn paethPredictor(a: i32, b: i32, c: i32): u8 {
        let p = a + b - c;
        let pa = if (p > a) (p - a) else (a - p);
        let pb = if (p > b) (p - b) else (b - p);
        let pc = if (p > c) (p - c) else (c - p);
        if (pa <= pb && pa <= pc) return (a & 0xFF) as u8;
        if (pb <= pc) return (b & 0xFF) as u8;
        return (c & 0xFF) as u8;
    }

    /// 将 RGBA8 像素切片编码为标准 PNG 二进制字节流
    /// 专供 CapSurf 网页截图引擎与通用图形导出使用
    pub static fn encodeRgba(rgba: []const u8, width: u32, height: u32): []u8 {
        if (width == 0 || height == 0 || rgba.len < (width * height * 4) as usize) {
            return "";
        }

        // 1. 构造未压缩扫描线（每行前缀 1 字节滤波类型 0x00 = None）
        let row_stride = (width * 4) as usize;
        let filtered_row_len = row_stride + 1;
        let raw_scanlines_len = (height as usize) * filtered_row_len;
        let scanlines_ptr: [*]u8 = @alloc(raw_scanlines_len);
        let scanlines = unsafe { scanlines_ptr[0..raw_scanlines_len] };

        let y: usize = 0;
        while (y < height as usize) {
            let row_offset = y * filtered_row_len;
            scanlines[row_offset] = 0; // Filter None
            let src_offset = y * row_stride;
            let x: usize = 0;
            while (x < row_stride) {
                scanlines[row_offset + 1 + x] = rgba[src_offset + x];
                x += 1;
            }
            y += 1;
        }

        // 2. 使用 Zlib Deflate 压缩扫描线生成 IDAT 载荷
        let max_comp_bound = compressBound(raw_scanlines_len);
        let comp_buffer: [*]u8 = @alloc(max_comp_bound);
        let comp_len: usize = max_comp_bound;
        let zlib_code = compress(comp_buffer, &comp_len, scanlines.ptr, raw_scanlines_len);

        if (zlib_code != 0) {
            return "";
        }

        // 3. 组装最终 PNG 文件结构
        // - 8 字节文件签名
        // - IHDR Chunk: 4 len + 4 type + 13 data + 4 crc = 25 字节
        // - IDAT Chunk: 4 len + 4 type + comp_len data + 4 crc = 12 + comp_len 字节
        // - IEND Chunk: 4 len + 4 type + 0 data + 4 crc = 12 字节
        let total_png_size = 8 + 25 + (12 + comp_len) + 12;
        let out_png_ptr: [*]u8 = @alloc(total_png_size);
        let out_png = unsafe { out_png_ptr[0..total_png_size] };

        let mut_pos: usize = 0;

        // [3.1] PNG 签名
        out_png[0] = 0x89;
        out_png[1] = 0x50; // 'P'
        out_png[2] = 0x4E; // 'N'
        out_png[3] = 0x47; // 'G'
        out_png[4] = 0x0D; // '\r'
        out_png[5] = 0x0A; // '\n'
        out_png[6] = 0x1A; // '\x1a'
        out_png[7] = 0x0A; // '\n'
        mut_pos += 8;

        // [3.2] IHDR Chunk
        Png.writeU32Be(out_png, mut_pos, 13); // length
        out_png[mut_pos + 4] = 0x49; // 'I'
        out_png[mut_pos + 5] = 0x48; // 'H'
        out_png[mut_pos + 6] = 0x44; // 'D'
        out_png[mut_pos + 7] = 0x52; // 'R'
        Png.writeU32Be(out_png, mut_pos + 8, width);
        Png.writeU32Be(out_png, mut_pos + 12, height);
        out_png[mut_pos + 16] = 8; // bit depth: 8
        out_png[mut_pos + 17] = 6; // color type: 6 (RGBA)
        out_png[mut_pos + 18] = 0; // compression: deflate
        out_png[mut_pos + 19] = 0; // filter: adaptive
        out_png[mut_pos + 20] = 0; // interlace: none

        // IHDR CRC-32 (over type + data, len = 4 + 13 = 17)
        let ihdr_crc = Crc32.checksum(out_png[mut_pos + 4 .. mut_pos + 21]);
        Png.writeU32Be(out_png, mut_pos + 21, ihdr_crc);
        mut_pos += 25;

        // [3.3] IDAT Chunk
        Png.writeU32Be(out_png, mut_pos, comp_len as u32);
        out_png[mut_pos + 4] = 0x49; // 'I'
        out_png[mut_pos + 5] = 0x44; // 'D'
        out_png[mut_pos + 6] = 0x41; // 'A'
        out_png[mut_pos + 7] = 0x54; // 'T'

        // 拷贝压缩数据
        let ci: usize = 0;
        while (ci < comp_len) {
            out_png[mut_pos + 8 + ci] = unsafe { comp_buffer[ci] };
            ci += 1;
        }

        // IDAT CRC-32 (over type + data, len = 4 + comp_len)
        let idat_crc = Crc32.checksum(out_png[mut_pos + 4 .. mut_pos + 8 + comp_len]);
        Png.writeU32Be(out_png, mut_pos + 8 + comp_len, idat_crc);
        mut_pos += 12 + comp_len;

        // [3.4] IEND Chunk
        Png.writeU32Be(out_png, mut_pos, 0); // length 0
        out_png[mut_pos + 4] = 0x49; // 'I'
        out_png[mut_pos + 5] = 0x45; // 'E'
        out_png[mut_pos + 6] = 0x4E; // 'N'
        out_png[mut_pos + 7] = 0x44; // 'D'
        // IEND 常量 CRC: 0xAE426082
        let iend_crc = Crc32.checksum(out_png[mut_pos + 4 .. mut_pos + 8]);
        Png.writeU32Be(out_png, mut_pos + 8, iend_crc);

        return out_png;
    }

    /// 解码任意标准 PNG 字节流，还原为 RGBA8 像素图像
    pub static fn decode(bytes: []const u8): PngImage? {
        if (bytes.len < 8) return null;

        // 1. 验证签名
        if (bytes[0] != 0x89 || bytes[1] != 0x50 || bytes[2] != 0x4E || bytes[3] != 0x47 ||
            bytes[4] != 0x0D || bytes[5] != 0x0A || bytes[6] != 0x1A || bytes[7] != 0x0A) {
            return null;
        }

        let width: u32 = 0;
        let height: u32 = 0;
        let bit_depth: u8 = 8;
        let color_type_raw: u8 = 6;
        let plte_offset: usize = 0;
        let plte_len: usize = 0;
        let trns_offset: usize = 0;
        let trns_len: usize = 0;

        // 2. 遍历 Chunks 搜集 IHDR, PLTE, tRNS 与 IDAT 数据
        let offset: usize = 8;
        let total_idat_bytes: usize = 0;

        // 首轮扫描：确定尺寸、调色板与 IDAT 总长度（带严格边界防护）
        while (offset + 12 <= bytes.len) {
            let chunk_len = Png.readU32Be(bytes, offset) as usize;
            if (chunk_len > bytes.len - offset - 12) {
                return null;
            }
            let c0 = bytes[offset + 4];
            let c1 = bytes[offset + 5];
            let c2 = bytes[offset + 6];
            let c3 = bytes[offset + 7];

            if (c0 == 0x49 && c1 == 0x48 && c2 == 0x44 && c3 == 0x52) { // IHDR
                if (chunk_len < 13) {
                    return null;
                }
                width = Png.readU32Be(bytes, offset + 8);
                height = Png.readU32Be(bytes, offset + 12);
                bit_depth = bytes[offset + 16];
                color_type_raw = bytes[offset + 17];
            } else if (c0 == 0x50 && c1 == 0x4C && c2 == 0x54 && c3 == 0x45) { // PLTE
                plte_offset = offset + 8;
                plte_len = chunk_len;
            } else if (c0 == 0x74 && c1 == 0x52 && c2 == 0x4E && c3 == 0x53) { // tRNS
                trns_offset = offset + 8;
                trns_len = chunk_len;
            } else if (c0 == 0x49 && c1 == 0x44 && c2 == 0x41 && c3 == 0x54) { // IDAT
                total_idat_bytes += chunk_len;
            } else if (c0 == 0x49 && c1 == 0x45 && c2 == 0x4E && c3 == 0x44) { // IEND
                break;
            }
            offset += 12 + chunk_len;
        }

        if (width == 0 || height == 0 || total_idat_bytes == 0 || bit_depth != 8) {
            return null;
        }
        // Guard against width * height * 4 overflow
        if ((width as u64) * (height as u64) > 268435456) {
            return null;
        }

        // 3. 聚合所有 IDAT 压缩块
        let combined_ptr: [*]u8 = @alloc(total_idat_bytes);
        let combined_idat = unsafe { combined_ptr[0..total_idat_bytes] };

        offset = 8;
        let idat_write_pos: usize = 0;
        while (offset + 12 <= bytes.len) {
            let chunk_len = Png.readU32Be(bytes, offset) as usize;
            if (chunk_len > bytes.len - offset - 12) {
                return null;
            }
            let c0 = bytes[offset + 4];
            let c1 = bytes[offset + 5];
            let c2 = bytes[offset + 6];
            let c3 = bytes[offset + 7];

            if (c0 == 0x49 && c1 == 0x44 && c2 == 0x41 && c3 == 0x54) { // IDAT
                let di: usize = 0;
                while (di < chunk_len) {
                    combined_idat[idat_write_pos] = bytes[offset + 8 + di];
                    idat_write_pos += 1;
                    di += 1;
                }
            } else if (c0 == 0x49 && c1 == 0x45 && c2 == 0x4E && c3 == 0x44) {
                break;
            }
            offset += 12 + chunk_len;
        }

        // 4. 计算每像素字节数与解压后原始扫描线长度
        let bpp: usize = switch (color_type_raw) {
            6 => 4 as usize, // RGBA
            2 => 3 as usize, // RGB
            0 => 1 as usize, // Grayscale
            3 => 1 as usize, // Indexed
            4 => 2 as usize, // Grayscale + Alpha
            else => return null,
        };

        let row_bytes = (width as usize) * bpp;
        let decompressed_scanlines_len = (height as usize) * (row_bytes + 1);

        let uncomp_ptr: [*]u8 = @alloc(decompressed_scanlines_len);
        let uncomp_buf = unsafe { uncomp_ptr[0..decompressed_scanlines_len] };

        let actual_decomp_len: usize = decompressed_scanlines_len;
        let u_res = uncompress(uncomp_buf.ptr, &actual_decomp_len, combined_idat.ptr, total_idat_bytes);

        if (u_res != 0 || actual_decomp_len < decompressed_scanlines_len) {
            return null;
        }

        // 5. 逆滤波（Reverse Filtering）并输出为统一直觉的 RGBA8 像素矩阵
        let out_rgba_len = (width as usize) * (height as usize) * 4;
        let out_rgba_ptr: [*]u8 = @alloc(out_rgba_len);
        let out_rgba = unsafe { out_rgba_ptr[0..out_rgba_len] };

        let prev_recon_ptr: [*]u8 = @alloc(row_bytes);
        let prev_recon = unsafe { prev_recon_ptr[0..row_bytes] };
        @memset(prev_recon, 0);

        let curr_recon_ptr: [*]u8 = @alloc(row_bytes);
        let curr_recon = unsafe { curr_recon_ptr[0..row_bytes] };

        let y: usize = 0;
        let scan_src_offset: usize = 0;

        while (y < height as usize) {
            let filter_type = uncomp_buf[scan_src_offset];
            scan_src_offset += 1;

            let x: usize = 0;
            while (x < row_bytes) {
                let raw_val = uncomp_buf[scan_src_offset + x] as i32;
                let recon_val: u8 = 0;

                let left = if (x >= bpp) (curr_recon[x - bpp] as i32) else 0;
                let above = prev_recon[x] as i32;
                let upper_left = if (x >= bpp) (prev_recon[x - bpp] as i32) else 0;

                if (filter_type == 0) {
                    // None
                    recon_val = (raw_val & 0xFF) as u8;
                } else if (filter_type == 1) {
                    // Sub
                    recon_val = ((raw_val + left) & 0xFF) as u8;
                } else if (filter_type == 2) {
                    // Up
                    recon_val = ((raw_val + above) & 0xFF) as u8;
                } else if (filter_type == 3) {
                    // Average
                    let avg = (left + above) >> 1;
                    recon_val = ((raw_val + avg) & 0xFF) as u8;
                } else if (filter_type == 4) {
                    // Paeth
                    let paeth = Png.paethPredictor(left, above, upper_left) as i32;
                    recon_val = ((raw_val + paeth) & 0xFF) as u8;
                } else {
                    recon_val = (raw_val & 0xFF) as u8;
                }

                curr_recon[x] = recon_val;
                x += 1;
            }
            scan_src_offset += row_bytes;

            // 将当前行转换为目标 RGBA8 格式
            let px: usize = 0;
            while (px < width as usize) {
                let out_idx = (y * (width as usize) + px) * 4;
                if (color_type_raw == 6) {
                    // RGBA
                    let src_px = px * 4;
                    out_rgba[out_idx] = curr_recon[src_px];
                    out_rgba[out_idx + 1] = curr_recon[src_px + 1];
                    out_rgba[out_idx + 2] = curr_recon[src_px + 2];
                    out_rgba[out_idx + 3] = curr_recon[src_px + 3];
                } else if (color_type_raw == 2) {
                    // RGB -> RGBA (Alpha = 255)
                    let src_px = px * 3;
                    out_rgba[out_idx] = curr_recon[src_px];
                    out_rgba[out_idx + 1] = curr_recon[src_px + 1];
                    out_rgba[out_idx + 2] = curr_recon[src_px + 2];
                    out_rgba[out_idx + 3] = 255;
                } else if (color_type_raw == 0) {
                    // Grayscale
                    let gray = curr_recon[px];
                    out_rgba[out_idx] = gray;
                    out_rgba[out_idx + 1] = gray;
                    out_rgba[out_idx + 2] = gray;
                    out_rgba[out_idx + 3] = 255;
                } else if (color_type_raw == 3) {
                    // Indexed (PLTE + optional tRNS)
                    let pidx = curr_recon[px] as usize;
                    if (pidx * 3 + 2 < plte_len) {
                        let po = plte_offset + pidx * 3;
                        out_rgba[out_idx] = bytes[po];
                        out_rgba[out_idx + 1] = bytes[po + 1];
                        out_rgba[out_idx + 2] = bytes[po + 2];
                    } else {
                        out_rgba[out_idx] = 0;
                        out_rgba[out_idx + 1] = 0;
                        out_rgba[out_idx + 2] = 0;
                    }
                    out_rgba[out_idx + 3] = if (pidx < trns_len) bytes[trns_offset + pidx] else 255;
                } else if (color_type_raw == 4) {
                    // Grayscale + Alpha
                    let src_px = px * 2;
                    let gray = curr_recon[src_px];
                    let alpha = curr_recon[src_px + 1];
                    out_rgba[out_idx] = gray;
                    out_rgba[out_idx + 1] = gray;
                    out_rgba[out_idx + 2] = gray;
                    out_rgba[out_idx + 3] = alpha;
                }
                px += 1;
            }

            // 更新 prev_recon 行
            let cpy_i: usize = 0;
            while (cpy_i < row_bytes) {
                prev_recon[cpy_i] = curr_recon[cpy_i];
                cpy_i += 1;
            }

            y += 1;
        }

        let parsed_color_type: PngColorType = switch (color_type_raw) {
            0 => PngColorType.Grayscale,
            2 => PngColorType.Rgb,
            3 => PngColorType.Indexed,
            4 => PngColorType.GrayscaleAlpha,
            6 => PngColorType.Rgba,
            else => PngColorType.Rgba,
        };

        return PngImage{
            width: width,
            height: height,
            color_type: parsed_color_type,
            bit_depth: bit_depth,
            rgba: out_rgba,
        };
    }

    /// 将内存像素写入文件
    pub static fn save(path: string, rgba: []const u8, width: u32, height: u32): bool {
        let png_bytes = Png.encodeRgba(rgba, width, height);
        if (png_bytes.len == 0) return false;
        writeFileSync(path, png_bytes);
        return true;
    }

    /// 从文件读取并解码 PNG
    pub static fn load(path: string): PngImage? {
        let content = readFileSync(path);
        if (content.len == 0) return null;
        return Png.decode(content);
    }
}
